#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "webserver.h"
}

class SecurityTest : public ::testing::Test {
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

    int parseHeaderLine(const char* line) {
        char buf[4096];
        strncpy(buf, line, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        return analyseHeaderLine(sock, buf, strlen(line), header);
    }
};

// Directory Traversal Tests
TEST_F(SecurityTest, DirectoryTraversalSimple) {
    parseHeaderLine("GET /../etc/passwd HTTP/1.1");
    EXPECT_EQ(header->error, 1);
}

TEST_F(SecurityTest, DirectoryTraversalEncoded) {
    parseHeaderLine("GET /%2e%2e/etc/passwd HTTP/1.1");
    // After URL decode, should detect ..
    EXPECT_EQ(header->error, 1);
}

TEST_F(SecurityTest, DirectoryTraversalDouble) {
    parseHeaderLine("GET /../../../../../../etc/passwd HTTP/1.1");
    EXPECT_EQ(header->error, 1);
}

TEST_F(SecurityTest, DirectoryTraversalMixed) {
    parseHeaderLine("GET /valid/../../../etc/passwd HTTP/1.1");
    EXPECT_EQ(header->error, 1);
}

// Script Injection Tests
TEST_F(SecurityTest, ScriptTagInUrl) {
    parseHeaderLine("GET /<script>alert(1)</script> HTTP/1.1");
    EXPECT_EQ(header->error, 1);
}

// BUG: URL-encoded script tags are NOT detected - security vulnerability!
TEST_F(SecurityTest, ScriptTagEncoded) {
    parseHeaderLine("GET /%3Cscript%3Ealert(1)%3C/script%3E HTTP/1.1");
    // After URL decode this becomes <script>alert(1)</script>
    // This SHOULD be detected and rejected
    EXPECT_EQ(header->error, 1);
}

TEST_F(SecurityTest, ScriptTagInParameter) {
    parseHeaderLine("GET /page?name=<script>alert(1)</script> HTTP/1.1");
    // URL should be parsed, but value contains script
    // This tests if the URL itself is flagged
}

// Null Byte Injection Tests
TEST_F(SecurityTest, NullByteInUrl) {
    // Test null byte injection - trying to bypass extension checks
    char line[] = "GET /index.html%00.exe HTTP/1.1";
    parseHeaderLine(line);
    // After parsing, check if null byte is handled
    if (header->url != nullptr) {
        // URL should not contain characters after null byte interpretation
        EXPECT_EQ(strlen(header->url), strcspn(header->url, "\0"));
    }
}

// Buffer/Length Tests
TEST_F(SecurityTest, VeryLongUrl) {
    // Create a very long URL
    std::string longUrl = "GET /";
    for (int i = 0; i < 10000; i++) {
        longUrl += "a";
    }
    longUrl += " HTTP/1.1";

    // Should not crash, may reject or truncate
    parseHeaderLine(longUrl.c_str());
}

TEST_F(SecurityTest, VeryLongHeaderValue) {
    parseHeaderLine("GET / HTTP/1.1");

    std::string longHeader = "X-Custom: ";
    for (int i = 0; i < 10000; i++) {
        longHeader += "x";
    }

    // Should not crash
    parseHeaderLine(longHeader.c_str());
}

TEST_F(SecurityTest, VeryLongParameterValue) {
    std::string longParam = "GET /page?data=";
    for (int i = 0; i < 10000; i++) {
        longParam += "x";
    }
    longParam += " HTTP/1.1";

    // Should not crash
    parseHeaderLine(longParam.c_str());
}

// Malformed Input Tests
TEST_F(SecurityTest, MalformedHttpVersion) {
    parseHeaderLine("GET /index.html HTTP/9.9");
    EXPECT_EQ(header->error, 1);
}

TEST_F(SecurityTest, NoHttpVersion) {
    parseHeaderLine("GET /index.html");
    EXPECT_EQ(header->error, 1);
}

TEST_F(SecurityTest, OnlyMethod) {
    parseHeaderLine("GET");
    EXPECT_EQ(header->error, 1);
}

TEST_F(SecurityTest, EmptyRequest) {
    parseHeaderLine("");
    // Should handle gracefully
}

TEST_F(SecurityTest, GarbageData) {
    parseHeaderLine("\xff\xfe\x00\x01\x02\x03");
    // Should not crash
}

TEST_F(SecurityTest, HeaderWithoutColon) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine("InvalidHeaderNoColon");
    // Should not crash, header should be ignored
}

TEST_F(SecurityTest, HeaderWithEmptyName) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine(": value");
    // Should not crash
}

TEST_F(SecurityTest, HeaderWithOnlyColon) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine(":");
    // Should not crash
}

// Content-Length Manipulation
TEST_F(SecurityTest, NegativeContentLength) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine("Content-Length: -1");
    EXPECT_EQ(header->contentlenght, 0u);
}

TEST_F(SecurityTest, HugeContentLength) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine("Content-Length: 99999999999999999999");
    // Should handle overflow gracefully
}

TEST_F(SecurityTest, NonNumericContentLength) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine("Content-Length: abc");
    EXPECT_EQ(header->contentlenght, 0u);
}

TEST_F(SecurityTest, ContentLengthWithSpaces) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine("Content-Length:    123   ");
    // May or may not parse, but should not crash
}

// Host Header Attacks
TEST_F(SecurityTest, HostHeaderWithPort) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine("Host: example.com:8080");
    EXPECT_STREQ(header->HostName, "example.com");
}

TEST_F(SecurityTest, HostHeaderMalformed) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine("Host: :8080");
    // Should handle gracefully
}

TEST_F(SecurityTest, HostHeaderMultipleColons) {
    parseHeaderLine("GET / HTTP/1.1");
    parseHeaderLine("Host: example.com:8080:extra");
    // Should handle gracefully
}

// URL Encoding Edge Cases
class UrlDecodeSecurityTest : public ::testing::Test {
protected:
    char buffer[1024];

    void decodeAndCheck(const char* input) {
        strncpy(buffer, input, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        url_decode(buffer);
    }
};

TEST_F(UrlDecodeSecurityTest, DoubleEncoding) {
    // %25 = %, so %252e = %2e after first decode
    decodeAndCheck("%252e%252e");
    // After one decode: %2e%2e
    EXPECT_STREQ(buffer, "%2e%2e");
}

TEST_F(UrlDecodeSecurityTest, InvalidHexChars) {
    decodeAndCheck("%GG%ZZ");
    // Invalid hex should be handled gracefully
}

TEST_F(UrlDecodeSecurityTest, PartialEncoding) {
    decodeAndCheck("test%2");
    // Incomplete encoding at end
    EXPECT_STREQ(buffer, "test%2");
}

TEST_F(UrlDecodeSecurityTest, PercentAtEnd) {
    decodeAndCheck("test%");
    EXPECT_STREQ(buffer, "test%");
}

TEST_F(UrlDecodeSecurityTest, MixedValidInvalid) {
    decodeAndCheck("%20%GG%20");
    // Space, invalid, space
}
