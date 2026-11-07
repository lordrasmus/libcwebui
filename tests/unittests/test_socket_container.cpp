#include <gtest/gtest.h>
#include <algorithm>
#include <cerrno>
#include <sys/resource.h>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

extern "C" {
#include "webserver.h"
#include "reverse_proxy.h"
#include <miniz.h>
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

/*
 * Closing a websocket while the output thread is queueing a frame for it.
 *
 * The output thread finds the socket by GUID, which leaves its mutex locked,
 * queues the frame and registers the socket for writing
 * (insert_websocket_output_chunk()). The event loop closing the same socket
 * waits for that mutex in deleteSocket(). It must not have unregistered the
 * socket's events before that point: the frame would register it again, and
 * the socket was then freed while still registered - with the select backend
 * a pointer to freed memory in its table.
 */
class WebsocketCloseTest : public SocketContainerTest {
protected:
    static void SetUpTestSuite() {
        SocketContainerTest::SetUpTestSuite();
        static bool started = false;
        if (!started) {
            /* epoll for the registrations, and the websocket threads: closing
             * a websocket queues a disconnect message for the input thread */
            initEvents();
            initWebsocketApi();
            started = true;
        }
    }
};

TEST_F(WebsocketCloseTest, CloseUnregistersOnlyAfterTheSenderIsDone) {
    static const char guid[] = "close-order-test";

    socket_info* sock = make_sock();
    sock->isWebsocket = 1;
    sock->websocket_guid = (char*)WebserverMalloc(WEBSERVER_GUID_LENGTH + 1);
    snprintf(sock->websocket_guid, WEBSERVER_GUID_LENGTH + 1, "%s", guid);
    sock->header = WebserverMallocHttpRequestHeader();
    sock->header->url = (char*)WebserverMalloc(sizeof("NoSuchSocket"));
    strcpy(sock->header->url, "NoSuchSocket");
    addSocketContainer(sock);
    addSocketByGUID(sock);
    addEventSocketRead(sock);
    ASSERT_EQ(sock->registered, 1);

    /* What the output thread holds while it queues a frame */
    ASSERT_EQ(getSocketByGUID(guid), sock);
    ASSERT_EQ(sock->closeSocket, 0);

    std::thread closer([sock] { WebserverConnectionManagerCloseRequest(sock); });
    /* Give the closer time to run into the socket mutex */
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(sock->registered, 1)
        << "events were unregistered while another thread still held the socket";

    /* The rest of what the output thread does, then let the close finish */
    activateEventSocketWrite(sock);
    PlatformUnlockMutex(&sock->socket_mutex);
    closer.join();

    /* Gone for every other thread: no frame can be queued for it any more */
    EXPECT_EQ(getSocketByGUID(guid), nullptr);
    EXPECT_EQ(sockets_in_container(), 0);
}

/*
 * Shutdown with an open websocket.
 *
 * Closing the websocket queues a disconnect message; the input thread hands
 * it to the application's handler. shutdownWebserver() used to go straight
 * on releasing the handler table, sessions, config and the memory manager
 * while that handler was still running - and the threads were never ended.
 * Now it stops and joins both threads first, so the handler runs to the end
 * against a server that is still intact.
 *
 * The handler takes its time and then reads the config, like a real one
 * looking at server state. Runs in a fresh process: it initialises and shuts
 * down the whole server.
 */
namespace {

volatile int shutdown_handler_port = 0;

void slow_disconnect_handler(const int signal, const char* guid, const char binary,
                             const char* msg, const unsigned long long len) {
    (void)guid; (void)binary; (void)msg; (void)len;
    if (signal != WEBSOCKET_DISCONNECT) {
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    shutdown_handler_port = getConfigInt("port");
}

void shutdown_with_open_websocket() {
    alarm(10);
    initWebserver();
    initSocketContainer();
    RegisterWebsocketHandler("ShutdownTest", slow_disconnect_handler, __FILE__, __LINE__);

    socket_info* sock = make_sock();
    sock->isWebsocket = 1;
    sock->websocket_guid = (char*)WebserverMalloc(WEBSERVER_GUID_LENGTH + 1);
    snprintf(sock->websocket_guid, WEBSERVER_GUID_LENGTH + 1, "%s", "shutdown-test");
    sock->header = WebserverMallocHttpRequestHeader();
    sock->header->url = (char*)WebserverMalloc(sizeof("ShutdownTest"));
    strcpy(sock->header->url, "ShutdownTest");
    addSocketContainer(sock);
    addSocketByGUID(sock);
    addEventSocketRead(sock);

    WebserverShutdown();

    /* _exit: no leak report at exit, the shutdown is not meant to be leak
     * free here, only the order matters */
    _exit(shutdown_handler_port == 80 ? 0 : 2);
}

}  // namespace

TEST(WebsocketShutdownTest, DisconnectHandlerFinishesBeforeTheServerIsTornDown) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_EXIT(shutdown_with_open_websocket(), ::testing::ExitedWithCode(0), "");
}

/*
 * Descriptors handed in with WebserverInjectExternFD() (the device daemon
 * uses it for the watchdog and a timer). Their socket_info was never
 * released, and there was no way to take one out again. The descriptor
 * itself belongs to the caller and must stay open.
 */
namespace {

void ignore_extern_event(int fd, void* ptr) {
    (void)fd; (void)ptr;
}

bool fd_is_open(int fd) {
    return fcntl(fd, F_GETFD) != -1;
}

void shutdown_with_extern_fd() {
    alarm(10);
    initWebserver();
    initSocketContainer();
    int p[2];
    if (pipe(p) != 0) {
        _exit(4);
    }
    WebserverInjectExternFD(p[0], ignore_extern_event);
    if (countExternSockets() != 1) {
        _exit(5);
    }

    WebserverShutdown();

    if (countExternSockets() != 0) {
        _exit(2);   /* still held after shutdown */
    }
    _exit(fd_is_open(p[0]) ? 0 : 3);
}

}  // namespace

/* Same set-up as the websocket tests: needs epoll for the registration */
class ExternFdTest : public WebsocketCloseTest {};

TEST_F(ExternFdTest, RemoveReleasesButDoesNotClose) {
    int a[2], b[2];
    ASSERT_EQ(pipe(a), 0);
    ASSERT_EQ(pipe(b), 0);
    int before = countExternSockets();

    WebserverInjectExternFD(a[0], ignore_extern_event);
    WebserverInjectExternFD(b[0], ignore_extern_event);
    EXPECT_EQ(countExternSockets(), before + 2);

    EXPECT_EQ(WebserverRemoveExternFD(a[0]), 0);
    EXPECT_EQ(countExternSockets(), before + 1);
    EXPECT_TRUE(fd_is_open(a[0])) << "the descriptor belongs to the caller";

    /* Removed already, and never injected */
    EXPECT_EQ(WebserverRemoveExternFD(a[0]), -1);
    EXPECT_EQ(WebserverRemoveExternFD(b[1]), -1);

    EXPECT_EQ(WebserverRemoveExternFD(b[0]), 0);
    EXPECT_EQ(countExternSockets(), before);

    for (int fd : {a[0], a[1], b[0], b[1]}) {
        close(fd);
    }
}

TEST(ExternFdShutdownTest, ShutdownReleasesButDoesNotClose) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_EXIT(shutdown_with_extern_fd(), ::testing::ExitedWithCode(0), "");
}

