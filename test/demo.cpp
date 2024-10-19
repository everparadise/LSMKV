#include <gtest/gtest.h>
#include "kvstore.h"
TEST(KVStoreTest, simpleTestAddFunction)
{
    KVStore kvstore("./data", "./data/vlog");
    EXPECT_EQ(kvstore.add(1, 2), 3);
    EXPECT_EQ(1, 1);
}
TEST(KVStoreTest, simpleErrorAddFunction)
{
    KVStore kvstore("./data", "./data/vlog");
    EXPECT_EQ(kvstore.add(1, 2.11), 3.11);
}