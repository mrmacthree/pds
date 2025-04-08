#include "hash.hpp"

#include <gtest/gtest.h>

#include <random>

using namespace pds::hash;

// Mock HashFunction for testing
struct MockHashFunction {
    using seed_type = uint64_t;
    using hash_type = uint64_t;
    using key_type = std::string;

    hash_type operator()(const std::string &key, seed_type seed) const {
        return std::hash<std::string>{}(key) ^ seed;
    }
};

TEST(HashFunctionTest, CorrectConceptTest) {  
    // This test checks if the concept is satisfied when the hash function
    // has the correct types and operator.
    constexpr bool correct = HashFunction<MockHashFunction, std::string>;
    EXPECT_EQ(correct, true);
}
TEST(HashFunctionTest, KeyTypeTest) {
    // This test checks if the concept fails when the key_type does not match
    // the concept Key type.
    constexpr bool key_type = HashFunction<MockHashFunction, int>;
    EXPECT_EQ(key_type, false);
}
TEST(HashFunctionTest, NoKeyTypeTest) {
    // This test checks if the concept fails when the seed_type is not defined
    // in the hash function.
    struct MockHashFunction_no_key_type {
        using seed_type = uint64_t;
        using hash_type = uint64_t;
    
        hash_type operator()(const std::string &key, uint64_t seed) const {
            return std::hash<std::string>{}(key) ^ seed;
        }
    };
    constexpr bool key_type = HashFunction<MockHashFunction_no_key_type, std::string>;
    EXPECT_EQ(key_type, false);
}
TEST(HashFunctionTest, InconsistentKeyTypeTest) {
    // This test checks if the concept fails when the key_type is not the same
    // as the type of the first argument of the operator().
    struct MockHashFunction_inconsistent_key_type {
        using seed_type = uint64_t;
        using hash_type = uint64_t;
        using key_type = std::string;

        hash_type operator()(const int &key, seed_type seed) const {
            return key ^ seed;
        }
    };
    constexpr bool consistent_key_type = HashFunction<MockHashFunction_inconsistent_key_type, std::string>;
    EXPECT_EQ(consistent_key_type, false);
}
TEST(HashFunctionTest, NoSeedTypeTest) {
    // This test checks if the concept fails when the seed_type is not defined
    // in the hash function.
    struct MockHashFunction_no_seed_type {
        using hash_type = uint64_t;
        using key_type = std::string;
    
        hash_type operator()(const std::string &key, uint64_t seed) const {
            return std::hash<std::string>{}(key) ^ seed;
        }
    };
    constexpr bool seed_type = HashFunction<MockHashFunction_no_seed_type, std::string>;
    EXPECT_EQ(seed_type, false);
}
TEST(HashFunctionTest, InconsistentSeedTypeTest) {
    // This test checks if the concept fails when the seed_type is not the same
    // as the type of the second argument of the operator().
    struct MockHashFunction_inconsistent_seed_type {
        using seed_type = uint64_t;
        using hash_type = uint64_t;
        using key_type = std::string;
    
        hash_type operator()(const std::string &key, std::string seed) const {
            return std::hash<std::string>{}(key) ^ std::hash<std::string>{}(seed);
        }
    };
    constexpr bool consistent_seed_type = HashFunction<MockHashFunction_inconsistent_seed_type, std::string>;
    EXPECT_EQ(consistent_seed_type, false);
}
TEST(HashFunctionTest, NoHashTypeTest) {
    // This test checks if the concept fails when the hash_type is not defined
    // in the hash function.
    struct MockHashFunctio_no_hash_type {
        using seed_type = uint64_t;
        using key_type = std::string;
    
        uint64_t operator()(const std::string &key, seed_type seed) const {
            return std::hash<std::string>{}(key) ^ seed;
        }
    };
    constexpr bool hash_type = HashFunction<MockHashFunctio_no_hash_type, std::string>;
    EXPECT_EQ(hash_type, false);
}
TEST(HashFunctionTest, InconsistentHashTypeTest) {
    // This test checks if the concept fails when the hash_type is not the same
    // as the return type of the operator().
    struct MockHashFunction_inconsistent_hash_type {
        using seed_type = uint64_t;
        using hash_type = uint64_t;
        using key_type = std::string;
    
        uint32_t operator()(const std::string &key, seed_type seed) const {
            return std::hash<std::string>{}(key) ^ seed;
        }
    };
    constexpr bool consistent_hash_type = HashFunction<MockHashFunction_inconsistent_hash_type, std::string>;
    EXPECT_EQ(consistent_hash_type, false);
}
TEST(HashFunctionTest, NoOperatorTest) {
    // This test checks if the concept fails when the operator() is not defined
    // in the hash function.
    struct MockHashFunction_no_operator {
        using seed_type = uint64_t;
        using hash_type = uint64_t;
        using key_type = std::string;
    
    };
    constexpr bool op = HashFunction<MockHashFunction_no_operator, std::string>;
    EXPECT_EQ(op, false);
}

