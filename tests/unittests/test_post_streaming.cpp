#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>
#include <ctime>

extern "C" {
#include "webserver.h"
}

/*
 * Streaming Empfang von multipart/form-data POST Uploads.
 *
 * Der interessante Teil ist nicht der Normalfall sondern die Chunk Grenze:
 * ein Boundary kann beliebig zerteilt bei der Statemachine ankommen, und
 * Nutzdaten koennen Praefixe des Boundaries enthalten ohne eines zu sein.
 * Beides muss byte genau dasselbe Ergebnis liefern wie ein Empfang am
 * Stueck, sonst landet eine korrupte Firmware im Flash.
 *
 * Die Tests schieben denselben Body mit vielen verschiedenen Chunk Groessen
 * durch und vergleichen das Ergebnis mit den Originaldaten.
 */

namespace {

/* Kommt aus intern/post_streaming.h, damit der Test dieselbe Zahl sieht wie der
 * Build - auch wenn sie per -D ueberschrieben wird. */
const int MAX_PARTS = POST_STREAM_MAX_PARTS;

const char* BOUNDARY = "----WebKitFormBoundaryAbC123";
const char* TEST_URL = "/upload/test.html";

/* Was der Handler gesehen hat */
struct capture {
    std::vector<std::string> file_names;
    std::vector<std::string> payloads;
    int  start_calls;
    int  end_calls;
    int  last_success;
    bool abort_on_first_chunk;
    bool abort_in_on_end;
};

capture g_cap;

void on_start(post_stream_context* ctx) {
    g_cap.start_calls++;
    g_cap.file_names.push_back(ctx->file_name ? ctx->file_name : "");
    g_cap.payloads.push_back(std::string());
}

void on_chunk(post_stream_context* ctx, const unsigned char* data, uint32_t length) {
    if (!g_cap.payloads.empty()) {
        g_cap.payloads.back().append((const char*)data, length);
    }
    if (g_cap.abort_on_first_chunk) {
        PostStreamAbort(ctx);
    }
}

/* Prueffunktionen fuer die Registrierung - der Consumer gibt sie mit */
int auth_allow(dummy_handler*)  { return 1; }
int auth_deny(dummy_handler*)   { return 0; }

void on_end(post_stream_context* ctx, int success) {
    g_cap.end_calls++;
    g_cap.last_success = success;
    if (g_cap.abort_in_on_end) {
        PostStreamAbort(ctx);
    }
}

/* Multipart Body wie ihn ein Browser schickt */
std::string build_body(const std::string& file_name, const std::string& payload) {
    std::string b;
    b += std::string("--") + BOUNDARY + "\r\n";
    b += "Content-Disposition: form-data; name=\"datei\"; filename=\"" + file_name + "\"\r\n";
    b += "Content-Type: application/octet-stream\r\n";
    b += "\r\n";
    b += payload;
    b += "\r\n";
    b += std::string("--") + BOUNDARY + "--\r\n";
    return b;
}

std::string build_file_part(const std::string& file_name, const std::string& payload) {
    std::string b;
    b += std::string("--") + BOUNDARY + "\r\n";
    b += "Content-Disposition: form-data; name=\"datei\"; filename=\"" + file_name + "\"\r\n";
    b += "\r\n";
    b += payload;
    b += "\r\n";
    return b;
}

std::string build_field_part(const std::string& name, const std::string& value) {
    std::string b;
    b += std::string("--") + BOUNDARY + "\r\n";
    b += "Content-Disposition: form-data; name=\"" + name + "\"\r\n";
    b += "\r\n";
    b += value;
    b += "\r\n";
    return b;
}

/* Was der spaetere URL Handler ueber getFileName()/getFileSize() sieht.
 * Sammelt ein und gibt frei - so haengt das Aufraeumen nicht daran, dass die
 * Erwartungen im Test durchlaufen. */
struct upload_entry {
    std::string name;
    uint64_t    length;
    bool        has_data;
};

/* Die intern gepufferten Parts, bevor postStreamFillUploadInfo() die Liste
 * leert. Formularfelder erscheinen nicht mehr in upload_files - was sie
 * gepuffert haben, ist nur hier zu sehen. */
const post_stream_part* findPart(socket_info* sock, const std::string& name) {
    const post_stream_part* p = sock->post_stream_parts;
    while (p) {
        if (p->name && (name == p->name)) return p;
        p = p->next;
    }
    return NULL;
}

std::vector<upload_entry> collectUploads(socket_info* sock) {
    http_request s;
    memset(&s, 0, sizeof(http_request));
    ws_list_init(&s.upload_files);
    s.socket = sock;
    s.header = sock->header;
    postStreamFillUploadInfo(&s);

    /* Die Part Eintraege sind damit weg - ein zurueckgebliebener current_part
     * waere ein Zeiger ins Nichts, in den ein spaeteres endPart() schreibt. */
    EXPECT_EQ(sock->post_stream_current_part, nullptr);

    std::vector<upload_entry> out;
    upload_file_info* f;
    ws_list_iterator_start(&s.upload_files);
    while ((f = (upload_file_info*)ws_list_iterator_next(&s.upload_files))) {
        upload_entry e;
        e.name     = f->name ? f->name : "";
        e.length   = f->length;
        e.has_data = (f->data != NULL);
        out.push_back(e);

        if (f->name) WebserverFree(f->name);
        if (f->data) WebserverFree(f->data);
        WebserverFree(f);
    }
    ws_list_iterator_stop(&s.upload_files);
    ws_list_destroy(&s.upload_files);
    return out;
}

} // namespace

class PostStreamingTest : public ::testing::Test {
protected:
    socket_info* sock;

    void SetUp() override {
        g_cap = capture();

        /* Registry lebt ueber die Testlaufzeit, darum nur einmal */
        if (findPostStreamHandler(TEST_URL) == nullptr) {
            RegisterPostStreamHandler(TEST_URL, NULL, 0, on_start, on_chunk, on_end);
        }

        /* The auth path restores the session and needs the session store.
           It is created once for the whole run, see test_main.cpp. */

        sock = (socket_info*)calloc(1, sizeof(socket_info));
        sock->header = WebserverMallocHttpRequestHeader();

        size_t ulen = strlen(TEST_URL);
        sock->header->url = (char*)WebserverMalloc(ulen + 1);
        memcpy(sock->header->url, TEST_URL, ulen + 1);

        std::string bound = std::string("--") + BOUNDARY;
        sock->header->boundary = (char*)WebserverMalloc(bound.size() + 1);
        memcpy(sock->header->boundary, bound.c_str(), bound.size() + 1);

        sock->header->contenttype = MULTIPART_FORM_DATA;
    }

    void TearDown() override {
        cleanupSocketPostStreamState(sock);
        WebserverFreeHttpRequestHeader(sock->header);
        free(sock);
    }