/*
 * getSocketInfoSize() feeds the socket overview. It left out the websocket
 * frames waiting to be sent and the response being built, and it walked the
 * output list with the list's own iterator - state inside the list, which it
 * reset for whoever else was iterating.
 */
TEST(SocketInfoSizeTest, CountsQueuedWebsocketFrames) {
    socket_info* sock = WebserverMallocSocketInfo();
    uint32_t before = getSocketInfoSize(sock);

    char frame[3000];
    memset(frame, 'x', sizeof(frame));
    sendWebsocketChunk(sock, (const unsigned char*)frame, sizeof(frame));

    /* Two chunks of 2000 bytes each hold the 3000 */
    EXPECT_GE(getSocketInfoSize(sock), before + 4000);

    WebserverFreeSocketInfo(sock);
}

TEST(SocketInfoSizeTest, LeavesTheListIteratorsAlone) {
    socket_info* sock = WebserverMallocSocketInfo();
    struct list_entry somewhere = {};
    sock->output_list.cur_iter = &somewhere;

    getSocketInfoSize(sock);

    EXPECT_EQ(sock->output_list.cur_iter, &somewhere);
    sock->output_list.cur_iter = nullptr;
    WebserverFreeSocketInfo(sock);
}

/*
 * The per-address limit on the real accept path: handleServer() on a
 * listener on 127.0.0.1, clients connecting from there.
 */
#include <netinet/in.h>
#include <arpa/inet.h>

class AcceptLimitTest : public WebsocketCloseTest {
protected:
    socket_info* listener = nullptr;
    int port = 0;
    std::vector<int> clients;
    int saved_limit = 0;

    void SetUp() override {
        WebsocketCloseTest::SetUp();
        saved_limit = connLimitGetMax();

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_GE(fd, 0);
        struct sockaddr_in a = {};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        ASSERT_EQ(bind(fd, (struct sockaddr*)&a, sizeof(a)), 0);
        ASSERT_EQ(listen(fd, 256), 0);
        socklen_t len = sizeof(a);
        getsockname(fd, (struct sockaddr*)&a, &len);
        port = ntohs(a.sin_port);
        fcntl(fd, F_SETFL, O_NONBLOCK);

        listener = WebserverMallocSocketInfo();
        listener->socket = fd;
        listener->server = 1;
    }

    void TearDown() override {
        for (int c : clients) {
            close(c);
        }
        close(listener->socket);
        WebserverFreeSocketInfo(listener);
        connLimitSetMax(saved_limit);
        WebsocketCloseTest::TearDown();     /* closes the accepted ones */
    }

    int connect_client() {
        int c = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in a = {};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = htons(port);
        EXPECT_EQ(connect(c, (struct sockaddr*)&a, sizeof(a)), 0);
        clients.push_back(c);
        return c;
    }

    /* Whether the server has closed the connection of client c */
    static bool closed_by_server(int c) {
        char b;
        struct timeval tv = { 0, 200 * 1000 };
        setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ssize_t r = recv(c, &b, 1, MSG_PEEK);
        return r == 0 || (r < 0 && errno == ECONNRESET);
    }
};

TEST_F(AcceptLimitTest, FullGroupClosesTheLongestIdle) {
    connLimitSetMax(4);
    int first = connect_client();
    int second = connect_client();
    for (int i = 0; i < 4; i++) {
        connect_client();
    }
    handleServer(listener);

    EXPECT_EQ(sockets_in_container(), 4);
    EXPECT_TRUE(closed_by_server(first));
    EXPECT_TRUE(closed_by_server(second));
    EXPECT_FALSE(closed_by_server(clients.back()));
}

