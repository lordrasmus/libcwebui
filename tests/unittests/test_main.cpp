#include <gtest/gtest.h>

extern "C" {
#include "webserver.h"
}

/*
 * The session store is a file scope static in session.c, shared by every
 * test that touches sessions. It is created here once for the whole run:
 * a fixture calling initSessions() itself replaces the tree another fixture
 * already created, and the first one leaks.
 */
class SessionStoreEnvironment : public ::testing::Environment {
public:
    void SetUp() override { initSessions(); }
    void TearDown() override { freeSessions(); }
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new SessionStoreEnvironment);
    return RUN_ALL_TESTS();
}
