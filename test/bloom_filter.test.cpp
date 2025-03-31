#include "bloom_filter.hpp"

#include <gtest/gtest.h>

TEST(bloom_filter, CorrectSize) {
    pds::bloom_filter<int> bf(100, (size_t)4);
    bf.insert(2);
    EXPECT_EQ(bf.contains(2), true);
}