TEST_F(AcceptLimitTest, RefusedWhenEveryConnectionHasInputWaiting) {
    connLimitSetMax(2);
    int a = connect_client();
    int b = connect_client();
    handleServer(listener);
    ASSERT_EQ(sockets_in_container(), 2);

    /* Both have sent part of a request that has not been read yet: they are
     * about to work, neither may be closed for a newcomer */
    ASSERT_EQ(send(a, "GET", 3, 0), 3);
    ASSERT_EQ(send(b, "GET", 3, 0), 3);
    int third = connect_client();
    handleServer(listener);

    EXPECT_EQ(sockets_in_container(), 2);
    EXPECT_FALSE(closed_by_server(a));
    EXPECT_FALSE(closed_by_server(b));
    char answer[64] = {};
    struct timeval tv = { 1, 0 };
    setsockopt(third, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ASSERT_GT(recv(third, answer, sizeof(answer) - 1, 0), 0);
    EXPECT_EQ(strncmp(answer, "HTTP/1.1 503", 12), 0) << answer;
}

TEST_F(AcceptLimitTest, LoweredLimitTrimsOnTheNextConnection) {
    connLimitSetMax(6);
    for (int i = 0; i < 6; i++) {
        connect_client();
    }
    handleServer(listener);
    ASSERT_EQ(sockets_in_container(), 6);

    connLimitSetMax(3);
    connect_client();
    handleServer(listener);
    EXPECT_EQ(sockets_in_container(), 3);
}

TEST_F(AcceptLimitTest, OneRoundTakesABoundedNumber) {
    /* Without the limit, so nothing is closed: only the per-round cap acts */
    connLimitSetMax(0);
    for (int i = 0; i < 70; i++) {
        connect_client();
    }
    handleServer(listener);
    EXPECT_EQ(sockets_in_container(), 64);
    handleServer(listener);
    EXPECT_EQ(sockets_in_container(), 70);
}

TEST_F(AcceptLimitTest, IdleOneFoundBehindManyWithInputWaiting) {
    /* The nine oldest have input waiting, only the newest is idle: it must
     * still be found, every member is looked at once */
    connLimitSetMax(10);
    for (int i = 0; i < 10; i++) {
        connect_client();
    }
    handleServer(listener);
    ASSERT_EQ(sockets_in_container(), 10);
    for (int i = 0; i < 9; i++) {
        ASSERT_EQ(send(clients[i], "GET", 3, 0), 3);
    }
    int idle = clients[9];
    int newcomer = connect_client();
    handleServer(listener);

    EXPECT_EQ(sockets_in_container(), 10);
    EXPECT_TRUE(closed_by_server(idle));
    EXPECT_FALSE(closed_by_server(newcomer));
    for (int i = 0; i < 9; i++) {
        EXPECT_FALSE(closed_by_server(clients[i])) << "client " << i;
    }
}

/*
 * Idle timeouts: the sweep run by eventLoopAfterDispatch() once per event
 * loop second. The tests move the loop time g_loop_now themselves and build
 * sockets in the states that matter.
 */
class IdleTimeoutTest : public WebsocketCloseTest {
protected:
    uint32_t base = 0;

    void SetUp() override {
        WebsocketCloseTest::SetUp();
        base = g_loop_now + 1000;   /* well clear of earlier tests */
        g_loop_now = base;
    }

    /* A client connection as handleServer() leaves it, stamps at "now" */
    socket_info* client() {
        socket_info* s = make_sock();
        s->header = WebserverMallocHttpRequestHeader();
        s->rx_at = s->tx_at = s->phase_at = g_loop_now;
        addSocketContainer(s);
        return s;
    }

    /* An established websocket: closing it queues a disconnect for the
     * input thread, which needs the GUID and the URL */
    socket_info* websocket(const char* guid) {
        socket_info* s = client();
        s->isWebsocket = 1;
        s->bytes_received = 100;
        s->websocket_guid = (char*)WebserverMalloc(WEBSERVER_GUID_LENGTH + 1);
        snprintf(s->websocket_guid, WEBSERVER_GUID_LENGTH + 1, "%s", guid);
        s->header->url = (char*)WebserverMalloc(sizeof("NoSuchSocket"));
        strcpy(s->header->url, "NoSuchSocket");
        addSocketByGUID(s);
        return s;
    }

    /* What the write path does once the client has taken everything */
    void drain(socket_info* s) {
        while (!ws_list_empty(&s->websocket_chunk_list)) {
            WebserverFreeHtml_chunk((html_chunk*)ws_list_extract_at(&s->websocket_chunk_list, 0));
            s->ws_chunks_sent++;
        }
        s->tx_at = g_loop_now;
    }

    /* Let `seconds` pass since base and run the sweep */
    void at(uint32_t seconds) {
        g_loop_now = base + seconds;
        eventLoopAfterDispatch();
    }
};

TEST_F(IdleTimeoutTest, FreshConnectionGetsTheRequestHeaderDeadline) {
    client();                               /* never sends a byte */
    at(29);
    EXPECT_EQ(sockets_in_container(), 1);
    at(30);
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, KeepAliveHasItsOwnLongerTimeout) {
    socket_info* s = client();
    s->bytes_received = 100;                /* has had a request */
    at(31);
    EXPECT_EQ(sockets_in_container(), 1);
    at(119);
    EXPECT_EQ(sockets_in_container(), 1);
    at(120);
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, NewRequestAfterAPauseStartsItsOwnDeadline) {
    socket_info* s = client();
    s->bytes_received = 100;
    at(100);                                /* long keep-alive pause */
    s->header_buffer_pos = 5;               /* next request begins */
    s->phase_at = g_loop_now;
    at(129);
    EXPECT_EQ(sockets_in_container(), 1) << "not cut off for the pause before it";
    at(130);
    EXPECT_EQ(sockets_in_container(), 0) << "trickling header: one deadline";
}

TEST_F(IdleTimeoutTest, ResponseDeadlineCountsFromQueueingNotFromTheLastOne) {
    socket_info* s = client();
    s->bytes_received = 100;
    at(110);                                /* keep-alive almost over */
    output_struct* out = (output_struct*)WebserverMalloc(sizeof(output_struct));
    memset(out, 0, sizeof(*out));
    ws_list_append(&s->output_list, out, 0);
    s->tx_at = g_loop_now;                  /* what generateOutputBuffer() does */
    at(229);
    EXPECT_EQ(sockets_in_container(), 1);
    at(230);
    EXPECT_EQ(sockets_in_container(), 0) << "client takes nothing for send_idle";
}

TEST_F(IdleTimeoutTest, UploadWithoutProgressIsClosedButProgressKeepsIt) {
    socket_info* s = client();
    s->bytes_received = 100;
    s->header->header_complete = 1;         /* buffered POST body coming */
    at(100);
    s->rx_at = g_loop_now;                  /* a chunk arrived */
    at(219);
    EXPECT_EQ(sockets_in_container(), 1);
    at(220);
    EXPECT_EQ(sockets_in_container(), 0);
}

/* A socket whose input is read in the dispatch round in between */
TEST_F(IdleTimeoutTest, InputWaitingSavesAKeepAliveConnectionOnce) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    socket_info* s = WebserverMallocSocketInfo();
    s->socket = fds[0];
    s->client = 1;
    s->header = WebserverMallocHttpRequestHeader();
    s->bytes_received = 100;
    s->rx_at = s->tx_at = s->phase_at = g_loop_now;
    addSocketContainer(s);
    ASSERT_EQ(send(fds[1], "GET", 3, 0), 3);   /* arrived, not yet read */
    at(120);
    ASSERT_EQ(sockets_in_container(), 1) << "next request is here";
    EXPECT_EQ(s->spared_at, base + 120);
    char buf[8];
    ASSERT_EQ(recv(fds[0], buf, sizeof(buf), 0), 3);   /* the loop reads it */
    s->header_buffer_pos = 3;
    s->phase_at = g_loop_now;
    at(121);
    EXPECT_EQ(sockets_in_container(), 1);
    EXPECT_EQ(s->spared_at, 0u) << "spared once, then reset";
    close(fds[1]);
}

/* Input nothing is going to read: no second exception, and no sweep that
 * keeps coming back to it */
