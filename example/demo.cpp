#include <limits>
#include <random>
#include <iostream>
#include <sul/dynamic_bitset.hpp>
#include <vector>

#include "bloom_filter.hpp"
#include "hash.hpp"
struct MockHash {
    using seed_type = uint32_t;
    using hash_type = uint64_t;
    using key_type = std::string;

    hash_type operator()(const std::string& key, seed_type seed) const {
        return std::hash<std::string>{}(key) ^ seed;
    }
};

int main() {
    auto engine = std::default_random_engine{std::random_device{}()};
    auto dist = std::uniform_int_distribution<>{
        std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
    constexpr size_t NUM = 100;

    pds::hash::simple_hash_generator<std::string, MockHash> generator(3);
    std::string key = "test";

    auto hashes = generator.hashes(key);
    // auto vec = std::vector<uint64_t>{};
    // std::ranges::copy(hashes, std::back_inserter(vec));
    // std::vector<uint64_t> hash_results(std::ranges::begin(hashes),
    // std::ranges::end(hashes));

    std::vector<int> v(NUM);
    pds::bloom_filter<int> bf(NUM, 0.01);
    for (auto& x : v) {
        x = dist(engine);
        bf.insert(x);
    }
    int count = 0;
    for (size_t i = 0; i < NUM; i++) {
        if (!bf.contains(v[i])) count++;
    }
    std::cout << count << std::endl;
    count = 0;
    pds::bloom_filter<int> bf2(1000, (size_t)4);
    for (int i = 0; i < 5 * NUM; i++) {
        bf2.insert(i * 2);
    }
    for (int i = 0; i < 5 * NUM; i++) {
        if (bf2.contains(2 * i + 1)) count++;
    }
    for (int i = 5 * NUM; i < 10 * NUM; i++) {
        if (bf2.contains(i)) count++;
    }
    std::cout << "h " << count / 1000.0 << std::endl;
    std::cout << bf2.num_set_bits() << std::endl;

    pds::bloom_filter<int> nbf(1000, (size_t)4);
    for (auto x : v) {
        nbf.insert(x);
    }
    count = 0;
    for (size_t i = 0; i < NUM; i++) {
        if (!nbf.contains(v[i])) count++;
    }
    std::cout << count << std::endl;
    std::cout << nbf.num_set_bits() << std::endl;
    sul::dynamic_bitset<> bitset1(12, 0b0100010110111);
    std::cout << "bitset 1     = " << bitset1 << std::endl;
    struct Test {
        void operator()() { std::cout << "test\n"; }
    };
    struct Test2 {
        Test t;
        void dothis() {
            t = Test{};
            t();
            std::cout << "dothis\n";
        }
    };
    Test{}();
    Test2 t{};
    t.dothis();
    std::cout << sizeof(Test) << std::endl;
    std::cout << sizeof(Test2) << std::endl;

    pds::bloom_filter<int> sbf(1000, (size_t)4);
    count = 0;
    for (int i = 0; i < 5 * NUM; i++) {
        sbf.insert(i * 2);
    }
    for (int i = 0; i < 5 * NUM; i++) {
        if (sbf.contains(2 * i + 1)) count++;
    }
    for (int i = 5 * NUM; i < 10 * NUM; i++) {
        if (sbf.contains(i)) count++;
    }
    std::cout << count / 1000.0 << std::endl;
    std::cout << sbf.num_set_bits() << std::endl;
    std::cout << sizeof(uintmax_t) << std::endl;
    return 0;
}