    /* Body in Chunks der Groesse chunk_size durchschieben */
    int feed(const std::string& body, size_t chunk_size) {
        sock->header->contentlenght = body.size();
        sock->header->post_buffer_pos = 0;

        post_stream_handler_entry* h = findPostStreamHandler(TEST_URL);
        EXPECT_NE(h, nullptr);
        EXPECT_EQ(startPostStream(sock, h, 0), 0);

        int ret = 0;
        size_t pos = 0;
        while (pos < body.size()) {
            size_t n = std::min(chunk_size, body.size() - pos);
            ret = postStreamFeed(sock, (const unsigned char*)&body[pos], (uint32_t)n);
            if (ret != 0) break;
            pos += n;
        }
        return ret;
    }
};

/* Normalfall: alles in einem Rutsch */
TEST_F(PostStreamingTest, SingleChunkRoundTrip) {
    std::string payload = "hello firmware world";
    std::string body = build_body("container.bbs", payload);

    ASSERT_EQ(feed(body, body.size()), 1);

    EXPECT_EQ(g_cap.start_calls, 1);
    EXPECT_EQ(g_cap.end_calls, 1);
    EXPECT_EQ(g_cap.last_success, 1);
    ASSERT_EQ(g_cap.payloads.size(), 1u);
    EXPECT_EQ(g_cap.payloads[0], payload);
    EXPECT_EQ(g_cap.file_names[0], "container.bbs");
}

/*
 * Der eigentliche Test: dieselben Daten, jede denkbare Zerteilung.
 * Bei Chunk Groesse 1 liegt jede einzelne Boundary Byte Grenze auf einer
 * Chunk Grenze.
 */
TEST_F(PostStreamingTest, EveryChunkSizeYieldsIdenticalPayload) {
    std::string payload = "0123456789abcdefghijklmnopqrstuvwxyz-payload-with-some-length";
    std::string body = build_body("fw.img", payload);

    for (size_t cs = 1; cs <= body.size(); cs++) {
        TearDown();
        SetUp();
        ASSERT_EQ(feed(body, cs), 1) << "chunk size " << cs;
        ASSERT_EQ(g_cap.payloads.size(), 1u) << "chunk size " << cs;
        EXPECT_EQ(g_cap.payloads[0], payload) << "chunk size " << cs;
        EXPECT_EQ(g_cap.last_success, 1) << "chunk size " << cs;
    }
}

/*
 * Nutzdaten die wie ein Boundary anfangen aber keines sind.
 * Genau hier muss der Rueckhalte Puffer die Bytes wieder herausgeben.
 */
TEST_F(PostStreamingTest, PayloadContainingBoundaryPrefix) {
    std::string payload;
    payload += "vorher";
    payload += "\r\n--";                                  /* Anfang, bricht ab   */
    payload += "\r\n----WebKitFormBoundaryAbC12";         /* fast vollstaendig   */
    payload += "\r\n\r\n--";                              /* CRLF vor dem Muster */
    payload += "nachher";

    std::string body = build_body("tricky.bin", payload);

    for (size_t cs = 1; cs <= 40; cs++) {
        TearDown();
        SetUp();
        ASSERT_EQ(feed(body, cs), 1) << "chunk size " << cs;
        ASSERT_EQ(g_cap.payloads.size(), 1u) << "chunk size " << cs;
        EXPECT_EQ(g_cap.payloads[0], payload) << "chunk size " << cs;
    }
}

/* Binaerdaten mit allen 256 Bytewerten, mehrfach wiederholt */
TEST_F(PostStreamingTest, BinaryPayloadRoundTrip) {
    std::string payload;
    for (int rep = 0; rep < 40; rep++) {
        for (int i = 0; i < 256; i++) {
            payload.push_back((char)i);
        }
    }
    std::string body = build_body("binary.img", payload);

    const size_t sizes[] = { 1, 2, 3, 7, 13, 64, 255, 256, 257, 1024, 4096 };
    for (size_t si = 0; si < sizeof(sizes)/sizeof(sizes[0]); si++) {
        TearDown();
        SetUp();
        ASSERT_EQ(feed(body, sizes[si]), 1) << "chunk size " << sizes[si];
        ASSERT_EQ(g_cap.payloads.size(), 1u);
        EXPECT_EQ(g_cap.payloads[0], payload) << "chunk size " << sizes[si];
        EXPECT_EQ(g_cap.payloads[0].size(), payload.size());
    }
}

/* Datei mit Laenge 0 */
TEST_F(PostStreamingTest, EmptyPayload) {
    std::string body = build_body("empty.bin", "");

    for (size_t cs = 1; cs <= 20; cs++) {
        TearDown();
        SetUp();
        ASSERT_EQ(feed(body, cs), 1) << "chunk size " << cs;
        ASSERT_EQ(g_cap.payloads.size(), 1u);
        EXPECT_EQ(g_cap.payloads[0], "");
        EXPECT_EQ(g_cap.last_success, 1);
    }
}

/*
 * Formularfeld vor der Datei. Das Feld wird gepuffert, nur der Datei Part
 * geht an den Handler.
 */
TEST_F(PostStreamingTest, FormFieldIsBufferedFileIsStreamed) {
    std::string payload = "der eigentliche inhalt";
    std::string body = build_field_part("action", "upload_file");
    body += "Content-Disposition: form-data; name=\"datei\"; filename=\"x.bin\"\r\n";

    /* build_field_part endet vor dem naechsten Boundary, darum hier komplett bauen */
    body = build_field_part("action", "upload_file");
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"datei\"; filename=\"x.bin\"\r\n\r\n";
    body += payload;
    body += "\r\n";
    body += std::string("--") + BOUNDARY + "--\r\n";

    for (size_t cs = 1; cs <= 30; cs++) {
        TearDown();
        SetUp();
        ASSERT_EQ(feed(body, cs), 1) << "chunk size " << cs;

        /* on_start/on_chunk nur fuer den Datei Part */
        EXPECT_EQ(g_cap.start_calls, 1) << "chunk size " << cs;
        ASSERT_EQ(g_cap.payloads.size(), 1u);
        EXPECT_EQ(g_cap.payloads[0], payload) << "chunk size " << cs;
        EXPECT_EQ(g_cap.file_names[0], "x.bin");

        /* Beide Parts liegen am Socket, zuletzt gesehener vorne.
           Vorne die Datei ( gestreamt, ohne data ), dahinter das Formularfeld. */
        ASSERT_NE(sock->post_stream_parts, nullptr) << "chunk size " << cs;
        EXPECT_EQ(sock->post_stream_parts->is_file, 1) << "chunk size " << cs;
        EXPECT_STREQ(sock->post_stream_parts->name, "x.bin");
        EXPECT_EQ(sock->post_stream_parts->data, nullptr) << "Datei ist gestreamt";

        ASSERT_NE(sock->post_stream_parts->next, nullptr) << "chunk size " << cs;
        EXPECT_STREQ(sock->post_stream_parts->next->name, "action");
        EXPECT_STREQ(sock->post_stream_parts->next->data, "upload_file");

    }
}

