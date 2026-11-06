#include <gtest/gtest.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <thread>
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