TEST_F(IdleTimeoutTest, InputNeverReadDoesNotSaveTheConnectionTwice) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    socket_info* s = WebserverMallocSocketInfo();
    s->socket = fds[0];
    s->client = 1;
    s->header = WebserverMallocHttpRequestHeader();
    s->bytes_received = 100;
    s->rx_at = s->tx_at = s->phase_at = g_loop_now;
    addSocketContainer(s);
    ASSERT_EQ(send(fds[1], "GET", 3, 0), 3);
    at(120);
    ASSERT_EQ(sockets_in_container(), 1);
    at(121);
    EXPECT_EQ(sockets_in_container(), 1) << "at least a whole second";
    at(122);
    EXPECT_EQ(sockets_in_container(), 0);
    close(fds[1]);
}

/* A request arriving right at the deadline gets one second, not more */
TEST_F(IdleTimeoutTest, RequestHeaderDeadlineInputBuysOneSecond) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    socket_info* s = WebserverMallocSocketInfo();
    s->socket = fds[0];
    s->client = 1;
    s->header = WebserverMallocHttpRequestHeader();
    s->rx_at = s->tx_at = s->phase_at = g_loop_now;
    addSocketContainer(s);
    ASSERT_EQ(send(fds[1], "GET", 3, 0), 3);
    at(30);
    EXPECT_EQ(sockets_in_container(), 1);
    at(31);
    EXPECT_EQ(sockets_in_container(), 1);
    at(32);
    EXPECT_EQ(sockets_in_container(), 0) << "not a second longer";
    close(fds[1]);
}

/* Under load one dispatch round need not reach a spared socket: the
 * exception lasts the second, and an immediate further sweep in that
 * second neither closes it nor keeps finding it again */
TEST_F(IdleTimeoutTest, SparedSocketsLastTheSecondWithoutSpinning) {
    std::vector<int> peers;
    for (int i = 0; i < 100; i++) {
        int fds[2];
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
        socket_info* s = WebserverMallocSocketInfo();
        s->socket = fds[0];
        s->client = 1;
        s->header = WebserverMallocHttpRequestHeader();
        s->bytes_received = 100;
        s->rx_at = s->tx_at = s->phase_at = g_loop_now;
        addSocketContainer(s);
        ASSERT_EQ(send(fds[1], "GET", 3, 0), 3);
        peers.push_back(fds[1]);
    }
    at(120);
    EXPECT_EQ(sockets_in_container(), 100);
    EXPECT_EQ(eventLoopBeforeWait(), 0) << "64 per round, 36 left";
    eventLoopAfterDispatch();
    EXPECT_EQ(sockets_in_container(), 100) << "same second: spared, not closed";
    EXPECT_NE(eventLoopBeforeWait(), 0) << "nothing left to do this second";
    eventLoopAfterDispatch();
    EXPECT_EQ(sockets_in_container(), 100);
    at(121);                                /* spared just before the tick */
    EXPECT_EQ(sockets_in_container(), 100) << "a whole second, not milliseconds";
    EXPECT_NE(eventLoopBeforeWait(), 0);
    at(122);                                /* nobody read: now they go */
    eventLoopAfterDispatch();
    EXPECT_EQ(sockets_in_container(), 0);
    for (int fd : peers) {
        close(fd);
    }
}

/* More than one sweep can handle with no way to make progress on any of
 * them must not turn into a loop that never waits */
TEST_F(IdleTimeoutTest, ManyClosingWebsocketsWithQueuedFramesDoNotSpin) {
    std::vector<socket_info*> ws;
    for (int i = 0; i < 100; i++) {
        char guid[32];
        snprintf(guid, sizeof(guid), "closing-ws-%d", i);
        socket_info* s = websocket(guid);
        s->closeSocket = 1;
        sendWebsocketChunk(s, (const unsigned char*)"\x81\x01x", 3);  /* client never takes it */
        ws.push_back(s);
    }
    at(60);
    EXPECT_NE(eventLoopBeforeWait(), 0) << "nothing to do right away";
    EXPECT_EQ(sockets_in_container(), 100) << "no ping, no close yet";
    for (socket_info* s : ws) {
        EXPECT_EQ(s->ws_ping_at, 0u);
    }
    at(119);
    EXPECT_EQ(sockets_in_container(), 100);
    at(180);                                /* send_idle from first seen at 60 */
    EXPECT_EQ(sockets_in_container(), 36);
    eventLoopAfterDispatch();
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, ClosingWebsocketWithNothingQueuedUsesTheClosingDeadline) {
    socket_info* s = websocket("closing-empty");
    s->closeSocket = 1;
    at(4);
    EXPECT_EQ(sockets_in_container(), 1);
    at(5);
    EXPECT_EQ(sockets_in_container(), 0);
}

/* A client busy sending but not reading what it is sent */
TEST_F(IdleTimeoutTest, WebsocketOutputNotTakenIsClosedDespiteTraffic) {
    socket_info* s = websocket("not-reading");
    sendWebsocketChunk(s, (const unsigned char*)"\x81\x01x", 3);
    at(10);                                 /* first seen here */
    for (uint32_t t = 11; t < 130; t++) {
        s->rx_at = base + t;                /* frames keep coming in */
        at(t);
    }
    EXPECT_EQ(sockets_in_container(), 1);
    EXPECT_EQ(s->ws_ping_at, 0u) << "not silent, no ping";
    s->rx_at = base + 130;
    at(130);                                /* 120 s after it was seen at 10 */
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, WebsocketOutputBeingTakenKeepsIt) {
    socket_info* s = websocket("reading-slowly");
    at(10);
    sendWebsocketChunk(s, (const unsigned char*)"\x81\x01x", 3);
    at(11);
    s->tx_at = base + 100;                  /* some of it went out */
    at(219);
    EXPECT_EQ(sockets_in_container(), 1);
    at(220);
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, WebsocketPingsBeyondOneRoundAreSentInTheNext) {
    std::vector<socket_info*> ws;
    for (int i = 0; i < 100; i++) {
        char guid[32];
        snprintf(guid, sizeof(guid), "many-pings-%d", i);
        ws.push_back(websocket(guid));
    }
    at(60);
    EXPECT_EQ(eventLoopBeforeWait(), 0) << "36 pings still to send";
    eventLoopAfterDispatch();
    for (socket_info* s : ws) {
        EXPECT_NE(s->ws_ping_at, 0u);
    }
    EXPECT_NE(eventLoopBeforeWait(), 0);
}

TEST_F(IdleTimeoutTest, MarkedForClosingWithNothingToSend) {
    socket_info* s = client();
    s->bytes_received = 100;
    s->closeSocket = 1;
    at(4);
    EXPECT_EQ(sockets_in_container(), 1);
    at(5);
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, ListenersAndUnknownSocketsAreLeftAlone) {
    socket_info* l = make_sock();
    l->server = 1;
    addSocketContainer(l);
    socket_info* backend = make_sock();     /* no header, no handler */
    addSocketContainer(backend);
    at(10000);
    EXPECT_EQ(sockets_in_container(), 2);
}