/* Zwei Dateien nacheinander */
TEST_F(PostStreamingTest, TwoFileParts) {
    std::string p1 = "erste datei";
    std::string p2 = "zweite datei mit mehr inhalt";

    std::string body;
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"a\"; filename=\"a.bin\"\r\n\r\n";
    body += p1 + "\r\n";
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"b\"; filename=\"b.bin\"\r\n\r\n";
    body += p2 + "\r\n";
    body += std::string("--") + BOUNDARY + "--\r\n";

    for (size_t cs = 1; cs <= 25; cs++) {
        TearDown();
        SetUp();
        ASSERT_EQ(feed(body, cs), 1) << "chunk size " << cs;
        EXPECT_EQ(g_cap.start_calls, 2) << "chunk size " << cs;
        EXPECT_EQ(g_cap.end_calls, 2) << "chunk size " << cs;
        ASSERT_EQ(g_cap.payloads.size(), 2u);
        EXPECT_EQ(g_cap.payloads[0], p1) << "chunk size " << cs;
        EXPECT_EQ(g_cap.payloads[1], p2) << "chunk size " << cs;
        EXPECT_EQ(g_cap.file_names[0], "a.bin");
        EXPECT_EQ(g_cap.file_names[1], "b.bin");
    }
}

/* Bricht der Handler ab, muss on_end mit success == 0 kommen */
TEST_F(PostStreamingTest, HandlerAbortReportsFailure) {
    g_cap.abort_on_first_chunk = true;

    std::string body = build_body("abort.bin", "irgendwelche daten die abgebrochen werden");
    feed(body, 8);

    EXPECT_EQ(g_cap.start_calls, 1);
    EXPECT_GE(g_cap.end_calls, 1);
    EXPECT_EQ(g_cap.last_success, 0);
}

/*
 * Bricht der Body vor dem End Boundary ab, darf der Upload nicht als
 * erfolgreich gemeldet werden. Sonst landet eine abgeschnittene Firmware
 * im Flash.
 */
TEST_F(PostStreamingTest, TruncatedBodyIsNotReportedAsSuccess) {
    std::string payload = "eine firmware die nie zu ende uebertragen wird";
    std::string full = build_body("truncated.bin", payload);

    /* Content-Length verspricht den ganzen Body, es kommt aber weniger */
    std::string truncated = full.substr(0, full.size() - 10);

    sock->header->contentlenght = truncated.size();
    sock->header->post_buffer_pos = 0;

    post_stream_handler_entry* h = findPostStreamHandler(TEST_URL);
    ASSERT_NE(h, nullptr);
    ASSERT_EQ(startPostStream(sock, h, 0), 0);

    int ret = postStreamFeed(sock, (const unsigned char*)truncated.data(), (uint32_t)truncated.size());

    EXPECT_EQ(ret, -1) << "abgeschnittener Body muss als Fehler gemeldet werden";
    EXPECT_EQ(g_cap.last_success, 0);
}

/*
 * Der Header Parser legt die URL ohne fuehrenden Slash ab. Registrierung mit
 * Slash muss trotzdem greifen, sonst laeuft der Upload still im alten Pfad.
 */
TEST_F(PostStreamingTest, UrlMatchesWithAndWithoutLeadingSlash) {
    ASSERT_NE(findPostStreamHandler(TEST_URL), nullptr) << "mit fuehrendem Slash";

    const char* without = TEST_URL + 1;   /* "upload/test.html" */
    EXPECT_EQ(findPostStreamHandler(without), findPostStreamHandler(TEST_URL))
        << "so kommt die URL aus dem Header Parser an";
}

/*
 * Die Pruefung des Consumers landet in der Registry. Ausgewertet wird sie in
 * check_post_header(), das den Body ohne Berechtigung gar nicht erst liest -
 * ohne sie wuerde ein Unberechtigter das tmpfs vollschreiben.
 */
TEST_F(PostStreamingTest, AuthCheckIsStored) {
    const char* url = "/upload/authed.html";

    if (findPostStreamHandler(url) == nullptr) {
        RegisterPostStreamHandler(url, auth_allow, 0, on_start, on_chunk, on_end);
    }

    post_stream_handler_entry* h = findPostStreamHandler(url);
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(h->auth_check, auth_allow);

    post_stream_handler_entry* open_h = findPostStreamHandler(TEST_URL);
    ASSERT_NE(open_h, nullptr);
    EXPECT_EQ(open_h->auth_check, (post_stream_auth_handler)NULL) << "ohne Pruefung darf jeder";
}

/*
 * Fehlt die Berechtigung, wird der Body zwar gelesen ( sonst haengt die
 * Verbindung ) aber nichts davon weitergereicht. Der Handler darf gar nicht
 * erst aufgerufen werden, und getFileName() muss 0 liefern - sonst wuerde ein
 * Webmodul eine Datei annehmen, die nie geschrieben wurde.
 */
TEST_F(PostStreamingTest, DiscardModeDropsPayloadAndReportsNoFile) {
    std::string payload = "diese daten duerfen nirgendwo landen";
    std::string body = build_body("verworfen.bin", payload);

    sock->header->contentlenght = body.size();
    sock->header->post_buffer_pos = 0;

    post_stream_handler_entry* h = findPostStreamHandler(TEST_URL);
    ASSERT_NE(h, nullptr);
    ASSERT_EQ(startPostStream(sock, h, 1), 0);   /* discard */

    int ret = 0;
    size_t pos = 0;
    while (pos < body.size()) {
        size_t n = std::min((size_t)7, body.size() - pos);
        ret = postStreamFeed(sock, (const unsigned char*)&body[pos], (uint32_t)n);
        if (ret != 0) break;
        pos += n;
    }
    ASSERT_EQ(ret, 1);

    EXPECT_EQ(g_cap.start_calls, 0) << "on_start darf im Discard Modus nicht laufen";
    EXPECT_EQ(g_cap.end_calls, 0)   << "on_end darf im Discard Modus nicht laufen";
    EXPECT_TRUE(g_cap.payloads.empty());

    /* keine Datei-Metadaten an den Request weiterreichen */
    http_request s;
    memset(&s, 0, sizeof(http_request));
    ws_list_init(&s.upload_files);
    s.socket = sock;
    s.header = sock->header;
    postStreamFillUploadInfo(&s);
    EXPECT_EQ(ws_list_size(&s.upload_files), 0u) << "verworfener Upload darf keine Datei melden";
    ws_list_destroy(&s.upload_files);
}

/*
 * Im Discard Modus darf NICHTS in upload_files landen - auch keine
 * Formularfelder. Sonst sickert der Feldname eines unangemeldeten Requests
 * durch, und ein Handler, der nur ws_list_size() prueft, haelt den Upload
 * faelschlich fuer vorhanden. Zudem waere data == NULL ein Crash-Kandidat
 * fuer jeden Aufrufer von getFileData().
 */
