#include <gtest/gtest.h>
#include <cstring>
#include <climits>

extern "C" {
#include "webserver.h"
}

/*
 * Overflow in the size arithmetic of the memory manager.
 *
 * real_WebserverMalloc() takes the size as unsigned long but carries on in
 * unsigned int:
 *
 *     unsigned int add_size = 0, real_alloc;
 *     ...
 *     real_alloc = size + add_size;
 *
 * With a size near the upper bound the sum wraps to a tiny value and the
 * allocation succeeds - handing back a block orders of magnitude smaller
 * than requested. The caller believes it owns a huge buffer, and the next
 * write runs past the end of it.
 *
 * Reachable from parsePostData(): the length of a multipart section is
 * computed as (offset3 - offset2) - 1, both uint64_t. An empty section makes
 * the difference 0, and that turns into the upper bound - see
 * test_post_multipart.cpp for that half of the chain.
 */
TEST(MemoryOverflowTest, HugeSizeMustNotYieldATinyBlock) {
    // Exactly the value produced by (0 - 1)
    unsigned long huge = (unsigned long)0 - 1;

    // A death test, because there is nothing to return: every caller of
    // WebserverMalloc assumes a valid pointer, so the function ends the
    // process - as it already did when the underlying malloc failed. What
    // matters is that it happens *before* the allocation, with a message,
    // instead of handing out a tiny block that the caller then writes past.
    EXPECT_EXIT(WebserverMalloc(huge),
                ::testing::ExitedWithCode(1),
                "refusing");
}

TEST(MemoryOverflowTest, OrdinarySizesStillWork) {
    // Counter check - without it the test above would also pass if every
    // allocation ended the process.
    void *p = WebserverMalloc(1024);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(WebserverMallocedSize(p), 1024u);
    memset(p, 0xAB, 1024);      // ASan checks the block really has that size
    WebserverFree(p);
}