TEST_F(IdleTimeoutTest, SilentWebsocketGetsAPingThenIsClosed) {
    socket_info* s = websocket("idle-ping-test");
    at(59);
    EXPECT_EQ(s->ws_ping_at, 0u);
    at(60);
    ASSERT_NE(s->ws_ping_at, 0u) << "ping sent";
    ASSERT_FALSE(ws_list_empty(&s->websocket_chunk_list));
    html_chunk* chunk = (html_chunk*)ws_list_get_at(&s->websocket_chunk_list, 0);
    ASSERT_EQ(chunk->length, 2u);
    EXPECT_EQ((unsigned char)chunk->text[0], 0x89);
    drain(s);                               /* the ping goes out */
    at(61);                                 /* seen gone: answer due from here */
    at(90);
    EXPECT_EQ(sockets_in_container(), 1);
    at(91);
    EXPECT_EQ(sockets_in_container(), 0) << "no answer to the ping";
}

/* The device's /CommandSocket pushes a frame every 5 s: the client's
 * kernel taking those says nothing about the application answering */
TEST_F(IdleTimeoutTest, SendingProgressDoesNotStandInForThePong) {
    socket_info* s = websocket("pushed-to");
    at(60);
    ASSERT_NE(s->ws_ping_at, 0u);
    drain(s);
    at(61);
    for (uint32_t t = 62; t < 91; t++) {
        sendWebsocketChunk(s, (const unsigned char*)"\x81\x01x", 3);
        drain(s);                           /* taken right away */
        at(t);
    }
    EXPECT_EQ(sockets_in_container(), 1);
    at(91);
    EXPECT_EQ(sockets_in_container(), 0);
}

/* Frames queued behind the ping all the time: the queue is never seen
 * empty, but the ping itself is out, and the answer is due from then */
TEST_F(IdleTimeoutTest, PongDeadlineRunsWhileNewFramesKeepTheQueueFull) {
    socket_info* s = websocket("busy-queue");
    at(60);
    ASSERT_NE(s->ws_ping_at, 0u);
    for (uint32_t t = 61; t < 91; t++) {
        drain(s);                           /* ping and all before it out */
        sendWebsocketChunk(s, (const unsigned char*)"\x81\x01x", 3);
        at(t);
    }
    EXPECT_EQ(sockets_in_container(), 1);
    drain(s);
    sendWebsocketChunk(s, (const unsigned char*)"\x81\x01x", 3);
    at(91);
    EXPECT_EQ(sockets_in_container(), 0) << "no answer 30 s after the ping went out";
}

/* The ping shares its chunk with frames appended after it: written past
 * the ping is out, even though the chunk is not done yet */
TEST_F(IdleTimeoutTest, PingOutWhenWrittenPastItInsideItsChunk) {
    socket_info* s = websocket("shared-chunk");
    static const unsigned char before[100] = { 0x81 };
    sendWebsocketChunk(s, before, sizeof(before));
    at(60);                                 /* ping appended to that chunk */
    ASSERT_NE(s->ws_ping_at, 0u);
    ASSERT_EQ(ws_list_size(&s->websocket_chunk_list), 1u);
    static const unsigned char after[500] = { 0x81 };
    sendWebsocketChunk(s, after, sizeof(after));   /* same chunk again */
    ASSERT_EQ(ws_list_size(&s->websocket_chunk_list), 1u);
    s->websocket_send_pos = 101;            /* one ping byte still to go */
    s->tx_at = base + 60;
    at(61);
    EXPECT_EQ(s->ws_ping_sent_at, 0u) << "ping not complete yet";
    s->websocket_send_pos = 102;            /* ping written, the rest not */
    at(62);
    EXPECT_EQ(s->ws_ping_sent_at, base + 62);
    at(91);
    EXPECT_EQ(sockets_in_container(), 1);
    at(92);
    EXPECT_EQ(sockets_in_container(), 0);
}

/* Behind frames the client does not take, the ping never leaves: that is
 * the send_idle deadline's case, not the pong's */
TEST_F(IdleTimeoutTest, PingStuckBehindFramesFallsUnderSendIdle) {
    socket_info* s = websocket("ping-stuck");
    at(60);                                 /* ping queued, never taken */
    ASSERT_NE(s->ws_ping_at, 0u);
    at(61);                                 /* output first seen waiting */
    at(91);
    EXPECT_EQ(sockets_in_container(), 1) << "no pong deadline before the ping is out";
    at(180);
    EXPECT_EQ(sockets_in_container(), 1);
    at(181);
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, AnsweredPingKeepsTheWebsocket) {
    socket_info* s = websocket("idle-pong-test");
    at(60);
    ASSERT_NE(s->ws_ping_at, 0u);
    at(65);
    s->rx_at = g_loop_now;                  /* pong (or any frame) arrived */
    at(66);
    EXPECT_EQ(s->ws_ping_at, 0u);
    at(100);
    EXPECT_EQ(sockets_in_container(), 1);
}

