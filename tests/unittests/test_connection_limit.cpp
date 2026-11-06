#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <cstring>

extern "C" {
#include "webserver.h"
#include "reverse_proxy.h"
}

/*
 * Per-address connection limit (system_sockets_container.c): which addresses
 * count as one client, the group bookkeeping, and which connection is closed
 * when a group is full. The accept path itself is exercised on the target
 * (Gabriel's test from QSXS-3113).
 */

namespace {

ws_peer_key key_v4(const char* text) {
    struct in_addr a;
    inet_pton(AF_INET, text, &a);
    ws_peer_key k;
    connLimitKeyFromAddress(&a, 4, &k);
    return k;
}

ws_peer_key key_v6(const char* text) {
    struct in6_addr a;
    inet_pton(AF_INET6, text, &a);
    ws_peer_key k;
    connLimitKeyFromAddress(&a, 16, &k);
    return k;
}

bool same(const ws_peer_key& a, const ws_peer_key& b) {
    return memcmp(a.addr, b.addr, sizeof(a.addr)) == 0;
}

/* A client socket as handleServer() leaves it right after accept */
socket_info* fresh_client() {
    socket_info* s = WebserverMallocSocketInfo();
    s->client = 1;
    s->socket = -1;
    s->header = WebserverMallocHttpRequestHeader();
    return s;
}

void free_client(socket_info* s) {
    WebserverFreeSocketInfo(s);
}

}  // namespace

/* ---------------------------------------------------------------- keys */

TEST(ConnLimitKeyTest, Ipv4AndMappedIpv6AreTheSameClient) {
    EXPECT_TRUE(same(key_v4("192.168.11.5"), key_v6("::ffff:192.168.11.5")));
    EXPECT_FALSE(same(key_v4("192.168.11.5"), key_v4("192.168.11.6")));
    EXPECT_EQ(key_v4("192.168.11.5").valid, 1);
}

TEST(ConnLimitKeyTest, GlobalIpv6CountsPerSlash64) {
    /* A host can use any address in its /64 */
    EXPECT_TRUE(same(key_v6("2001:db8:1:2::1"), key_v6("2001:db8:1:2:dead:beef:0:7")));
    EXPECT_FALSE(same(key_v6("2001:db8:1:2::1"), key_v6("2001:db8:1:3::1")));
}

TEST(ConnLimitKeyTest, LinkLocalAndUlaCountPerAddress) {
    /* All hosts of a segment share fe80::/64 - they must not share a group */
    EXPECT_FALSE(same(key_v6("fe80::1"), key_v6("fe80::2")));
    EXPECT_FALSE(same(key_v6("fd00::1"), key_v6("fd00::2")));
    EXPECT_FALSE(same(key_v6("::1"), key_v6("fe80::1")));
}

TEST(ConnLimitKeyTest, UnknownAddressTypeIsLimitedTogether) {
    ws_peer_key a, b;
    connLimitKeyFromAddress(nullptr, 0, &a);
    connLimitKeyFromAddress(nullptr, 7, &b);
    EXPECT_EQ(a.valid, 1) << "an unknown type must not bypass the limit";
    EXPECT_TRUE(same(a, b));
    EXPECT_FALSE(same(a, key_v4("0.0.0.0")));
}

/* -------------------------------------------------------------- groups */

TEST(ConnLimitGroupTest, SameAddressSameGroup) {
    ws_peer_key k = key_v4("10.0.0.1");
    struct conn_group_s* g1 = connLimitAcquire(&k);
    struct conn_group_s* g2 = connLimitAcquire(&k);
    ws_peer_key other = key_v4("10.0.0.2");
    struct conn_group_s* g3 = connLimitAcquire(&other);
    EXPECT_EQ(g1, g2);
    EXPECT_NE(g1, g3);
    connLimitRelease(g1);
    connLimitRelease(g2);
    connLimitRelease(g3);
}

