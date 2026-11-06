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

/*
 * The config store is a file scope static in server_config.c as well. The
 * fixtures used to create it themselves: one created it once and never
 * freed it, the next replaced it (leaking the first one), and
 * ServerConfigTest freed it after every test, leaving the next fixture a
 * dead store. Now it exists once for the whole run; ServerConfigTest
 * replaces it instead of dropping it.
 */
class ConfigStoreEnvironment : public ::testing::Environment {
public:
    void SetUp() override { initConfig(); }
    void TearDown() override { freeConfig(); }
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new SessionStoreEnvironment);
    ::testing::AddGlobalTestEnvironment(new ConfigStoreEnvironment);
    return RUN_ALL_TESTS();
}
