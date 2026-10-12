#include <gtest/gtest.h>
#include <cstring>
#include <string>

extern "C" {
#include "webserver.h"
}

class ErrorPageTest : public ::testing::Test {
protected:
    socket_info* sock;

    void SetUp() override {
        sock = (socket_info*)calloc(1, sizeof(socket_info));
        sock->header = WebserverMallocHttpRequestHeader();
        sock->header->isHttp1_1 = 1;

        ws_list_init(&sock->header_chunk_list);
        ws_list_init(&sock->html_chunk_list);
        ws_list_init(&sock->output_list);
    }

    void TearDown() override {
        output_struct* out;
        while ((out = (output_struct*)ws_list_extract_at(&sock->output_list, 0)) != NULL) {
            if (out->header.buffer) WebserverFree(out->header.buffer);
            if (out->body.buffer)   WebserverFree(out->body.buffer);
            WebserverFree(out);
        }
        ws_list_destroy(&sock->output_list);
        ws_list_destroy(&sock->html_chunk_list);
        ws_list_destroy(&sock->header_chunk_list);

        WebserverFreeHttpRequestHeader(sock->header);
        free(sock);
    }
};

/*
 * Eine Fehlerseite darf nie komprimiert ausgeliefert werden.
 *
 * sendHeaderError() schliesst den HTTP Header selbst ab ( eigene
 * Content-Length und das leere "\r\n" ). generateOutputBuffer() haengt bei
 * gesetztem use_output_compression aber noch Content-Encoding, eine zweite
 * Content-Length und ein weiteres "\r\n" an die Header Chunk Liste - die
 * landen dann hinter dem fertigen Header, also im Body.
 *
 * Das Flag ist ein Socket Feld und wird erst nach handleClientHeaderData()
 * genullt. Auf einer Keep-Alive Verbindung steht es im Fehlerpfad also noch
 * vom vorherigen Request, wenn der eine komprimierte Seite geliefert hat.
 */
TEST_F(ErrorPageTest, ErrorPageIsNotCompressedAfterCompressedRequest) {
    /* Der vorherige Request auf derselben Verbindung lieferte komprimiert */
    sock->use_output_compression = 1;

    sendHttpErrorPage(sock, 400, 0);
    generateOutputBuffer(sock);

    ASSERT_EQ(ws_list_size(&sock->output_list), 1u);
    output_struct* out = (output_struct*)ws_list_get_at(&sock->output_list, 0);
    ASSERT_NE(out, nullptr);

    /* Unkomprimiert liegen Header und Body zusammen in body.buffer,
     * komprimiert stehen sie getrennt in header.buffer / body.buffer. */
    std::string response;
    if (out->header.buffer) response.append(out->header.buffer, out->header.buffer_size);
    if (out->body.buffer)   response.append(out->body.buffer, out->body.buffer_size);

    size_t header_end = response.find("\r\n\r\n");
    ASSERT_NE(header_end, std::string::npos) << "kein abgeschlossener Header:\n" << response;

    std::string hdr = response.substr(0, header_end);

    EXPECT_EQ(hdr.find("Content-Encoding"), std::string::npos)
        << "Fehlerseite darf nicht komprimiert werden:\n" << hdr;

    /* Genau eine Content-Length - eine zweite waere die des deflate Bodys,
     * angehaengt hinter dem bereits fertigen Header. */
    size_t first = hdr.find("Content-Length:");
    ASSERT_NE(first, std::string::npos);
    EXPECT_EQ(hdr.find("Content-Length:", first + 1), std::string::npos)
        << "zwei Content-Length im Header:\n" << hdr;

    /* Und hinter dem Header steht nur noch die HTML Fehlerseite */
    EXPECT_EQ(response.find("Content-Encoding", header_end), std::string::npos)
        << "Header Zeilen im Body:\n" << response;
}