TEST(ConnLimitGroupTest, CountsMembersAndSurvivesWhilePinned) {
    ws_peer_key k = key_v4("10.0.0.10");
    struct conn_group_s* g = connLimitAcquire(&k);
    socket_info* a = fresh_client();
    connLimitAttach(g, a);
    EXPECT_EQ(connLimitCount(g), 1u);

    /* An accept in progress holds the group while it closes the last member */
    struct conn_group_s* held = connLimitAcquire(&k);
    ASSERT_EQ(held, g);
    free_client(a);                     /* detaches */
    EXPECT_EQ(connLimitCount(held), 0u);

    socket_info* b = fresh_client();
    connLimitAttach(held, b);           /* still the same, valid group */
    EXPECT_EQ(connLimitCount(held), 1u);
    free_client(b);
}

TEST(ConnLimitGroupTest, ManyAddressesManyGroups) {
    /* More groups than buckets: chains must work, and all go away again */
    std::vector<socket_info*> socks;
    for (int i = 0; i < 1000; i++) {
        uint8_t a[4] = { 10, 1, (uint8_t)(i >> 8), (uint8_t)i };
        ws_peer_key k;
        connLimitKeyFromAddress(a, 4, &k);
        struct conn_group_s* g = connLimitAcquire(&k);
        socket_info* s = fresh_client();
        connLimitAttach(g, s);
        EXPECT_EQ(connLimitCount(g), 1u);
        socks.push_back(s);
    }
    for (socket_info* s : socks) {
        free_client(s);
    }
}

/* ------------------------------------------------------- what is idle */

TEST(ConnLimitEvictTest, WaitingForTheClientIsEvictable) {
    socket_info* s = fresh_client();
    EXPECT_EQ(connLimitIsEvictable(s), 1) << "just accepted / keep-alive";

    s->header_buffer_pos = 5;
    EXPECT_EQ(connLimitIsEvictable(s), 1) << "request header coming in";
    s->header_buffer_pos = 0;
    s->header->parsed_bytes = 20;
    EXPECT_EQ(connLimitIsEvictable(s), 1);
    s->header->parsed_bytes = 0;

    /* POST header done, body announced but not started: otherwise a client
     * sending POST headers and then nothing would hold its slots for good */
    s->header->header_complete = 1;
    EXPECT_EQ(connLimitIsEvictable(s), 1) << "body announced, nothing of it yet";
    s->header->header_complete = 0;

    free_client(s);
}

TEST(ConnLimitEvictTest, StartedUploadIsNotEvictable) {
    socket_info* s = fresh_client();
    s->header->header_complete = 1;
    s->header->post_buffer_pos = 1;
    EXPECT_EQ(connLimitIsEvictable(s), 0) << "buffered upload in progress";
    s->header->post_buffer_pos = 0;
    s->header->header_complete = 0;
    free_client(s);
}

TEST(ConnLimitEvictTest, WorkingConnectionsAreNeverEvictable) {
    socket_info* s = fresh_client();

    s->post_streaming_active = 1;
    EXPECT_EQ(connLimitIsEvictable(s), 0) << "streamed upload";
    s->post_streaming_active = 0;

    s->isWebsocket = 1;
    EXPECT_EQ(connLimitIsEvictable(s), 0);
    s->isWebsocket = 0;

    s->closeSocket = 1;
    EXPECT_EQ(connLimitIsEvictable(s), 0);
    s->closeSocket = 0;

    int dummy;
    ws_list_append(&s->output_list, &dummy, 0);   /* response waiting */
    EXPECT_EQ(connLimitIsEvictable(s), 0);
    ws_list_clear(&s->output_list);

    socket_info* no_header = WebserverMallocSocketInfo();
    EXPECT_EQ(connLimitIsEvictable(no_header), 0) << "unknown state counts as busy";
    WebserverFreeSocketInfo(no_header);

    EXPECT_EQ(connLimitIsEvictable(s), 1);
    free_client(s);
}

static void some_other_handler(int fd, void* ptr) {
    (void)fd; (void)ptr;
}

