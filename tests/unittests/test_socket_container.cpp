#include <gtest/gtest.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

extern "C" {
#include "webserver.h"
#include "reverse_proxy.h"
}

/*
 * The socket container: the list of all sockets the server holds, and
 * deleteAllSockets() on shutdown.
 *
 * The proxy cases matter most: closing a proxy client releases its backend
 * socket as well. deleteAllSockets() used to collect all sockets first and
 * then close them one by one, so the backend came up again after it had
 * already been freed. Built with ASan (see Makefile), a second release of
 * the same socket aborts the run instead of passing silently.
 */

namespace {

socket_info* make_sock() {
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        return nullptr;
    }
    /* The peer is not needed; the socket only has to be a real descriptor
     * so that closing it is a real close. */
    close(fds[1]);
    socket_info* s = WebserverMallocSocketInfo();
    s->socket = fds[0];
    s->client = 1;
    s->active = 1;
    return s;
}

/* What proxy_connect_backend() and checkReverseProxy() set up, without a
 * real backend behind it. */
reverse_proxy_connection_t* link_proxy(socket_info* client, socket_info* backend) {
    reverse_proxy_connection_t* p =
        (reverse_proxy_connection_t*)WebserverMalloc(sizeof(reverse_proxy_connection_t));
    memset(p, 0, sizeof(*p));
    p->client_sock = client;
    p->backend_sock = backend;
    p->backend_fd = backend->socket;
    client->extern_handle_data_ptr = p;
    backend->extern_handle_data_ptr = p;
    return p;
}

int sockets_in_container() {
    int count = -1;
    dumpSocketsSize(&count);
    return count;
}

}  // namespace

class SocketContainerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        static bool initialised = false;
        if (!initialised) {
            initSocketContainer();
            initialised = true;
        }
    }

    void SetUp() override {
        ASSERT_EQ(sockets_in_container(), 0);
    }

    void TearDown() override {
        deleteAllSockets();
        EXPECT_EQ(sockets_in_container(), 0);
    }
};

TEST_F(SocketContainerTest, AddAndRemoveInAnyOrder) {
    socket_info* a = make_sock();
    socket_info* b = make_sock();
    socket_info* c = make_sock();
    socket_info* d = make_sock();
    addSocketContainer(a);
    addSocketContainer(b);
    addSocketContainer(c);
    addSocketContainer(d);
    EXPECT_EQ(sockets_in_container(), 4);

    deleteSocket(b);   /* middle */
    EXPECT_EQ(sockets_in_container(), 3);
    deleteSocket(a);   /* head */
    EXPECT_EQ(sockets_in_container(), 2);
    deleteSocket(d);   /* tail */
    EXPECT_EQ(sockets_in_container(), 1);

    /* Removing twice and removing a socket that was never added are no-ops */
    deleteSocket(b);
    socket_info* never = make_sock();
    deleteSocket(never);
    EXPECT_EQ(sockets_in_container(), 1);

    /* Adding twice does not link it twice */
    addSocketContainer(c);
    EXPECT_EQ(sockets_in_container(), 1);

    /* The list is still consistent after removing head and tail: new
     * entries go in and come out again */
    addSocketContainer(a);
    addSocketContainer(d);
    EXPECT_EQ(sockets_in_container(), 3);

    for (socket_info* s : {b, never}) {
        close(s->socket);
        WebserverFreeSocketInfo(s);
    }
    /* a, c, d are released by TearDown() through deleteAllSockets() */
}

TEST_F(SocketContainerTest, DeleteAllSocketsEmptiesTheList) {
    for (int i = 0; i < 10; i++) {
        addSocketContainer(make_sock());
    }
    EXPECT_EQ(sockets_in_container(), 10);
    deleteAllSockets();
    EXPECT_EQ(sockets_in_container(), 0);
}

/* The order the server creates them in: client accepted first, backend
 * connected later. Closing the client releases the backend too. */
TEST_F(SocketContainerTest, DeleteAllSocketsWithProxyClientFirst) {
    socket_info* client = make_sock();
    socket_info* backend = make_sock();
    addSocketContainer(client);
    addSocketContainer(backend);
    link_proxy(client, backend);
    socket_info* other = make_sock();
    addSocketContainer(other);

    deleteAllSockets();
    EXPECT_EQ(sockets_in_container(), 0);
}

/* The other order: the backend comes up first and is closed through its own
 * socket. reverse_proxy_cleanup() then must not release it a second time. */
TEST_F(SocketContainerTest, DeleteAllSocketsWithProxyBackendFirst) {
    socket_info* client = make_sock();
    socket_info* backend = make_sock();
    addSocketContainer(backend);
    addSocketContainer(client);
    link_proxy(client, backend);

    deleteAllSockets();
    EXPECT_EQ(sockets_in_container(), 0);
}

/* Same, closed directly: after the backend is gone the client is a plain
 * socket again and closes normally. */
TEST_F(SocketContainerTest, ClosingProxyBackendDetachesClient) {
    socket_info* client = make_sock();
    socket_info* backend = make_sock();
    addSocketContainer(client);
    addSocketContainer(backend);
    link_proxy(client, backend);

    WebserverConnectionManagerCloseRequest(backend);
    EXPECT_EQ(sockets_in_container(), 1);
    EXPECT_EQ(client->extern_handle_data_ptr, nullptr);

    WebserverConnectionManagerCloseRequest(client);
    EXPECT_EQ(sockets_in_container(), 0);
}