struct MockHashGenerator {
    using hash_type = uint64_t;
    using key_type = std::string;

    MockHashGenerator(size_t hashes_per_key, size_t range = std::numeric_limits<size_t>::max())
        : hashes_per_key_(hashes_per_key), range_(range) {}

    auto hashes(const std::string &key) const {
        auto res = std::vector<hash_type>(hashes_per_key_);
        std::iota(res.begin(), res.end(), 0);
        return res;
    }

    size_t hashes_per_key() const { return hashes_per_key_; }
    size_t range() const { return range_; }

    size_t hashes_per_key_;
    size_t range_;
};

TEST(HashGeneratorTest, CorrectConceptTest) {
    constexpr bool correct = HashGenerator<MockHashGenerator, std::string>;
    EXPECT_EQ(correct, true);
}

// Test simple_hash_generator initialization
TEST(SimpleHashGeneratorTest, Initialization) {
    simple_hash_generator<std::string, MockHashFunction> generator(5);
    EXPECT_EQ(generator.hashes_per_key(), 5);
}

// Test hash generation output
TEST(SimpleHashGeneratorTest, HashOutput) {
    simple_hash_generator<std::string, MockHashFunction> generator(3);
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
    simple_hash_generator<std::string, MockHashFunction> generator(3);
    std::string key = "";
    auto hashes = generator.hashes(key);
    EXPECT_EQ(std::ranges::size(hashes), 3);
}

// Edge case: Zero hashes per key
TEST(SimpleHashGeneratorTest, ZeroHashesPerKey) {
    simple_hash_generator<std::string, MockHashFunction> generator(0);
    std::string key = "any_key";
    auto hashes = generator.hashes(key);
    EXPECT_EQ(std::ranges::size(hashes), 0);
}

// Edge case: Long key
TEST(SimpleHashGeneratorTest, LongKey) {
    simple_hash_generator<std::string, MockHashFunction> generator(5);
    std::string long_key(1000, 'a');
    auto hashes = generator.hashes(long_key);
    EXPECT_EQ(std::ranges::size(hashes), 5);
}

// Performance test: Large number of hashes
TEST(SimpleHashGeneratorTest, LargeNumberOfHashes) {
    simple_hash_generator<std::string, MockHashFunction> generator(100000);
    std::string key = "performance_key";
    auto hashes = generator.hashes(key);
    EXPECT_EQ(std::ranges::size(hashes), 100000);
}

// Stress test: Large number of unique keys
TEST(SimpleHashGeneratorTest, ManyUniqueKeys) {
    simple_hash_generator<std::string, MockHashFunction> generator(3);
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
    simple_hash_generator<std::string, MockHashFunction> generator(10);
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
    simple_hash_generator<std::string, MockHashFunction> generator(5);
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