TEST(ConnLimitEvictTest, ProxyOnlyWhileReceivingTheRequest) {
    socket_info* s = fresh_client();
    reverse_proxy_connection_t proxy;
    memset(&proxy, 0, sizeof(proxy));
    s->extern_handle = reverse_proxy_client_handler;
    s->extern_handle_data_ptr = &proxy;

    proxy.state = PROXY_STATE_RECV_CLIENT_HEADER;
    EXPECT_EQ(connLimitIsEvictable(s), 1);
    proxy.state = PROXY_STATE_RECV_CLIENT_BODY;
    EXPECT_EQ(connLimitIsEvictable(s), 1) << "body announced, nothing of it yet";
    proxy.request_body_received = 100;
    EXPECT_EQ(connLimitIsEvictable(s), 0) << "upload through the proxy in progress";
    proxy.request_body_received = 0;
    proxy.state = PROXY_STATE_SEND_TO_BACKEND;
    EXPECT_EQ(connLimitIsEvictable(s), 0);
    proxy.state = PROXY_STATE_STREAMING;
    EXPECT_EQ(connLimitIsEvictable(s), 0);
    proxy.state = PROXY_STATE_WEBSOCKET_TUNNEL;
    EXPECT_EQ(connLimitIsEvictable(s), 0);

    /* Any other external handler: unknown, so busy */
    s->extern_handle = some_other_handler;
    EXPECT_EQ(connLimitIsEvictable(s), 0);

    s->extern_handle = nullptr;
    s->extern_handle_data_ptr = nullptr;
    free_client(s);
}

/* --------------------------------------------------------- the victim */

class ConnLimitVictimTest : public ::testing::Test {
protected:
    ws_peer_key key = key_v4("10.0.1.1");
    struct conn_group_s* group = nullptr;
    std::vector<socket_info*> socks;

    void SetUp() override { group = connLimitAcquire(&key); }

    socket_info* add() {
        socket_info* s = fresh_client();
        connLimitAttach(group, s);
        group = connLimitAcquire(&key);     /* keep holding it, as accept does */
        socks.push_back(s);
        return s;
    }

    void TearDown() override {
        for (socket_info* s : socks) {
            free_client(s);
        }
        connLimitRelease(group);
    }
};

TEST_F(ConnLimitVictimTest, OldestIdleIsChosen) {
    socket_info* a = add();
    socket_info* b = add();
    socket_info* c = add();
    (void)a; (void)c;
    CONN_LIMIT_NOTE_IO(a);              /* a, c used later than b */
    CONN_LIMIT_NOTE_IO(c);
    EXPECT_EQ(connLimitPickVictim(group), b);
}

TEST_F(ConnLimitVictimTest, OnlyTheTimeCountsNotTheState) {
    /* A connection stuck in a half sent header for long is older than a
     * fresh one - it goes first. Otherwise old slowloris connections would
     * stay and fresh ones would keep replacing each other in the last slot. */
    socket_info* stuck = add();
    stuck->header_buffer_pos = 3;
    socket_info* fresh = add();
    (void)fresh;
    EXPECT_EQ(connLimitPickVictim(group), stuck);
}

TEST_F(ConnLimitVictimTest, BusyOnesAreSkippedHoweverOld) {
    socket_info* busy = add();          /* oldest */
    busy->post_streaming_active = 1;
    socket_info* waiting = add();
    EXPECT_EQ(connLimitPickVictim(group), waiting);
    busy->post_streaming_active = 0;
}

TEST_F(ConnLimitVictimTest, NoVictimWhenAllAreBusy) {
    for (int i = 0; i < 5; i++) {
        add()->post_streaming_active = 1;
    }
    EXPECT_EQ(connLimitPickVictim(group), nullptr);
    for (socket_info* s : socks) {
        s->post_streaming_active = 0;
    }
}

TEST_F(ConnLimitVictimTest, NewcomerIsTheMostRecent) {
    socket_info* a = add();
    socket_info* b = add();
    EXPECT_GT(b->last_io, a->last_io);
    EXPECT_EQ(connLimitPickVictim(group), a);
}

/* -------------------------------------------------------------- config */

TEST(ConnLimitConfigTest, SetThroughTheConfig) {
    int before = connLimitGetMax();
    WebserverConfigSetInt("max_connections_per_ip", 5);
    EXPECT_EQ(connLimitGetMax(), 5);
    WebserverConfigSetInt("max_connections_per_ip", -3);
    EXPECT_EQ(connLimitGetMax(), 0) << "negative switches the limit off";
    WebserverConfigSetInt("max_connections_per_ip", before);
    EXPECT_EQ(connLimitGetMax(), before);
}
