#include <gtest/gtest.h>
#include <atomic>
#include <cstring>
#include <string>
#include <thread>

extern "C" {
#include "webserver.h"
}

// WebSocket Handshake Key Calculation Tests
// RFC 6455: Sec-WebSocket-Accept = Base64(SHA1(Sec-WebSocket-Key + GUID)),
// computed by websocketAcceptKey(), which startWebsocketConnection() uses
class WebSocketHandshakeTest : public ::testing::Test {};

// Test vector from RFC 6455 Section 4.2.2
TEST_F(WebSocketHandshakeTest, RFC6455TestVector) {
    unsigned char acceptKey[40];
    ASSERT_EQ(websocketAcceptKey("dGhlIHNhbXBsZSBub25jZQ==", acceptKey, sizeof(acceptKey)), 0);
    EXPECT_STREQ((char*)acceptKey, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

// The key is always 16 bytes in base64: 22 characters and "=="
TEST_F(WebSocketHandshakeTest, KeysThatAreNotSixteenBytesAreRefused) {
    unsigned char acceptKey[40];
    EXPECT_EQ(websocketAcceptKey("", acceptKey, sizeof(acceptKey)), -1);
    EXPECT_EQ(websocketAcceptKey("abc", acceptKey, sizeof(acceptKey)), -1);
    EXPECT_EQ(websocketAcceptKey("dGhlIHNhbXBsZSBub25jZQ=", acceptKey, sizeof(acceptKey)), -1);
    EXPECT_EQ(websocketAcceptKey("dGhlIHNhbXBsZSBub25jZQ==A", acceptKey, sizeof(acceptKey)), -1);
    EXPECT_EQ(websocketAcceptKey("dGhlIHNhbXBsZSBub25jZ$==", acceptKey, sizeof(acceptKey)), -1);
    EXPECT_EQ(websocketAcceptKey(NULL, acceptKey, sizeof(acceptKey)), -1);
}

// Used to be copied with strncpy(…, 100) - no terminator - and appended to
TEST_F(WebSocketHandshakeTest, OverlongKeyIsRefused) {
    std::string longKey(1000, 'A');
    unsigned char acceptKey[40];
    EXPECT_EQ(websocketAcceptKey(longKey.c_str(), acceptKey, sizeof(acceptKey)), -1);
}

/* The header releases its strings with WebserverFree() */
static char* header_string(const char* text) {
    char* p = (char*)WebserverMalloc(strlen(text) + 1);
    strcpy(p, text);
    return p;
}

static socket_info* upgrade_request(const char* key) {
    socket_info* s = WebserverMallocSocketInfo();
    s->socket = -1;
    s->header = WebserverMallocHttpRequestHeader();
    s->header->Connection = header_string("Upgrade");
    s->header->Upgrade = header_string("websocket");
    s->header->SecWebSocketKey = header_string(key);
    s->header->SecWebSocketVersion = 13;
    return s;
}

static void free_upgrade_request(socket_info* s) {
    WebserverFreeSocketInfo(s);
}

TEST_F(WebSocketHandshakeTest, UpgradeWithAnInvalidKeyGets400) {
    std::string longKey(200, 'A');
    socket_info* s = upgrade_request(longKey.c_str());
    EXPECT_EQ(checkIskWebsocketConnection(s), 1);
    EXPECT_EQ(s->header->isWebsocket, 2) << "refused, response prepared";
    EXPECT_EQ(s->header->response_status, 400);
    free_upgrade_request(s);
}

TEST_F(WebSocketHandshakeTest, UpgradeWithAValidKeyIsAccepted) {
    socket_info* s = upgrade_request("dGhlIHNhbXBsZSBub25jZQ==");
    EXPECT_EQ(checkIskWebsocketConnection(s), 1);
    EXPECT_EQ(s->header->isWebsocket, 1);
    free_upgrade_request(s);
}

// WebSocket Frame Length Encoding Tests
// Tests for the frame header length encoding logic
class WebSocketFrameLengthTest : public ::testing::Test {};

TEST_F(WebSocketFrameLengthTest, SmallPayload) {
    // Payload < 126 bytes uses 1-byte length
    unsigned char frame[2];
    uint32_t length = 100;

    frame[0] = 0x81;  // FIN + Text opcode
    frame[1] = (unsigned char)length;

    EXPECT_EQ(frame[1], 100);
}

TEST_F(WebSocketFrameLengthTest, MediumPayload) {
    // Payload 126-65535 bytes uses 2-byte extended length
    unsigned char frame[4];
    uint32_t length = 1000;

    frame[0] = 0x81;  // FIN + Text opcode
    frame[1] = 126;   // Extended length marker
    frame[2] = (unsigned char)(length >> 8);
    frame[3] = (unsigned char)(length >> 0);

    uint32_t decoded = ((uint32_t)frame[2] << 8) | frame[3];
    EXPECT_EQ(decoded, 1000u);
}

TEST_F(WebSocketFrameLengthTest, LargePayload) {
    // Payload > 65535 bytes uses 8-byte extended length
    unsigned char frame[10];
    uint32_t length = 100000;

    frame[0] = 0x81;  // FIN + Text opcode
    frame[1] = 127;   // 8-byte extended length marker
    frame[2] = 0;     // Upper 4 bytes (0 for 32-bit length)
    frame[3] = 0;
    frame[4] = 0;
    frame[5] = 0;
    frame[6] = (unsigned char)(length >> 24);
    frame[7] = (unsigned char)(length >> 16);
    frame[8] = (unsigned char)(length >> 8);
    frame[9] = (unsigned char)(length >> 0);

    uint32_t decoded = ((uint32_t)frame[6] << 24) |
                       ((uint32_t)frame[7] << 16) |
                       ((uint32_t)frame[8] << 8) |
                       frame[9];
    EXPECT_EQ(decoded, 100000u);
}

TEST_F(WebSocketFrameLengthTest, BoundaryAt125) {
    // 125 should use 1-byte length
    uint32_t length = 125;
    unsigned char lengthByte = (length < 126) ? (unsigned char)length : 126;
    EXPECT_EQ(lengthByte, 125);
}

TEST_F(WebSocketFrameLengthTest, BoundaryAt126) {
    // 126 should trigger 2-byte extended length
    uint32_t length = 126;
    unsigned char lengthByte = (length < 126) ? (unsigned char)length : 126;
    EXPECT_EQ(lengthByte, 126);
}

TEST_F(WebSocketFrameLengthTest, BoundaryAt65535) {
    // 65535 should use 2-byte extended length
    uint32_t length = 65535;
    bool use8Byte = (length >= 65536);
    EXPECT_FALSE(use8Byte);
}

TEST_F(WebSocketFrameLengthTest, BoundaryAt65536) {
    // 65536 should trigger 8-byte extended length
    uint32_t length = 65536;
    bool use8Byte = (length >= 65536);
    EXPECT_TRUE(use8Byte);
}

// WebSocket Masking Tests
class WebSocketMaskingTest : public ::testing::Test {};

TEST_F(WebSocketMaskingTest, MaskUnmask) {
    // XOR masking is symmetric: mask(mask(data)) = data
    unsigned char mask[4] = {0x37, 0xfa, 0x21, 0x3d};
    unsigned char original[] = "Hello";
    unsigned char data[6];
    memcpy(data, original, 6);

    // Mask
    for (int i = 0; i < 5; i++) {
        data[i] ^= mask[i % 4];
    }

    // Data should be different now
    EXPECT_NE(memcmp(data, original, 5), 0);

    // Unmask
    for (int i = 0; i < 5; i++) {
        data[i] ^= mask[i % 4];
    }

    // Data should be back to original
    EXPECT_EQ(memcmp(data, original, 5), 0);
}

TEST_F(WebSocketMaskingTest, MaskWithZero) {
    // Zero mask should leave data unchanged
    unsigned char mask[4] = {0x00, 0x00, 0x00, 0x00};
    unsigned char data[] = "Test";
    unsigned char original[] = "Test";

    for (int i = 0; i < 4; i++) {
        data[i] ^= mask[i % 4];
    }

    EXPECT_EQ(memcmp(data, original, 4), 0);
}

TEST_F(WebSocketMaskingTest, MaskLongData) {
    // Test mask cycling through longer data
    unsigned char mask[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    unsigned char data[100];
    unsigned char original[100];

    for (int i = 0; i < 100; i++) {
        original[i] = (unsigned char)i;
        data[i] = (unsigned char)i;
    }

    // Mask
    for (int i = 0; i < 100; i++) {
        data[i] ^= mask[i % 4];
    }

    // Unmask
    for (int i = 0; i < 100; i++) {
        data[i] ^= mask[i % 4];
    }

    EXPECT_EQ(memcmp(data, original, 100), 0);
}

// WebSocket Opcode Tests
class WebSocketOpcodeTest : public ::testing::Test {};

TEST_F(WebSocketOpcodeTest, TextOpcode) {
    unsigned char byte = 0x81;  // FIN + Text
    int fin = (byte & 0x80) ? 1 : 0;
    int opcode = byte & 0x0F;

    EXPECT_EQ(fin, 1);
    EXPECT_EQ(opcode, 0x1);  // WSF_TEXT
}

TEST_F(WebSocketOpcodeTest, BinaryOpcode) {
    unsigned char byte = 0x82;  // FIN + Binary
    int opcode = byte & 0x0F;

    EXPECT_EQ(opcode, 0x2);  // WSF_BINARY
}

TEST_F(WebSocketOpcodeTest, CloseOpcode) {
    unsigned char byte = 0x88;  // FIN + Close
    int opcode = byte & 0x0F;

    EXPECT_EQ(opcode, 0x8);  // WSF_CLOSE
}

TEST_F(WebSocketOpcodeTest, PingOpcode) {
    unsigned char byte = 0x89;  // FIN + Ping
    int opcode = byte & 0x0F;

    EXPECT_EQ(opcode, 0x9);  // WSF_PING
}

TEST_F(WebSocketOpcodeTest, PongOpcode) {
    unsigned char byte = 0x8A;  // FIN + Pong
    int opcode = byte & 0x0F;

    EXPECT_EQ(opcode, 0xA);  // WSF_PONG
}

TEST_F(WebSocketOpcodeTest, ContinuationOpcode) {
    unsigned char byte = 0x00;  // No FIN + Continuation
    int fin = (byte & 0x80) ? 1 : 0;
    int opcode = byte & 0x0F;

    EXPECT_EQ(fin, 0);
    EXPECT_EQ(opcode, 0x0);  // WSF_CONTINUE
}

TEST_F(WebSocketOpcodeTest, FragmentedTextStart) {
    unsigned char byte = 0x01;  // No FIN + Text (start of fragmented message)
    int fin = (byte & 0x80) ? 1 : 0;
    int opcode = byte & 0x0F;

    EXPECT_EQ(fin, 0);
    EXPECT_EQ(opcode, 0x1);  // WSF_TEXT
}

TEST_F(WebSocketOpcodeTest, ReservedBits) {
    // RSV1, RSV2, RSV3 should be 0 in normal frames
    unsigned char byte = 0x81;  // Normal frame
    int rsv = (byte & 0x70);

    EXPECT_EQ(rsv, 0);
}

TEST_F(WebSocketOpcodeTest, ReservedBitsSet) {
    // If RSV bits are set, it indicates extension use
    unsigned char byte = 0xC1;  // RSV1 set
    int rsv1 = (byte & 0x40) ? 1 : 0;
    int rsv2 = (byte & 0x20) ? 1 : 0;
    int rsv3 = (byte & 0x10) ? 1 : 0;

    EXPECT_EQ(rsv1, 1);
    EXPECT_EQ(rsv2, 0);
    EXPECT_EQ(rsv3, 0);
}

// WebSocket Close Status Code Tests
class WebSocketCloseCodeTest : public ::testing::Test {};

TEST_F(WebSocketCloseCodeTest, NormalClosure) {
    uint16_t status = 1000;
    unsigned char payload[2];
    payload[0] = (status >> 8) & 0xFF;
    payload[1] = status & 0xFF;

    uint16_t decoded = ((uint16_t)payload[0] << 8) | payload[1];
    EXPECT_EQ(decoded, 1000);
}

TEST_F(WebSocketCloseCodeTest, ProtocolError) {
    uint16_t status = 1002;
    unsigned char payload[2];
    payload[0] = (status >> 8) & 0xFF;
    payload[1] = status & 0xFF;

    uint16_t decoded = ((uint16_t)payload[0] << 8) | payload[1];
    EXPECT_EQ(decoded, 1002);
}

TEST_F(WebSocketCloseCodeTest, InvalidUTF8) {
    uint16_t status = 1007;
    unsigned char payload[2];
    payload[0] = (status >> 8) & 0xFF;
    payload[1] = status & 0xFF;

    uint16_t decoded = ((uint16_t)payload[0] << 8) | payload[1];
    EXPECT_EQ(decoded, 1007);
}

TEST_F(WebSocketCloseCodeTest, MessageTooBig) {
    uint16_t status = 1009;
    unsigned char payload[2];
    payload[0] = (status >> 8) & 0xFF;
    payload[1] = status & 0xFF;

    uint16_t decoded = ((uint16_t)payload[0] << 8) | payload[1];
    EXPECT_EQ(decoded, 1009);
}

/*
 * The handlers of a URL are called from two threads: CONNECT from the event
 * loop, messages and DISCONNECT from the websocket input thread. Walking the
 * handler list used the iterator kept in the list itself, shared by both.
 */
extern "C" hashmap_t* websocket_handler_map;

static std::atomic<long> handler_calls[3];

static void count_a(WEBSOCKET_SIGNALS, const char*, const char, const char*, const unsigned long long) { handler_calls[0]++; }
static void count_b(WEBSOCKET_SIGNALS, const char*, const char, const char*, const unsigned long long) { handler_calls[1]++; }
static void count_c(WEBSOCKET_SIGNALS, const char*, const char, const char*, const unsigned long long) { handler_calls[2]++; }

TEST(WebsocketHandlerDispatchTest, TwoThreadsAtOnceCallEveryHandlerEveryTime) {
    if (websocket_handler_map == NULL) {
        websocket_handler_map = hashmap_create();
    }
    register_function_websocket_handler("dispatch-race", count_a, __FILE__, __LINE__);
    register_function_websocket_handler("dispatch-race", count_b, __FILE__, __LINE__);
    register_function_websocket_handler("dispatch-race", count_c, __FILE__, __LINE__);

    const long rounds = 200000;
    std::thread loop([&] {          /* like the event loop: connects */
        for (long i = 0; i < rounds; i++) {
            handleWebsocketConnection(WEBSOCKET_SIGNAL_CONNECT, "g1", "dispatch-race", 0, 0, 0);
        }
    });
    std::thread input([&] {         /* like the input thread: messages */
        for (long i = 0; i < rounds; i++) {
            handleWebsocketConnection(WEBSOCKET_SIGNAL_MSG, "g2", "dispatch-race", 0, "x", 1);
        }
    });
    loop.join();
    input.join();

    for (int h = 0; h < 3; h++) {
        EXPECT_EQ(handler_calls[h].load(), 2 * rounds) << "handler " << h;
    }
}
