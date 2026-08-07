#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "webserver.h"
}

/*
 * Multipart POST: a section with no delimiter before the next boundary.
 *
 * parsePostData() (server.c) derives the length of a section from
 * offset3 - offset2 and subtracts 2, because a CRLF has to precede the
 * boundary:
 *
 *     data = WebserverMalloc( ( offset3 - offset2 ) - 1 );
 *     memcpy( data, data_start , ( offset3 - offset2 ) - 2);
 *
 * Both values are uint64_t. If the section comes out empty - the boundary
 * follows immediately after the CRLFCRLF of the part header - the difference
 * is 0, and 0 - 1 becomes the maximum value rather than -1.
 *
 * Whether that state is reachable depends on what analyseFormDataLine()
 * returns as offset2: it looks for the CRLFCRLF and returns the position
 * behind it. With the boundary sitting right there, the inner loop in
 * parsePostData matches on its first iteration and the difference is zero.
 *
 * These tests pin down that intermediate value. parsePostData() itself is
 * static and not reachable from here, but the arithmetic on top of it is
 * unambiguous once the value is known - see also test_memory_overflow.cpp
 * for the second half of the chain.
 */
class PostMultipartTest : public ::testing::Test {
protected:
    HttpRequestHeader *header;
    socket_info *sock;

    void SetUp() override {
        header = WebserverMallocHttpRequestHeader();
        sock = (socket_info*)calloc(1, sizeof(socket_info));
        sock->header = header;
    }

    void TearDown() override {
        WebserverFreeHttpRequestHeader(header);
        free(sock);
    }
};

TEST_F(PostMultipartTest, FormDataLineFindsEndOfPartHeader) {
    // Regular case: payload follows the part header
    char body[] = "Content-Disposition: form-data; name=\"x\"; filename=\"a\"\r\n"
                  "\r\n"
                  "hello\r\n"
                  "--bound--\r\n";
    HttpRequestHeader *part = WebserverMallocHttpRequestHeader();

    int off = analyseFormDataLine(sock, body, strlen(body), part);

    // The payload has to start right behind the CRLFCRLF
    ASSERT_GT(off, 0);
    EXPECT_STREQ(&body[off], "hello\r\n--bound--\r\n");

    WebserverFreeHttpRequestHeader(part);
}

TEST_F(PostMultipartTest, EmptyPartLeavesZeroLengthSection) {
    // Same shape, but without payload and without the CRLF before the
    // boundary. parsePostData computes offset3 - offset2 == 0 here and uses
    // (0 - 1) as the allocation size - on uint64_t the maximum value,
    // followed by a memcpy of the same length.
    //
    // This test records that exactly that state arises: the boundary sits
    // right at the position analyseFormDataLine returns.
    char body[] = "Content-Disposition: form-data; name=\"x\"; filename=\"a\"\r\n"
                  "\r\n"
                  "--bound--\r\n";
    HttpRequestHeader *part = WebserverMallocHttpRequestHeader();

    int off = analyseFormDataLine(sock, body, strlen(body), part);

    ASSERT_GT(off, 0);
    EXPECT_EQ(strncmp(&body[off], "--bound", 7), 0)
        << "The boundary follows directly - the section length is 0, and "
           "parsePostData subtracts 1 and 2 from it.";

    WebserverFreeHttpRequestHeader(part);
}