TEST_F(PostStreamingTest, DiscardModeLeaksNoFormFields) {
    std::string body;
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"secretfield\"\r\n\r\n";
    body += "geheim\r\n";
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"datei\"; filename=\"x.bin\"\r\n\r\n";
    body += "inhalt\r\n";
    body += std::string("--") + BOUNDARY + "--\r\n";

    sock->header->contentlenght = body.size();
    sock->header->post_buffer_pos = 0;

    post_stream_handler_entry* h = findPostStreamHandler(TEST_URL);
    ASSERT_NE(h, nullptr);
    ASSERT_EQ(startPostStream(sock, h, 1), 0);   /* discard */

    int ret = 0;
    size_t pos = 0;
    while (pos < body.size()) {
        size_t n = std::min((size_t)11, body.size() - pos);
        ret = postStreamFeed(sock, (const unsigned char*)&body[pos], (uint32_t)n);
        if (ret != 0) break;
        pos += n;
    }
    ASSERT_EQ(ret, 1);

    EXPECT_EQ(sock->post_stream_parts, nullptr) << "im Discard Modus darf kein Part entstehen";

    http_request s;
    memset(&s, 0, sizeof(http_request));
    ws_list_init(&s.upload_files);
    s.socket = sock;
    s.header = sock->header;
    postStreamFillUploadInfo(&s);

    EXPECT_EQ(ws_list_size(&s.upload_files), 0u)
        << "weder Datei noch Formularfeld duerfen gemeldet werden";
    ws_list_destroy(&s.upload_files);
}

/*
 * name= ist ein Teilstring von filename=. Steht filename zuerst, oder enthaelt
 * ein Dateiname selbst " name=", darf die Suche nicht darauf hereinfallen -
 * post_stream_part_is_file haengt an file_name und damit die Einordnung
 * Datei/Formularfeld.
 */
TEST_F(PostStreamingTest, DispositionNameIsNotConfusedWithFilename) {
    /* filename vor name, spec-konform nur unueblich */
    std::string body;
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; filename=\"echt.bin\"; name=\"feld\"\r\n\r\n";
    body += "daten\r\n";
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 13), 1);

    ASSERT_EQ(g_cap.file_names.size(), 1u) << "muss als Datei erkannt werden";
    EXPECT_EQ(g_cap.file_names[0], "echt.bin");
    EXPECT_EQ(g_cap.payloads[0], "daten");
}

TEST_F(PostStreamingTest, DispositionSurvivesNameInsideFilename) {
    /* Der Dateiname enthaelt selbst " name=" - die Suche muss weitersuchen */
    std::string body;
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; filename=\"mein name=x.bin\"; name=\"feld\"\r\n\r\n";
    body += "daten\r\n";
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 17), 1);

    ASSERT_EQ(g_cap.file_names.size(), 1u);
    EXPECT_EQ(g_cap.file_names[0], "mein name=x.bin");
    EXPECT_EQ(g_cap.payloads[0], "daten");
}

/*
 * Die Metadaten, die der spaetere URL-Handler ueber getFileName()/getFileSize()
 * sieht. Der ctx wird ueber alle Parts wiederverwendet - ohne eigenen Eintrag
 * pro Part verschwindet eine Datei, sobald ihr ein Formularfeld folgt.
 */
TEST_F(PostStreamingTest, UploadInfoKeepsFileWhenFieldFollows) {
    std::string payload = "der dateiinhalt";

    std::string body;
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"datei\"; filename=\"fw.bin\"\r\n\r\n";
    body += payload + "\r\n";
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"action\"\r\n\r\n";
    body += "upload_file\r\n";
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 9), 1);

    http_request s;
    memset(&s, 0, sizeof(http_request));
    ws_list_init(&s.upload_files);
    s.socket = sock;
    s.header = sock->header;
    postStreamFillUploadInfo(&s);

    ASSERT_EQ(ws_list_size(&s.upload_files), 1u) << "nur die Datei, nicht das Formularfeld";
    EXPECT_STREQ(getFileName((dummy_handler*)&s, 0), "fw.bin") << "die Datei steht an Position 0";
    EXPECT_EQ((size_t)getFileSize((dummy_handler*)&s, 0), payload.size());
    EXPECT_EQ(getFileData((dummy_handler*)&s, 0), nullptr) << "gestreamt, liegt nicht im RAM";

    upload_file_info* f;
    ws_list_iterator_start(&s.upload_files);
    while ((f = (upload_file_info*)ws_list_iterator_next(&s.upload_files))) {
        if (f->name) WebserverFree(f->name);
        if (f->data) WebserverFree(f->data);
        WebserverFree(f);
    }
    ws_list_iterator_stop(&s.upload_files);
    ws_list_destroy(&s.upload_files);
}

/* Zwei Dateien muessen beide sichtbar sein, nicht nur die letzte */
TEST_F(PostStreamingTest, UploadInfoListsBothFiles) {
    std::string body;
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"a\"; filename=\"a.bin\"\r\n\r\n";
    body += "aaa\r\n";
    body += std::string("--") + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"b\"; filename=\"b.bin\"\r\n\r\n";
    body += "bbbbb\r\n";
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 7), 1);

    http_request s;
    memset(&s, 0, sizeof(http_request));
    ws_list_init(&s.upload_files);
    s.socket = sock;
    s.header = sock->header;
    postStreamFillUploadInfo(&s);

    ASSERT_EQ(ws_list_size(&s.upload_files), 2u);
    EXPECT_STREQ(getFileName((dummy_handler*)&s, 0), "a.bin");
    EXPECT_EQ(getFileSize((dummy_handler*)&s, 0), 3);
    EXPECT_STREQ(getFileName((dummy_handler*)&s, 1), "b.bin");
    EXPECT_EQ(getFileSize((dummy_handler*)&s, 1), 5);

    upload_file_info* f;
    ws_list_iterator_start(&s.upload_files);
    while ((f = (upload_file_info*)ws_list_iterator_next(&s.upload_files))) {
        if (f->name) WebserverFree(f->name);
        if (f->data) WebserverFree(f->data);
        WebserverFree(f);
    }
    ws_list_iterator_stop(&s.upload_files);
    ws_list_destroy(&s.upload_files);
}

/*
 * Ein abgebrochener Part darf nicht als vorhandene Datei gemeldet werden -
 * der Handler hat seine Teildatei in on_end( ctx, 0 ) bereits weggeraeumt.
 */
TEST_F(PostStreamingTest, AbortedFileIsNotReportedAsUpload) {
    g_cap.abort_on_first_chunk = true;

    std::string body = build_body("abort.bin", "daten die abgebrochen werden");
    feed(body, 8);

    http_request s;
    memset(&s, 0, sizeof(http_request));
    ws_list_init(&s.upload_files);
    s.socket = sock;
    s.header = sock->header;
    postStreamFillUploadInfo(&s);

    EXPECT_EQ(ws_list_size(&s.upload_files), 0u)
        << "abgebrochener Upload darf keine Datei melden";
    ws_list_destroy(&s.upload_files);
}

/*
 * Der harte Fall: die Nutzdaten enthalten ein VOLLSTAENDIGES "\r\n--<boundary>",
 * gefolgt von einem beliebigen Zeichen. Nach RFC 2046 ist das kein Delimiter -
 * dort muss CRLF oder "--" folgen. Wird das nicht geprueft, meldet on_end()
 * Erfolg fuer eine abgeschnittene Datei, und der Handler haelt sie fuer
 * vollstaendig. Ein Client kann das gezielt konstruieren, er kennt das
 * Boundary ja selbst.
 */
