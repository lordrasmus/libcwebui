#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "webserver.h"
}

/*
 * Session stores are kept in a red black tree keyed by the GUID, and every
 * pass over that tree asks for the range "0" to "z":
 *
 *     stack = RBEnumerate(session_store_tree, (void*)"0", (void*)"z");
 *
 * checkSessionTimeout() uses it to find expired entries, dumpSessionsSize()
 * to count them. Generated GUIDs are hex with dashes and start with a digit
 * or an uppercase letter, so they fall inside that range.
 *
 * The key does not have to be generated, though: checkSessionCookie() copies
 * whatever arrives in the session-id cookie, and createSession() inserts a
 * store under exactly that value. A key sorting outside "0".."z" - anything
 * starting with '-', '!', '{' and so on - is therefore invisible to both
 * passes: it is never counted, and it is never removed on timeout.
 *
 * These tests measure both halves against the real tree.
 */
class SessionRangeTest : public ::testing::Test {
protected:
    http_request req;
    socket_info sock;
    HttpRequestHeader *header;

    void SetUp() override {
        // The session tree is created once for the whole run, see
        // test_main.cpp. Counts are therefore taken relative to "before".
        memset(&req, 0, sizeof(req));
        memset(&sock, 0, sizeof(sock));
        header = WebserverMallocHttpRequestHeader();
        sock.header = header;
        req.socket = &sock;
    }

    void TearDown() override {
        WebserverFreeHttpRequestHeader(header);
    }

    // Create a session store under a caller chosen key, the way a request
    // with a session-id cookie does.
    void createWithGuid(const char *guid) {
        memset(req.guid, 0, sizeof(req.guid));
        snprintf((char*)req.guid, sizeof(req.guid), "%s", guid);
        createSession(&req, 0);
    }

    int sessionCount() {
        int count = 0;
        dumpSessionsSize(&count);
        return count;
    }
};

TEST_F(SessionRangeTest, GeneratedKeysAreCounted) {
    int before = sessionCount();

    // Shaped like generateGUID(): uppercase hex with dashes
    createWithGuid("8D956560-201C-451C-A4CA-0E54EE8A0CBD");

    EXPECT_EQ(sessionCount(), before + 1)
        << "A generated GUID sorts inside \"0\"..\"z\" and has to show up.";
}

TEST_F(SessionRangeTest, KeysOutsideTheRangeAreInvisible) {
    int before = sessionCount();

    // The key comes from the cookie and is used unchanged. A leading '-'
    // sorts before "0" and thus outside the enumerated range.
    createWithGuid("-8D956560-201C-451C-A4CA-0E54EE8A0CBD");

    EXPECT_EQ(sessionCount(), before + 1)
        << "The store exists but is not enumerated: dumpSessionsSize misses "
           "it, and so does checkSessionTimeout - the entry is never freed.";
}

TEST_F(SessionRangeTest, KeysAboveTheRangeAreInvisible) {
    int before = sessionCount();

    // '{' sorts after 'z'
    createWithGuid("{8D956560-201C-451C-A4CA-0E54EE8A0CBD");

    EXPECT_EQ(sessionCount(), before + 1)
        << "Same on the upper end of the range.";
}