TEST_F(IdleTimeoutTest, ProxyWaitsLongForTheBackendButNotForever) {
    socket_info* s = client();
    reverse_proxy_connection_t* p =
        (reverse_proxy_connection_t*)WebserverMalloc(sizeof(reverse_proxy_connection_t));
    memset(p, 0, sizeof(*p));
    p->client_sock = s;
    p->backend_fd = -1;
    p->state = PROXY_STATE_RECV_BACKEND_HEADER;
    s->extern_handle = reverse_proxy_client_handler;
    s->extern_handle_data_ptr = p;
    at(0);                                  /* sweep notices the state */
    at(299);
    EXPECT_EQ(sockets_in_container(), 1) << "long polling";
    at(300);
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, ProxyStreamKeptAliveByBackendProgress) {
    socket_info* s = client();
    reverse_proxy_connection_t* p =
        (reverse_proxy_connection_t*)WebserverMalloc(sizeof(reverse_proxy_connection_t));
    memset(p, 0, sizeof(*p));
    p->client_sock = s;
    p->backend_fd = -1;
    p->state = PROXY_STATE_STREAMING;
    p->backend_at = g_loop_now;
    s->extern_handle = reverse_proxy_client_handler;
    s->extern_handle_data_ptr = p;
    at(100);
    p->backend_at = g_loop_now;             /* backend delivers */
    at(219);
    EXPECT_EQ(sockets_in_container(), 1);
    at(220);
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, ManyExpiredAreClosedOverSeveralRounds) {
    for (int i = 0; i < 100; i++) {
        client();
    }
    at(30);
    EXPECT_EQ(sockets_in_container(), 36) << "at most 64 per round";
    EXPECT_EQ(eventLoopBeforeWait(), 0) << "next round right away";
    eventLoopAfterDispatch();               /* same second, pending rest */
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(IdleTimeoutTest, SwitchedOffTimeoutNeverExpires) {
    connLimitSetConfig("timeout_request_header", 0);
    client();
    at(100000);
    EXPECT_EQ(sockets_in_container(), 1);
    connLimitSetConfig("timeout_request_header", 30);
}

/* Time spent blocked in a handler does not count against the others */
TEST(EventLoopClockTest, BlockedTimeIsTakenOut) {
    if (connMonotonicSec() == 0) {
        GTEST_SKIP() << "no clock on this platform";
    }
    eventLoopAfterWait();
    uint32_t before = g_loop_now;
    sleep(3);                               /* a handler blocking for 3 s */
    eventLoopBeforeWait();
    eventLoopAfterWait();
    EXPECT_LE(g_loop_now - before, 1u) << "only one of the three seconds counts";
}

/* Total limit on the accept path */
TEST_F(AcceptLimitTest, TotalLimitClosesTheLongestIdleOfAnyAddress) {
    connLimitSetMax(0);                     /* per address off: only the total */
    connLimitSetConfig("max_connections_total", 4);
    int first = connect_client();
    for (int i = 0; i < 3; i++) {
        connect_client();
    }
    handleServer(listener);
    ASSERT_EQ(sockets_in_container(), 4);
    int newcomer = connect_client();
    handleServer(listener);
    EXPECT_EQ(sockets_in_container(), 4);
    EXPECT_TRUE(closed_by_server(first));
    EXPECT_FALSE(closed_by_server(newcomer));
    connLimitSetConfig("max_connections_total", 0);
}

TEST_F(AcceptLimitTest, TotalLimitRefusesWhenAllAreBusy) {
    connLimitSetMax(0);
    connLimitSetConfig("max_connections_total", 2);
    int a = connect_client();
    int b = connect_client();
    handleServer(listener);
    ASSERT_EQ(send(a, "GET", 3, 0), 3);     /* both have input waiting */
    ASSERT_EQ(send(b, "GET", 3, 0), 3);
    int third = connect_client();
    handleServer(listener);
    EXPECT_EQ(sockets_in_container(), 2);
    char answer[64] = {};
    struct timeval tv = { 1, 0 };
    setsockopt(third, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ASSERT_GT(recv(third, answer, sizeof(answer) - 1, 0), 0);
    EXPECT_EQ(strncmp(answer, "HTTP/1.1 503", 12), 0) << answer;
    connLimitSetConfig("max_connections_total", 0);
}

TEST_F(AcceptLimitTest, LoweredTotalLimitComesDownInsteadOfStaying) {
    connLimitSetMax(0);
    connLimitSetConfig("max_connections_total", 0);
    for (int i = 0; i < 20; i++) {
        connect_client();
    }
    handleServer(listener);
    ASSERT_EQ(sockets_in_container(), 20);
    connLimitSetConfig("max_connections_total", 4);
    connect_client();
    handleServer(listener);
    EXPECT_EQ(sockets_in_container(), 20 - 8 + 1) << "eight per call, the new one in";
    connect_client();
    handleServer(listener);
    EXPECT_EQ(sockets_in_container(), 13 - 8 + 1);
    connect_client();
    handleServer(listener);
    EXPECT_EQ(sockets_in_container(), 4) << "down to the limit";
    connLimitSetConfig("max_connections_total", 0);
}

/* Lowered limit, almost all busy: closing the one idle connection must
 * not just make room for the next idle one, the total has to come down */
TEST_F(AcceptLimitTest, LoweredTotalLimitWithBusyConnectionsStillComesDown) {
    connLimitSetMax(0);
    connLimitSetConfig("max_connections_total", 0);
    std::vector<int> busy;
    int idle = connect_client();
    for (int i = 0; i < 9; i++) {
        busy.push_back(connect_client());
    }
    handleServer(listener);
    ASSERT_EQ(sockets_in_container(), 10);
    for (int fd : busy) {
        ASSERT_EQ(send(fd, "GET", 3, 0), 3);   /* input waiting: not evictable */
    }
    connLimitSetConfig("max_connections_total", 4);
    int newcomer = connect_client();
    handleServer(listener);
    EXPECT_TRUE(closed_by_server(idle));
    EXPECT_EQ(sockets_in_container(), 9) << "the newcomer would have kept it at 10";
    char answer[64] = {};
    struct timeval tv = { 1, 0 };
    setsockopt(newcomer, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ASSERT_GT(recv(newcomer, answer, sizeof(answer) - 1, 0), 0);
    EXPECT_EQ(strncmp(answer, "HTTP/1.1 503", 12), 0) << answer;
    connLimitSetConfig("max_connections_total", 0);
}

TEST(ConnLimitTotalTest, AutomaticLimitLeavesTheReserve) {
    struct rlimit rl;
    ASSERT_EQ(getrlimit(RLIMIT_NOFILE, &rl), 0);
    if (rl.rlim_cur == RLIM_INFINITY || rl.rlim_cur > (1 << 20)) {
        GTEST_SKIP() << "no usable descriptor limit";
    }
    connLimitInit();
    int fds = (int)rl.rlim_cur;
    connLimitSetConfig("max_connections_total", 0);
    connLimitSetConfig("max_connections_fd_reserve", 128);
    EXPECT_EQ(connLimitTotalMax(), fds - std::min(128, fds / 2));
    connLimitSetConfig("max_connections_fd_reserve", fds);
    EXPECT_EQ(connLimitTotalMax(), fds - fds / 2) << "at most half for the reserve";
    connLimitSetConfig("max_connections_total", 7);
    EXPECT_EQ(connLimitTotalMax(), 7) << "configured wins";
    connLimitSetConfig("max_connections_total", 0);
    connLimitSetConfig("max_connections_fd_reserve", 128);
}

#ifdef WEBSERVER_USE_SSL
/*
 * First read event on the TLS port, before the SSL object exists. The unit
 * tests have no SSL_CTX, so building the object always fails here - which
 * makes "was it built" visible: the connection would be closed.
 */
class TlsFirstReadTest : public WebsocketCloseTest {
protected:
    int peer = -1;

    socket_info* tls_client() {
        int fds[2];
        EXPECT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
        fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
        peer = fds[1];
        socket_info* s = WebserverMallocSocketInfo();
        s->socket = fds[0];
        s->client = 1;
        s->use_ssl = 1;
        s->run_ssl_accept = 1;
        s->header = WebserverMallocHttpRequestHeader();
        s->rx_at = s->tx_at = s->phase_at = g_loop_now;
        addSocketContainer(s);
        return s;
    }

    void TearDown() override {
        if (peer >= 0) {
            close(peer);
        }
        WebsocketCloseTest::TearDown();
    }
};

TEST_F(TlsFirstReadTest, ClosedBeforeTheClientHelloBuildsNoSslObject) {
    socket_info* s = tls_client();
    close(peer);
    peer = -1;
    handleer(s->socket, EVENT_READ, s);
    EXPECT_EQ(sockets_in_container(), 0);
}

TEST_F(TlsFirstReadTest, WokenWithoutDataWaitsWithoutSslObject) {
    socket_info* s = tls_client();
    uint64_t io_before = s->last_io;
    handleer(s->socket, EVENT_READ, s);
    ASSERT_EQ(sockets_in_container(), 1) << "an SSL object was attempted";
    EXPECT_EQ(s->ssl_context, nullptr);
    EXPECT_EQ(s->last_io, io_before) << "nothing arrived, no activity";
}
#endif

#ifdef WEBSERVER_USE_WEBSOCKETS
/*
 * Frames through the real receive path: bytes written to the peer of a
 * socketpair, read by recFrame().
 */
class WebsocketFrameTest : public WebsocketCloseTest {
protected:
    int peer = -1;
    socket_info* ws = nullptr;

    void SetUp() override {
        WebsocketCloseTest::SetUp();
        int fds[2];
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
        fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
        peer = fds[1];
        ws = WebserverMallocSocketInfo();
        ws->socket = fds[0];
        ws->client = 1;
        ws->isWebsocket = 1;
        ws->header = WebserverMallocHttpRequestHeader();
        ws->header->SecWebSocketVersion = 13;
        ws->header->url = (char*)WebserverMalloc(sizeof("NoSuchSocket"));
        strcpy(ws->header->url, "NoSuchSocket");
        ws->websocket_guid = (char*)WebserverMalloc(WEBSERVER_GUID_LENGTH + 1);
        snprintf(ws->websocket_guid, WEBSERVER_GUID_LENGTH + 1, "%s", "frame-test");
        ws->websocket_buffer = (unsigned char*)WebserverMalloc(WEBSOCKET_INIT_INBUFFER_SIZE);
        addSocketContainer(ws);
        addSocketByGUID(ws);
    }

    void TearDown() override {
        if (peer >= 0) {
            close(peer);
        }
        WebsocketCloseTest::TearDown();
    }

    /* A masked frame header with a 64 bit length field */
    void send_header64(unsigned char first, uint64_t length) {
        unsigned char h[14] = { first, 0x80 | 127 };
        for (int i = 0; i < 8; i++) {
            h[2 + i] = (unsigned char)(length >> (56 - 8 * i));
        }
        /* mask 00 00 00 00: the payload goes through unchanged */
        ASSERT_EQ(send(peer, h, sizeof(h), 0), (ssize_t)sizeof(h));
    }

    /* A masked frame with a short payload (mask 00 00 00 00) */
    static void add_frame(std::vector<unsigned char>& out, unsigned char first,
                          const char* payload, size_t len) {
        unsigned char h[6] = { first, (unsigned char)(0x80 | len), 0, 0, 0, 0 };
        out.insert(out.end(), h, h + sizeof(h));
        out.insert(out.end(), payload, payload + len);
    }

    /* One send for all of them: many small sends fill a unix socket's
     * buffer with per-message overhead long before the bytes add up */
    void send_frames(const std::vector<unsigned char>& frames) {
        ASSERT_EQ(send(peer, frames.data(), frames.size(), 0), (ssize_t)frames.size());
    }

    /* Read until the socket has nothing more or is closed */
    int receive_all() {
        int ret;
        do {
            ret = recFrame(ws);
        } while (ret > 0);
        return ret;
    }
};

/* Empty continuation frames used to cost an allocation and a list node
 * each without counting against any limit */
TEST_F(WebsocketFrameTest, EndlessEmptyContinuationFramesAreStopped) {
    std::vector<unsigned char> frames;
    add_frame(frames, 0x01, "a", 1);                /* text, not final */
    for (int i = 0; i < WEBSOCKET_MAX_FRAGMENTS + 100; i++) {
        add_frame(frames, 0x00, "", 0);             /* continuation, not final */
    }
    send_frames(frames);
    receive_all();
    EXPECT_EQ(ws->closeSocket, 1) << "more frames than a message may have";
    EXPECT_LE(ws_list_size(&ws->websocket_fragments), 1u) << "empty frames are not stored";
}

TEST_F(WebsocketFrameTest, FragmentedMessageWithAnEmptyFrameStillArrives) {
    std::vector<unsigned char> frames;
    add_frame(frames, 0x01, "he", 2);
    add_frame(frames, 0x00, "", 0);
    add_frame(frames, 0x80, "llo", 3);              /* continuation, final */
    send_frames(frames);
    receive_all();
    EXPECT_EQ(ws->closeSocket, 0);
    EXPECT_EQ(ws->websocket_fragment_start_opcode, 0u) << "message complete";
    EXPECT_EQ(ws->websocket_fragment_count, 0u);
    EXPECT_EQ(ws_list_size(&ws->websocket_fragments), 0u);
}

/* 14 bytes announcing almost 4 GB: the length check used to wrap around */
TEST_F(WebsocketFrameTest, HugeAnnouncedBinaryFrameIsRefusedNotAllocated) {
    send_header64(0x82, 0xFFFFFFFFu);
    EXPECT_EQ(recFrame(ws), -1);
    EXPECT_EQ(ws->closeSocket, 1);
}

TEST_F(WebsocketFrameTest, BinaryFrameAboveTheInputLimitIsRefused) {
    send_header64(0x82, WEBSOCKET_MAX_INBUFFER_SIZE + 1);
    EXPECT_EQ(recFrame(ws), -1);
    EXPECT_EQ(ws->closeSocket, 1);
}

TEST_F(WebsocketFrameTest, SmallBinaryFrameStillGoesThrough) {
    unsigned char frame[2 + 4 + 3] = { 0x82, 0x80 | 3, 0, 0, 0, 0, 'a', 'b', 'c' };
    ASSERT_EQ(send(peer, frame, sizeof(frame), 0), (ssize_t)sizeof(frame));
    EXPECT_GE(recFrame(ws), 0);
    EXPECT_EQ(ws->closeSocket, 0);
}
#endif

/*
 * Request headers through the real read path. Each line is limited, the
 * header as a whole was not: Cookie lines kept adding to a list until the
 * header ended, which a client never has to do.
 */
class RequestHeaderSizeTest : public WebsocketCloseTest {
protected:
    int peer = -1;
    socket_info* s = nullptr;

    void SetUp() override {
        WebsocketCloseTest::SetUp();
        int fds[2];
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
        fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
        peer = fds[1];
        s = WebserverMallocSocketInfo();
        s->socket = fds[0];
        s->client = 1;
        s->header = WebserverMallocHttpRequestHeader();
        s->rx_at = s->tx_at = s->phase_at = g_loop_now;
        addSocketContainer(s);
    }

    void TearDown() override {
        if (peer >= 0) {
            close(peer);
        }
        WebsocketCloseTest::TearDown();
    }

    /* Everything the server answered so far */
    std::string answer() {
        if (sockets_in_container() > 0) {
            handleer(s->socket, EVENT_WRITE, s);
        }
        std::string a;
        char buf[4096];
        struct timeval tv = { 0, 200000 };
        setsockopt(peer, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ssize_t n;
        while ((n = recv(peer, buf, sizeof(buf), 0)) > 0) {
            a.append(buf, (size_t)n);
        }
        return a;
    }
};

TEST_F(RequestHeaderSizeTest, EndlessHeaderLinesGet431) {
    std::string req = "GET / HTTP/1.1\r\nHost: x\r\n";
    for (int i = 0; i < 2000; i++) {
        req += "Cookie: a=b\r\n";                   /* no end of header */
    }
    ASSERT_EQ(send(peer, req.data(), req.size(), 0), (ssize_t)req.size());
    handleer(s->socket, EVENT_READ, s);
    if (sockets_in_container() > 0) {
        EXPECT_LE(ws_list_size(&s->header->cookie_list),
                  (unsigned)(WEBSERVER_MAX_HEADER_SIZE / 13 + 1))
            << "cookies only up to the limit";
    }
    std::string a = answer();
    EXPECT_EQ(a.compare(0, 12, "HTTP/1.1 431"), 0) << a.substr(0, 60);
}

/* Line by line through the parser, as handleClientHeaderData() does it -
 * serving the request would need the whole server set up */
static int parse_all(socket_info* s, std::string req) {
    unsigned int off = 0;
    unsigned int parsed = 0;
    int r;
    do {
        r = ParseHeader(s, s->header, &req[off], (unsigned int)(req.size() - off), &parsed);
        off += parsed;
    } while (r > 0);
    return r;
}

TEST_F(RequestHeaderSizeTest, LargeHeaderBelowTheLimitIsParsed) {
    std::string req = "GET /x HTTP/1.1\r\nHost: x\r\n";
    for (int i = 0; i < 15; i++) {
        req += "X-Filler: " + std::string(1000, 'f') + "\r\n";   /* ~15 KB */
    }
    req += "\r\n";
    ASSERT_LT(req.size(), (size_t)WEBSERVER_MAX_HEADER_SIZE);
    EXPECT_EQ(parse_all(s, req), -2) << "header complete";
    EXPECT_EQ(s->header->header_complete, 1);
}

TEST_F(RequestHeaderSizeTest, HeaderJustAboveTheLimitIsRefused) {
    std::string req = "GET /x HTTP/1.1\r\nHost: x\r\n";
    while (req.size() < (size_t)WEBSERVER_MAX_HEADER_SIZE) {
        req += "X-Filler: " + std::string(1000, 'f') + "\r\n";
    }
    req += "\r\n";
    EXPECT_EQ(parse_all(s, req), -8);
    EXPECT_EQ(s->header->header_complete, 0);
}

/*
 * Compressed responses (Accept-Encoding: deflate on a template page). The
 * body buffer used to be the size of the page, while deflate output can be
 * larger: data that does not compress grows a little, and every chunk is
 * flushed on its own.
 */
static output_struct* deflate_page(socket_info* s, const std::vector<unsigned char>& page) {
    s->header = WebserverMallocHttpRequestHeader();
    s->use_output_compression = 1;
    printHeaderChunk(s, "HTTP/1.1 200 OK\r\n");
    sendHTMLChunk(s, (const char*)page.data(), (unsigned int)page.size());
    generateOutputBuffer(s);
    return (output_struct*)ws_list_get_at(&s->output_list, 0);
}

static std::vector<unsigned char> inflate_body(const output_struct* out) {
    size_t len = 0;
    void* p = tinfl_decompress_mem_to_heap(out->body.buffer, out->body.buffer_size, &len, 0);
    std::vector<unsigned char> v;
    if (p != NULL) {
        v.assign((unsigned char*)p, (unsigned char*)p + len);
        mz_free(p);
    }
    return v;
}

TEST(DeflateOutputTest, PageThatDoesNotCompressFitsAndInflatesBack) {
    std::vector<unsigned char> page(20000);
    unsigned int seed = 12345;
    for (auto& b : page) {
        seed = seed * 1103515245u + 12345u;
        b = (unsigned char)(seed >> 16);
    }
    socket_info* s = make_sock();
    output_struct* out = deflate_page(s, page);
    ASSERT_NE(out, nullptr);
    std::string header(out->header.buffer, out->header.buffer_size);
    EXPECT_NE(header.find("Content-Encoding: deflate"), std::string::npos) << header;
    EXPECT_NE(header.find("Content-Length: " + std::to_string(out->body.buffer_size)),
              std::string::npos) << header;
    EXPECT_GT(out->body.buffer_size, page.size()) << "random data does not shrink";
    EXPECT_EQ(inflate_body(out), page);
    WebserverFreeSocketInfo(s);
}

TEST(DeflateOutputTest, OrdinaryPageStillShrinks) {
    std::string html;
    while (html.size() < 20000) {
        html += "<tr><td class=\"port\">Port</td><td class=\"state\">up</td></tr>\n";
    }
    std::vector<unsigned char> page(html.begin(), html.end());
    socket_info* s = make_sock();
    output_struct* out = deflate_page(s, page);
    ASSERT_NE(out, nullptr);
    EXPECT_LT(out->body.buffer_size, page.size() / 4);
    EXPECT_EQ(inflate_body(out), page);
    WebserverFreeSocketInfo(s);
}