TEST_F(PostStreamingTest, FullBoundaryInPayloadIsNotADelimiter) {
    std::string payload;
    payload += "vorher";
    payload += std::string("\r\n--") + BOUNDARY + "X";   /* echtes Boundary, ungueltige Fortsetzung */
    payload += "nachher";
    payload += std::string("\r\n--") + BOUNDARY + "-";   /* nur ein Strich, auch ungueltig */
    payload += "ende";

    std::string body = build_body("tricky2.bin", payload);

    for (size_t cs = 1; cs <= 40; cs++) {
        TearDown();
        SetUp();
        ASSERT_EQ(feed(body, cs), 1) << "chunk size " << cs;
        ASSERT_EQ(g_cap.payloads.size(), 1u) << "chunk size " << cs;
        EXPECT_EQ(g_cap.payloads[0], payload)
            << "chunk size " << cs << ": Payload mit Boundary-Sequenz muss unveraendert ankommen";
        EXPECT_EQ(g_cap.end_calls, 1) << "chunk size " << cs << ": genau ein Part";
        EXPECT_EQ(g_cap.last_success, 1) << "chunk size " << cs;
    }
}

/*
 * Ein eigenes Limit pro URL. Ohne das muesste max_post_size global auf
 * Firmware-Groesse stehen, und an jedem beliebigen Endpunkt liesse sich ein
 * Body dieser Groesse abladen.
 */
TEST_F(PostStreamingTest, PerUrlMaxSizeIsStoredAndReported) {
    const char* big_url = "/upload/firmware.html";
    const uint64_t limit = 46u * 1024u * 1024u;

    if (findPostStreamHandler(big_url) == nullptr) {
        RegisterPostStreamHandler(big_url, auth_allow, limit, on_start, on_chunk, on_end);
    }

    post_stream_handler_entry* h = findPostStreamHandler(big_url);
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(h->max_size, limit);

    /* 0 heisst: globales max_post_size verwenden */
    post_stream_handler_entry* plain = findPostStreamHandler(TEST_URL);
    ASSERT_NE(plain, nullptr);
    EXPECT_EQ(plain->max_size, 0u);

    /* Das Frontend fragt nach der groesstmoeglichen Uploadgroesse */
    EXPECT_GE(GetMaxPostStreamSize(), limit);
}

/* URL ohne registrierten Handler darf keinen Stream aufsetzen */
TEST_F(PostStreamingTest, UnregisteredUrlHasNoHandler) {
    EXPECT_EQ(findPostStreamHandler("/nicht/registriert.html"), nullptr);
}

/*
 * Gesamtbudget fuer gepufferte POST Bodies.
 *
 * max_post_size begrenzt nur den einzelnen Request. Der Puffer wird aber
 * sofort in voller Content-Length belegt, sobald das erste Byte eintrifft -
 * ein Client kann also mit vielen parallelen Verbindungen Speicher binden,
 * ohne die Daten je zu senden. Das Budget deckelt die Summe.
 */
class PostBudgetTest : public ::testing::Test {
protected:
    void SetUp() override {
        /* Der Config-Store ist global und wird von anderen Tests mitbenutzt.
           Darum nur einmal anlegen und nie freigeben - ein freeConfig() hier
           laesst nachfolgende Tests auf einen toten Store zugreifen. */
        static bool config_ready = false;
        if (!config_ready) {
            initConfig();
            /* Das Budget hat einen eigenen Mutex. Ohne diesen Aufruf laeuft
               postBufferReserve() auf einen nie initialisierten Mutex - unter
               Linux faellt das nur nicht auf, weil ein nullinitialisierter
               pthread_mutex zufaellig gueltig ist. */
            initPostBufferBudget();
            config_ready = true;
        }
        /* Reste aus anderen Tests abraeumen - das Budget ist ebenfalls global */
        postBufferRelease(postBufferAllocated());
    }

    void TearDown() override {
        postBufferRelease(postBufferAllocated());
    }
};

TEST_F(PostBudgetTest, ReserveSucceedsWithinLimit) {
    setConfigInt("max_post_memory_total", 1000);

    EXPECT_EQ(postBufferReserve(400), 1);
    EXPECT_EQ(postBufferAllocated(), 400u);

    EXPECT_EQ(postBufferReserve(600), 1) << "genau bis zum Limit muss gehen";
    EXPECT_EQ(postBufferAllocated(), 1000u);
}

TEST_F(PostBudgetTest, ReserveFailsBeyondLimit) {
    setConfigInt("max_post_memory_total", 1000);

    ASSERT_EQ(postBufferReserve(900), 1);
    EXPECT_EQ(postBufferReserve(200), 0) << "ueber dem Limit muss abgelehnt werden";
    EXPECT_EQ(postBufferAllocated(), 900u) << "abgelehnte Anforderung darf nichts belegen";
}

/* Das ist der Angriff: viele Verbindungen, jede fuer sich unter max_post_size */
TEST_F(PostBudgetTest, ManySmallReservationsHitTheLimit) {
    setConfigInt("max_post_memory_total", 10000);

    int granted = 0;
    for (int i = 0; i < 100; i++) {
        if (postBufferReserve(1000)) granted++;
    }

    EXPECT_EQ(granted, 10) << "in Summe darf nur das Budget vergeben werden";
    EXPECT_EQ(postBufferAllocated(), 10000u);
}

TEST_F(PostBudgetTest, ReleaseGivesBudgetBack) {
    setConfigInt("max_post_memory_total", 1000);

    ASSERT_EQ(postBufferReserve(1000), 1);
    ASSERT_EQ(postBufferReserve(1), 0);

    postBufferRelease(500);
    EXPECT_EQ(postBufferAllocated(), 500u);
    EXPECT_EQ(postBufferReserve(500), 1) << "freigegebenes Budget muss wieder nutzbar sein";
}

/* Darf nie unter null laufen, sonst waere danach unbegrenzt Budget da */
TEST_F(PostBudgetTest, ReleaseDoesNotUnderflow) {
    setConfigInt("max_post_memory_total", 1000);

    ASSERT_EQ(postBufferReserve(100), 1);
    postBufferRelease(999999);

    EXPECT_EQ(postBufferAllocated(), 0u);
    EXPECT_EQ(postBufferReserve(1000), 1);
}

TEST_F(PostBudgetTest, ZeroLimitDisablesBudget) {
    setConfigInt("max_post_memory_total", 0);

    EXPECT_EQ(postBufferReserve(100u * 1024u * 1024u), 1) << "0 = Budget abgeschaltet";
}

/*
 * PostStreamAbort() gilt fuer den restlichen Request, nicht nur fuer den
 * laufenden Part. Sonst laeuft der Handler nach dem Abbruch beim naechsten
 * Part wieder an und schreibt weiter - genau das, was der Abbruch verhindern
 * soll.
 */
TEST_F(PostStreamingTest, AbortStaysActiveForFollowingParts) {
    g_cap.abort_on_first_chunk = true;

    std::string body;
    body += build_file_part("erste.bin", "daten die abgebrochen werden");
    body += build_file_part("zweite.bin", "diese darf nicht mehr ankommen");
    body += std::string("--") + BOUNDARY + "--\r\n";

    feed(body, 8);

    EXPECT_EQ(g_cap.last_success, 0);

    /* Und er darf den Handler gar nicht mehr anlaufen lassen: on_start legt
     * beim echten Consumer die Zieldatei an und loescht dabei die vorhandene.
     * Ohne Nutzdaten dazwischen ist das reine Zerstoerung. */
    EXPECT_EQ(g_cap.start_calls, 1) << "on_start darf nach dem Abbruch nicht mehr laufen";
    EXPECT_EQ(g_cap.end_calls, 1)   << "on_end darf nach dem Abbruch nicht mehr laufen";
}

