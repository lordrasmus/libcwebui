#include <gtest/gtest.h>
#include <cstring>
#include <string>

extern "C" {
#include "webserver.h"
}

/*
 * Access log: the status a response went out with, the user set by the
 * application, and the entry handed to the handler.
 */

static int calls;
static std::string got_method, got_url, got_user, got_ip;
static int got_status;
static uint64_t got_bytes;

static void capture(const ws_access_log_entry* e) {
    calls++;
    got_method = e->method ? e->method : "<null>";
    got_url = e->url ? e->url : "<null>";
    got_user = e->user ? e->user : "<null>";
    got_ip = e->client_ip ? e->client_ip : "<null>";
    got_status = e->status;
    got_bytes = e->bytes;
}

static char* dup(const char* s) {
    char* d = (char*)WebserverMalloc(strlen(s) + 1);
    strcpy(d, s);
    return d;
}

class AccessLogTest : public ::testing::Test {
protected:
    socket_info *sock;
    http_request req;

    void SetUp() override {
        /* sendHeader() reads the config. The store is global and shared with
           other tests: create it once and never free it, as test_post_streaming
           does - a freeConfig() here leaves later tests on a dead store. */
        static bool config_ready = false;
        if (!config_ready) {
            initConfig();
            config_ready = true;
        }
        sock = WebserverMallocSocketInfo();
        sock->header = WebserverMallocHttpRequestHeader();
        sock->header->isHttp1_1 = 1;
        strcpy(sock->client_ip_str, "192.0.2.7");

        memset((dummy_handler*)&req, 0, sizeof(req));
        req.socket = sock;
        req.header = sock->header;
        ws_list_init(&req.custom_response_headers);

        calls = 0;
        ws_set_access_log_handler(capture);
    }

    void TearDown() override {
        ws_set_access_log_handler(NULL);
        ws_list_destroy(&req.custom_response_headers);
        WebserverFreeSocketInfo(sock);
    }

    std::string statusLine() {
        WebserverFileInfo info;
        memset(&info, 0, sizeof(info));
        info.FileType = FILE_TYPE_PLAIN;
        info.TemplateFile = 1;
        sendHeader(&req, &info, 0);
        generateOutputBuffer(sock);
        output_struct* out = (output_struct*)ws_list_get_at(&sock->output_list, 0);
        std::string all(out->body.buffer, out->body.buffer_size);
        return all.substr(0, all.find("\r\n"));
    }
};

TEST_F(AccessLogTest, SendHeaderDefaultsTo200) {
    EXPECT_EQ(statusLine(), "HTTP/1.1 200 OK");
    EXPECT_EQ(sock->header->response_status, 200);
}

TEST_F(AccessLogTest, SendHeaderUsesStatusFromHandler) {
    ws_set_response_status((dummy_handler*)&req, 404);
    EXPECT_EQ(statusLine(), "HTTP/1.1 404 Not Found");
}

TEST_F(AccessLogTest, SendHeaderIgnoresInvalidStatus) {
    ws_set_response_status((dummy_handler*)&req, 42);
    EXPECT_EQ(statusLine(), "HTTP/1.1 200 OK");
}

TEST_F(AccessLogTest, ResponseBytesCoverHeaderAndBody) {
    statusLine();
    output_struct* out = (output_struct*)ws_list_get_at(&sock->output_list, 0);
    EXPECT_EQ(sock->header->response_bytes, out->body.buffer_size);
    EXPECT_GT(sock->header->response_bytes, 0u);
}

TEST_F(AccessLogTest, ErrorPagesRecordTheirStatus) {
    sendHeaderError(sock, (char*)"403 Forbidden", 0);
    EXPECT_EQ(sock->header->response_status, 403);
    sendHttpErrorPage(sock, 405, 0);
    EXPECT_EQ(sock->header->response_status, 405);
}

TEST_F(AccessLogTest, EntryCarriesTheRequest) {
    sock->header->method = HTTP_POST;
    sock->header->url = dup("api/server");
    sock->header->response_status = 201;
    sock->header->response_bytes = 42;
    ws_set_access_log_user((dummy_handler*)&req, "admin");

    accessLogRequest(sock);

    ASSERT_EQ(calls, 1);
    EXPECT_EQ(got_method, "POST");
    EXPECT_EQ(got_url, "/api/server");
    EXPECT_EQ(got_user, "admin");
    EXPECT_EQ(got_ip, "192.0.2.7");
    EXPECT_EQ(got_status, 201);
    EXPECT_EQ(got_bytes, 42u);
}

TEST_F(AccessLogTest, RejectedMethodKeepsItsName) {
    strcpy(sock->header->error_method, "PROPFIND");
    accessLogRequest(sock);
    ASSERT_EQ(calls, 1);
    EXPECT_EQ(got_method, "PROPFIND");
    EXPECT_EQ(got_url, "<null>");
}

TEST_F(AccessLogTest, NoHandlerNoCall) {
    ws_set_access_log_handler(NULL);
    accessLogRequest(sock);
    EXPECT_EQ(calls, 0);
}

TEST_F(AccessLogTest, ResetClearsPerRequestFields) {
    sock->header->response_status = 500;
    sock->header->response_bytes = 7;
    sock->header->request_start_ms = 1;
    strcpy(sock->header->error_method, "PUT");
    ws_set_access_log_user((dummy_handler*)&req, "admin");

    WebserverResetHttpRequestHeader(sock->header);

    EXPECT_EQ(sock->header->response_status, 0);
    EXPECT_EQ(sock->header->response_bytes, 0u);
    EXPECT_EQ(sock->header->request_start_ms, 0u);
    EXPECT_EQ(sock->header->error_method[0], '\0');
    EXPECT_EQ(sock->header->access_log_user, nullptr);
}
