#include <iostream>
#include <limits>
#include <random>
#include <ranges>
#include <unordered_set>
#include <vector>
#include <sul/dynamic_bitset.hpp>
#include "ds.hpp"
#include "hash.hpp"
#include "bloom_filter.hpp"
#include "MurmurHash3.h"


int main() {
    auto engine =std::default_random_engine{std::random_device{}()};
    auto dist = std::uniform_int_distribution<>{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
    constexpr size_t NUM = 100;
    
    std::vector<int> v(NUM);
    pds::bloom_filter<int> bf(NUM, 0.01);
    for ( auto& x : v) {
        x=dist(engine);
        bf.insert(x);
    }
    int count = 0;
    for (size_t i = 0; i<NUM; i++) {
        if(!bf.contains(v[i])) count++;
    }
    std::cout << count << std::endl;
    count=0;
    pds::bloom_filter<int> bf2(1000,(size_t)4);
    for (int i = 0; i<5*NUM; i++) {
        bf2.insert(i*2);
    }
    for (int i = 0; i<5*NUM; i++) {
        if(bf2.contains(2*i+1)) count++;
    }
    for (int i = 5*NUM; i<10*NUM; i++) {
        if(bf2.contains(i)) count++;
    }
    std::cout << "h " <<count/1000.0 << std::endl;
    std::cout << bf2.num_set_bits()<< std::endl;
    
    pds::bloom_filter<int> nbf(1000,(size_t)4);
    for ( auto x : v) {
        nbf.insert(x);
    }
    count = 0;
    for (size_t i = 0; i<NUM; i++) {
        if(!nbf.contains(v[i])) count++;
    }
    std::cout << count << std::endl;
    std::cout << nbf.num_set_bits()<< std::endl;
    sul::dynamic_bitset<> bitset1(12, 0b0100010110111);
    std::cout << "bitset1     = " << bitset1 << std::endl;
    struct Test {
        void operator()(){std::cout << "test\n";}
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
    std::cout << sizeof(Test)<<std::endl;
    std::cout << sizeof(Test2)<<std::endl;
    
    pds::bloom_filter<int> sbf(1000,(size_t)4);
    count = 0;
    for (int i = 0; i<5*NUM; i++) {
        sbf.insert(i*2);
    }
    for (int i = 0; i<5*NUM; i++) {
        if(sbf.contains(2*i+1)) count++;
    }
    for (int i = 5*NUM; i<10*NUM; i++) {
        if(sbf.contains(i)) count++;
    }
    std::cout << count/1000.0 << std::endl;
    std::cout << sbf.num_set_bits() << std::endl;
    
    return 0;
}