/*
 * Ein Part kostet einen Eintrag und seinen Namen. Ein leerer Part braucht im
 * Body nur ein paar Dutzend Bytes - ohne Obergrenze bindet ein Upload in
 * Firmwaregroesse damit hunderttausende Eintraege, am Gesamtbudget vorbei.
 */
TEST_F(PostStreamingTest, TooManyPartsAbortsTheRequest) {
    std::string body;
    for (int i = 0; i < 200; i++) {
        char name[32];
        snprintf(name, sizeof(name), "feld%d", i);
        body += build_field_part(name, "x");
    }
    body += std::string("--") + BOUNDARY + "--\r\n";

    /* Der Rueckgabewert gehoert zur Zusage: der Request laeuft normal weiter,
     * nur ohne die Parts ueber dem Limit. */
    ASSERT_EQ(feed(body, 64), 1);

    /* Nicht alle 200 Felder duerfen angelegt werden. Formularfelder erscheinen
     * nicht in upload_files, die Grenze ist deshalb am internen Zaehler zu
     * sehen - und der wird von postStreamFillUploadInfo() genullt, also vorher
     * pruefen. */
    EXPECT_EQ(sock->post_stream_parts_count, (uint32_t)MAX_PARTS)
        << "genau bis POST_STREAM_MAX_PARTS, danach Abbruch";

    http_request s;
    memset(&s, 0, sizeof(http_request));
    ws_list_init(&s.upload_files);
    s.socket = sock;
    s.header = sock->header;
    postStreamFillUploadInfo(&s);

    EXPECT_EQ(ws_list_size(&s.upload_files), 0u) << "Formularfelder sind keine Dateien";

    upload_file_info* f;
    ws_list_iterator_start(&s.upload_files);
    while ((f = (upload_file_info*)ws_list_iterator_next(&s.upload_files))) {
        if (f->name) WebserverFree(f->name);
        if (f->data) WebserverFree(f->data);
        WebserverFree(f);
    }
    ws_list_iterator_stop(&s.upload_files);
    ws_list_destroy(&s.upload_files);
}

/*
 * Am Part Limit darf der letzte noch angenommene Part nicht nachtraeglich
 * kaputtgehen. Ueber dem Limit legt beginPart() keinen Eintrag mehr an, das
 * nachfolgende endPart() darf deshalb nicht auf den Listenkopf zugreifen - das
 * ist der vorherige, fertige Part. Ist der eine Datei, wuerde sie auf Laenge 0
 * gesetzt und als fehlgeschlagen markiert, obwohl der Handler sie komplett
 * bekommen und weggeschrieben hat.
 */
TEST_F(PostStreamingTest, PartLimitKeepsPreviousFilePart) {
    const std::string payload = "firmware container inhalt";

    std::string body;
    for (int i = 0; i < MAX_PARTS - 1; i++) {                /* Parts 1..63 */
        char name[32];
        snprintf(name, sizeof(name), "feld%d", i);
        body += build_field_part(name, "x");
    }
    body += build_file_part("container.bbs", payload);       /* Part 64, noch erlaubt */
    body += build_field_part("action", "upload");            /* Part 65, ueber dem Limit */
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 64), 1);

    /* Der Handler hat die Datei vollstaendig bekommen */
    EXPECT_EQ(g_cap.start_calls, 1);
    EXPECT_EQ(g_cap.end_calls, 1);
    EXPECT_EQ(g_cap.last_success, 1);
    ASSERT_EQ(g_cap.payloads.size(), 1u);
    EXPECT_EQ(g_cap.payloads[0], payload);

    EXPECT_EQ(sock->post_stream_parts_count, (uint32_t)MAX_PARTS)
        << "genau bis POST_STREAM_MAX_PARTS angelegt";

    /* und sie muss dem URL Handler auch gemeldet werden */
    std::vector<upload_entry> uploads = collectUploads(sock);

    const upload_entry* found = NULL;
    for (size_t i = 0; i < uploads.size(); i++) {
        if (uploads[i].name == "container.bbs") found = &uploads[i];
    }

    ASSERT_NE(found, (const upload_entry*)NULL) << "die Datei aus Part 64 fehlt in upload_files";
    EXPECT_EQ(found->length, payload.size()) << "Laenge der Datei aus Part 64 ueberschrieben";
    EXPECT_EQ(uploads.size(), 1u) << "gemeldet wird die Datei, nicht die Felder";
}

/*
 * Nach PostStreamAbort() gilt der restliche Request als verworfen. Ein danach
 * folgendes Formularfeld darf deshalb nicht doch noch in upload_files landen -
 * ein Handler, der nur auf das Feld schaut ( action=upload ), wuerde sonst
 * weiterarbeiten, obwohl die zugehoerige Datei abgebrochen wurde.
 */
TEST_F(PostStreamingTest, AbortDropsFollowingFormFields) {
    g_cap.abort_on_first_chunk = true;

    std::string body;
    body += build_file_part("abgebrochen.bin", "daten die abgebrochen werden");
    body += build_field_part("action", "upload");
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 16), 1);

    EXPECT_EQ(collectUploads(sock).size(), 0u)
        << "nach dem Abbruch darf weder Datei noch Formularfeld gemeldet werden";
}

/*
 * Der Handler darf auch noch in on_end abbrechen - dort faellt zum Beispiel
 * eine Pruefsumme ueber die fertige Datei auf. Danach darf die Datei nicht
 * mehr als vorhanden gemeldet werden.
 */
TEST_F(PostStreamingTest, AbortInOnEndDropsTheFile) {
    g_cap.abort_in_on_end = true;

    std::string body = build_body("pruefsumme_falsch.bin", "inhalt der nicht passt");

    ASSERT_EQ(feed(body, 12), 1);

    EXPECT_EQ(g_cap.start_calls, 1);
    EXPECT_EQ(g_cap.end_calls, 1);

    EXPECT_EQ(collectUploads(sock).size(), 0u)
        << "in on_end abgebrochene Datei darf nicht in upload_files stehen";
}

/*
 * Ein NUL Byte im Part Header darf den Event Loop nicht anhalten.
 *
 * analyseFormDataLine() sucht das CRLF hinter dem Feldnamen mit stringfind(),
 * das intern strlen() benutzt und am NUL endet. Ohne Treffer liefert es 0, die
 * Bedingung crlf_pos > h_len ist dann falsch und das Makro macht ein continue,
 * ohne line2/length2 zu bewegen: eine Endlosschleife mitten im Parser. Der
 * Webserver arbeitet den Event Loop synchron ab, damit steht der ganze Server.
 *
 * Der Weg dorthin liegt vor jeder Berechtigungspruefung - im Discard Modus wird
 * der Part Header genauso ausgewertet. Ein NUL Byte in einem Header ist nach
 * RFC 7578 ungueltig, der Request muss also abgelehnt werden.
 */
