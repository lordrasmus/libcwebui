#include <gtest/gtest.h>
#include <cstring>
#include <unistd.h>
#include <string>

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

/*
 * Multipart POST: a part header line without its CRLF.
 *
 * analyseFormDataLine() matched "Content-Disposition: " / "Content-Type: "
 * and searched the rest of the buffer for the CRLF ending the line. When
 * there was none - a body cut short, or a NUL byte in front of the CRLF,
 * since the search went by strlen - it did not move on and matched the same
 * position again, forever. One request was enough to take the event loop
 * down.
 *
 * Each call runs in a child with an alarm, so a regression fails the test
 * instead of hanging the suite.
 */
static void expectReturns(socket_info *sock, const char *data, unsigned int length) {
    EXPECT_EXIT({
        alarm(2);
        char *body = (char*)malloc(length + 1);
        memcpy(body, data, length);
        body[length] = '\0';
        HttpRequestHeader *part = WebserverMallocHttpRequestHeader();
        int off = analyseFormDataLine(sock, body, length, part);
        exit((off >= 0 && (unsigned int)off <= length) ? 0 : 1);
    }, ::testing::ExitedWithCode(0), "");
}

TEST_F(PostMultipartTest, ContentDispositionWithoutCrlfReturns) {
    const char body[] = "Content-Disposition: form-data; name=\"x\"";
    expectReturns(sock, body, sizeof(body) - 1);
}

TEST_F(PostMultipartTest, ContentTypeWithoutCrlfReturns) {
    const char body[] = "Content-Disposition: form-data; name=\"x\"\r\n"
                        "Content-Type: text/plain";
    expectReturns(sock, body, sizeof(body) - 1);
}

TEST_F(PostMultipartTest, NulBeforeCrlfReturns) {
    const char body[] = "Content-Type: text\0plain\r\n\r\nhello\r\n";
    expectReturns(sock, body, sizeof(body) - 1);
}

TEST_F(PostMultipartTest, EmptyValueReturns) {
    const char body[] = "Content-Disposition: ";
    expectReturns(sock, body, sizeof(body) - 1);
}

/*
 * Multipart POST: the filename of an upload.
 *
 * getFormDataFilename() skips '="' behind "filename=" and drops the closing
 * quote. With nothing or just a quote behind "filename=", the length came
 * out negative in uint32_t - about 4 GB - and the allocation and strncpy
 * that followed went with it.
 */
static std::string filenameOf(const char *content_disposition) {
    char *name = getFormDataFilename(content_disposition);
    if (name == 0) {
        return "<none>";
    }
    std::string ret(name);
    WebserverFree(name);
    return ret;
}

TEST_F(PostMultipartTest, FilenameRegular) {
    EXPECT_EQ(filenameOf("form-data; name=\"x\"; filename=\"a.txt\""), "a.txt");
}

TEST_F(PostMultipartTest, FilenameMissing) {
    EXPECT_EQ(filenameOf("form-data; name=\"x\""), "<none>");
    EXPECT_EQ(filenameOf(0), "<none>");
}

TEST_F(PostMultipartTest, FilenameEmptyQuoted) {
    EXPECT_EQ(filenameOf("form-data; filename=\"\""), "");
}

TEST_F(PostMultipartTest, FilenameWithoutValue) {
    EXPECT_EQ(filenameOf("form-data; filename="), "");
}

TEST_F(PostMultipartTest, FilenameOnlyOpeningQuote) {
    EXPECT_EQ(filenameOf("form-data; filename=\""), "");
}
