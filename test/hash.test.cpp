#include "hash.hpp"

#include <gtest/gtest.h>

#include <random>

using namespace pds::hash;

// Mock hash function for testing
struct MockHash {
    using seed_type = uint32_t;
    using hash_type = uint64_t;

    hash_type operator()(const std::string &key, seed_type seed) const {
        return std::hash<std::string>{}(key) ^ seed;
    }
};

TEST(HashFunctionTest, one) {
    // static_assert(HashFunction<MockHash, std::string>, "Nope");
    constexpr bool x = HashFunction<MockHash, std::string>;
    EXPECT_EQ(x, true);
    constexpr bool y = HashFunction<MockHash, int>;
    EXPECT_EQ(y, false);
}

// Test simple_hash_generator initialization
TEST(SimpleHashGeneratorTest, Initialization) {
    simple_hash_generator<std::string, MockHash> generator(5);
    EXPECT_EQ(generator.hashes_per_key(), 5);
}

// Test hash generation output
TEST(SimpleHashGeneratorTest, HashOutput) {
    simple_hash_generator<std::string, MockHash> generator(3);
    std::string key = "any_key";

    auto hashes = generator.hashes(key);
    auto hash_results = std::vector<uint64_t>{};
    std::ranges::copy(hashes, std::back_inserter(hash_results));
    // for (auto hash : hashes) {
    //     hash_results.push_back(hash);
    // }

    EXPECT_EQ(hash_results.size(), 3);
    EXPECT_NE(hash_results[0], hash_results[1]);
    EXPECT_NE(hash_results[1], hash_results[2]);
}

// Edge case: Empty key
TEST(SimpleHashGeneratorTest, EmptyKey) {
    simple_hash_generator<std::string, MockHash> generator(3);
    std::string key = "";
    auto hashes = generator.hashes(key);
    EXPECT_EQ(std::ranges::size(hashes), 3);
}

// Edge case: Zero hashes per key
TEST(SimpleHashGeneratorTest, ZeroHashesPerKey) {
    simple_hash_generator<std::string, MockHash> generator(0);
    std::string key = "any_key";
    auto hashes = generator.hashes(key);
    EXPECT_EQ(std::ranges::size(hashes), 0);
}

// Edge case: Long key
TEST(SimpleHashGeneratorTest, LongKey) {
    simple_hash_generator<std::string, MockHash> generator(5);
    std::string long_key(1000, 'a');
    auto hashes = generator.hashes(long_key);
    EXPECT_EQ(std::ranges::size(hashes), 5);
}

// Performance test: Large number of hashes
TEST(SimpleHashGeneratorTest, LargeNumberOfHashes) {
    simple_hash_generator<std::string, MockHash> generator(100000);
    std::string key = "performance_key";
    auto hashes = generator.hashes(key);
    EXPECT_EQ(std::ranges::size(hashes), 100000);
}

// Stress test: Large number of unique keys
TEST(SimpleHashGeneratorTest, ManyUniqueKeys) {
    simple_hash_generator<std::string, MockHash> generator(3);
    std::unordered_set<uint64_t> all_hashes;
    for (int i = 0; i < 10000; ++i) {
        std::string key = "key_" + std::to_string(i);
        auto hashes = generator.hashes(key);
        for (auto h : hashes) {
            all_hashes.insert(h);
        }
    }
    EXPECT_EQ(all_hashes.size(), 10000 * 3);
}

// Fuzz test: Random keys
TEST(SimpleHashGeneratorTest, RandomKeysFuzzTest) {
    simple_hash_generator<std::string, MockHash> generator(10);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist('a', 'z');

    for (int i = 0; i < 1000; ++i) {
        std::string key(20, ' ');
        for (auto &c : key) c = dist(rng);
        auto hashes = generator.hashes(key);
        EXPECT_EQ(std::ranges::size(hashes), 10);
    }
}

// Collision detection test
TEST(SimpleHashGeneratorTest, CollisionDetection) {
    simple_hash_generator<std::string, MockHash> generator(5);
    std::unordered_set<uint64_t> hash_set;
    std::vector<std::string> keys = {"key1", "key2", "key3", "key4", "key5"};

    for (const auto &key : keys) {
        auto hashes = generator.hashes(key);
        for (auto h : hashes) {
            EXPECT_EQ(hash_set.count(h), 0)
                << "Collision detected for key: " << key;
            hash_set.insert(h);
        }
    }
}