TEST_F(PostStreamingTest, NulByteInPartHeaderIsRejected) {
    std::string body;
    body += std::string("--") + BOUNDARY + "\r\n";
    body += std::string("Content-Disposition: form-data; name=\"x\0\"\r\n", 43);
    body += "\r\n";
    body += "daten\r\n";
    body += std::string("--") + BOUNDARY + "--\r\n";

    sock->header->contenttype = MULTIPART_FORM_DATA;
    sock->header->contentlenght = body.size();
    sock->header->post_buffer_pos = 0;

    post_stream_handler_entry* h = findPostStreamHandler(TEST_URL);
    ASSERT_NE(h, nullptr);
    ASSERT_EQ(startPostStream(sock, h, 1), 0);   /* discard: ohne Anmeldung erreichbar */

    int ret = 0;
    size_t pos = 0;
    while (pos < body.size()) {
        size_t n = std::min((size_t)32, body.size() - pos);
        ret = postStreamFeed(sock, (const unsigned char*)&body[pos], (uint32_t)n);
        if (ret != 0) break;
        pos += n;
    }

    EXPECT_EQ(ret, -1) << "kaputter Part Header muss den Request ablehnen";
}

/*
 * Nach dem Abbruch wird der restliche Body nur noch ueberlesen - er darf den
 * Request nicht mehr scheitern lassen.
 *
 * Die Statemachine sammelte auch nach ctx->aborted jeden weiteren Part Header
 * weiter und prueft dabei die 8 KiB Grenze. Ein einziger ueberlanger Header
 * hinter dem Abbruch machte daraus einen Parserfehler: postStreamFeed()
 * lieferte -1, der Socket setzte header->error und der Client bekam eine
 * generische 400 statt der Antwort seines URL Handlers. Die API sagt aber zu,
 * den Rest zu verwerfen und normal weiterzuleiten.
 */
TEST_F(PostStreamingTest, OverlongHeaderAfterAbortDoesNotFailTheRequest) {
    g_cap.abort_on_first_chunk = true;

    std::string langer_name(9000, 'a');

    std::string body;
    body += build_file_part("abgebrochen.bin", "daten die abgebrochen werden");
    body += build_field_part(langer_name, "x");
    body += std::string("--") + BOUNDARY + "--\r\n";

    EXPECT_EQ(feed(body, 512), 1)
        << "verworfener Rest darf den Request nicht mit 400 beenden";
}

/*
 * Dasselbe am Part Limit: ist es erreicht, gilt der Rest als verworfen. Ein
 * ueberlanger Header danach darf die zuvor akzeptierten Parts nicht noch
 * mitreissen.
 */
TEST_F(PostStreamingTest, OverlongHeaderAfterPartLimitDoesNotFailTheRequest) {
    std::string langer_name(9000, 'a');

    std::string body;
    for (int i = 0; i < MAX_PARTS; i++) {
        char name[32];
        snprintf(name, sizeof(name), "feld%d", i);
        body += build_field_part(name, "x");
    }
    body += build_field_part(langer_name, "x");     /* ueber dem Limit */
    body += std::string("--") + BOUNDARY + "--\r\n";

    EXPECT_EQ(feed(body, 512), 1)
        << "verworfener Rest darf den Request nicht mit 400 beenden";

    EXPECT_EQ(sock->post_stream_parts_count, (uint32_t)MAX_PARTS)
        << "die Parts bis zum Limit muessen erhalten bleiben";
}

/*
 * Ein Formularfeld ueber der Groessengrenze wird gekuerzt - der gekuerzte Wert
 * darf nicht als gueltige Eingabe gelten.
 *
 * emitPartData() setzt bei Ueberschreitung nur part->truncated und verwirft den
 * Rest. Ein Handler koennte den abgeschnittenen Wert nicht von einem
 * tatsaechlich so gesendeten kuerzeren unterscheiden - aus einer
 * Groessenueberschreitung wuerde dann eine erfolgreich uebernommene,
 * veraenderte Einstellung. Der Request selbst bleibt gueltig.
 */
TEST_F(PostStreamingTest, TruncatedFormFieldIsMarked) {
    /* POST_STREAM_MAX_FIELD_SIZE ist 64 KiB */
    std::string zu_gross(70u * 1024u, 'x');

    std::string body;
    body += build_field_part("config", zu_gross);
    body += build_field_part("action", "save");
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 4096), 1) << "der Request selbst bleibt gueltig";

    const post_stream_part* config = findPart(sock, "config");
    ASSERT_NE(config, (const post_stream_part*)NULL);
    EXPECT_EQ(config->truncated, 1) << "das uebergrosse Feld muss als gekuerzt markiert sein";

    const post_stream_part* action = findPart(sock, "action");
    ASSERT_NE(action, (const post_stream_part*)NULL);
    EXPECT_EQ(action->truncated, 0) << "die uebrigen Felder bleiben unberuehrt";

    EXPECT_EQ(collectUploads(sock).size(), 0u) << "Formularfelder sind keine Dateien";
}
TEST_F(PostStreamingTest, FieldLimitIsLoggedOnlyOncePerRequest) {
    /* 40 KiB pro Feld: unter der Feldgrenze von 64 KiB, aber jedes Feld reisst
     * die Gesamtsumme von 256 KiB erneut, sobald sie einmal voll ist. */
    std::string gross(40u * 1024u, 'x');

    std::string body;
    for (int i = 0; i < 12; i++) {
        char name[32];
        snprintf(name, sizeof(name), "feld%d", i);
        body += build_field_part(name, gross);
    }
    body += std::string("--") + BOUNDARY + "--\r\n";

    testing::internal::CaptureStdout();
    ASSERT_EQ(feed(body, 8192), 1);
    std::string log = testing::internal::GetCapturedStdout();

    /* Praefix, den beide Meldungen teilen ( Feldgrenze und Gesamtsumme ) */
    const std::string marker = "POST stream: form field";

    size_t treffer = 0;
    size_t p = 0;
    while ((p = log.find(marker, p)) != std::string::npos) {
        treffer++;
        p += marker.size();
    }

    EXPECT_LE(treffer, 1u)
        << "hoechstens eine Meldung pro Request, waren aber " << treffer;
}

/*
 * Ein Formularfeld muss linear gepuffert werden, nicht quadratisch.
 *
 * emitPartData() allokierte pro Chunk neu und kopierte den bisherigen Wert
 * dabei komplett um. Das faellt nicht auf, solange die Chunks gross sind - der
 * Boundary Scanner gibt die Daten aber byteweise heraus, sobald sie wie ein
 * Musteranfang aussehen ( needle[0] ist '\r' ). Ein Feldwert aus lauter '\r'
 * erzwingt das fuer jedes Byte: aus 60 KiB Eingabe werden dann 60000
 * Allokationen mit im Mittel 30 KiB Kopie. Gemessen auf dem Entwicklungsrechner
 * 27 ms allein fuer die Pufferarbeit gegenueber 0,03 ms - auf dem Geraet mit
 * langsamerem Speicher entsprechend mehr, und beliebig wiederholbar.
 */
TEST_F(PostStreamingTest, FieldValueOfBoundaryPrefixesIsNotQuadratic) {
    const size_t N = 60u * 1024u;

    std::string harmlos(N, 'x');
    std::string boesartig(N, '\r');

    auto lauf = [&](const std::string& wert) -> double {
        TearDown();
        SetUp();
        std::string body = build_field_part("feld", wert);
        body += std::string("--") + BOUNDARY + "--\r\n";

        struct timespec a, b;
        clock_gettime(CLOCK_MONOTONIC, &a);
        int ret = feed(body, 8192);
        clock_gettime(CLOCK_MONOTONIC, &b);
        EXPECT_EQ(ret, 1);
        return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6;
    };

    double t_harmlos  = lauf(harmlos);
    double t_boesartig = lauf(boesartig);

    /* Grosszuegig: der byteweise Pfad ist auch linear teurer ( memmove im
     * Rueckhalte Puffer, ein Aufruf pro Byte ). Quadratisch waren es gemessen
     * mehr als das Tausendfache. */
    double schranke = (t_harmlos > 0.05 ? t_harmlos : 0.05) * 100.0;
    EXPECT_LT(t_boesartig, schranke)
        << "byteweise " << t_boesartig << " ms gegen " << t_harmlos
        << " ms - das riecht nach quadratischem Puffern";
}

/*
 * Und der Wert muss dabei unveraendert ankommen - die Kapazitaetslogik ist
 * genau die Stelle, an der ein Off-by-one den Wert abschneiden wuerde.
 */
TEST_F(PostStreamingTest, FieldValueOfBoundaryPrefixesArrivesComplete) {
    std::string wert;
    for (int i = 0; i < 2000; i++) wert += "\r\n-x";   /* sieht wie ein Boundary Anfang aus */

    std::string body = build_field_part("feld", wert);
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 512), 1);

    const post_stream_part* feld = findPart(sock, "feld");
    ASSERT_NE(feld, (const post_stream_part*)NULL);
    EXPECT_EQ(feld->length, wert.size())
        << "der Feldwert muss vollstaendig gepuffert werden";
    ASSERT_NE(feld->data, (const char*)NULL);
    EXPECT_EQ(std::string(feld->data, (size_t)feld->length), wert)
        << "und unveraendert sein";
}

/*
 * Der Name eines Parts kommt aus dem Part Header und ist nur durch dessen
 * 8 KiB Grenze beschraenkt. Alle Namen bleiben bis zum Ende des Requests
 * liegen: bei POST_STREAM_MAX_PARTS Parts sind das ueber 500 KiB, die am
 * Gesamtbudget max_post_memory_total vorbei Speicher binden - auf einem Geraet,
 * bei dem genau dieser Speicher knapp ist.
 */
TEST_F(PostStreamingTest, OverlongPartNameIsCapped) {
    std::string langer_name(6000, 'n');

    std::string body = build_file_part(langer_name + ".bin", "inhalt");
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 1024), 1);

    std::vector<upload_entry> uploads = collectUploads(sock);
    ASSERT_EQ(uploads.size(), 1u);
    EXPECT_LE(uploads[0].name.size(), 256u)
        << "der Name muss begrenzt werden, war " << uploads[0].name.size();
}

/*
 * getFileName()/getFileData()/getFileSize() liefern Dateien - keine
 * Formularfelder. Das sagt schon der Name der Funktionen.
 *
 * Landen Felder in derselben Liste, kann ein Konsument beides nicht
 * auseinanderhalten: ein Request ganz ohne Datei liefert dann trotzdem einen
 * Namen an Index 0. Ein Handler, der nur prueft ob ueberhaupt etwas da ist -
 * und genau so prueft der Firmware Upload - haelt den Upload damit
 * faelschlich fuer vorhanden und arbeitet mit einer Datei weiter, die aus
 * einem frueheren Request stammt.
 */
TEST_F(PostStreamingTest, FormFieldsAreNoUploadedFiles) {
    std::string body;
    body += build_field_part("action", "upload_file");
    body += build_field_part("kommentar", "nur ein Feld");
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 32), 1);

    EXPECT_EQ(collectUploads(sock).size(), 0u)
        << "ein Request ohne Datei darf keine Datei melden";
}

/*
 * Mit Datei: gemeldet wird genau die Datei, nicht die Felder daneben - und
 * zwar an Index 0, damit getFileName(s,0) den Upload trifft und nicht ein
 * Formularfeld, das zufaellig vor ihm im Body stand.
 */
TEST_F(PostStreamingTest, OnlyTheFileIsReportedAsUpload) {
    const std::string payload = "firmware inhalt";

    std::string body;
    body += build_field_part("action", "upload_file");
    body += build_file_part("container.bbs", payload);
    body += build_field_part("kommentar", "dahinter");
    body += std::string("--") + BOUNDARY + "--\r\n";

    ASSERT_EQ(feed(body, 16), 1);

    std::vector<upload_entry> uploads = collectUploads(sock);

    ASSERT_EQ(uploads.size(), 1u) << "nur die Datei gehoert in upload_files";
    EXPECT_EQ(uploads[0].name, "container.bbs");
    EXPECT_EQ(uploads[0].length, payload.size());
}

/*
 * Die Pruefung des Consumers entscheidet, nicht die Session.
 *
 * Vorher kannte der Streaming Pfad nur die angemeldete Session. Ein Client,
 * der sich per auth_user/auth_pass in der URL oder per Basic Authorization
 * anmeldet, hat aber bewusst keine Session - save_request.c markiert sie bei
 * URL Auth ausdruecklich nicht als eingeloggt. Sein Upload wurde deshalb
 * verworfen, obwohl er berechtigt war, und der Handler meldete anschliessend
 * "keine Datei". Genau so ist der Autotester gestolpert.
 *
 * Und nur der Consumer weiss, welches Recht sein Upload verlangt - deshalb
 * gibt er die Pruefung bei der Registrierung mit.
 */
TEST_F(PostStreamingTest, AuthCheckDecidesInsteadOfSession) {

    EXPECT_EQ(postStreamRequestAuthorized(sock, auth_allow), 1)
        << "die Pruefung des Consumers muss den Ausschlag geben";

    EXPECT_EQ(postStreamRequestAuthorized(sock, auth_deny), 0);
}

/*
 * Die Pruefung bekommt den Request mit Header und URL Parametern - sonst kann
 * sie auth_user/auth_pass gar nicht sehen.
 */
TEST_F(PostStreamingTest, AuthCheckSeesTheRequest) {
    static const char* gesehene_url;
    gesehene_url = NULL;

    auto merker = [](dummy_handler* s) -> int {
        http_request* r = (http_request*)s;
        gesehene_url = (r && r->header) ? r->header->url : NULL;
        return 1;
    };

    EXPECT_EQ(postStreamRequestAuthorized(sock, merker), 1);
    ASSERT_NE(gesehene_url, (const char*)NULL) << "die Pruefung braucht den Header";
    EXPECT_STREQ(gesehene_url, TEST_URL);
}
