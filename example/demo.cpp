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
class bloom_filterx {
public:
    bloom_filterx(unsigned long long n, double f) : _n{n}, _f{f} {
        _m = static_cast<unsigned long long>(-std::log(_f)*_n/std::pow(std::log(2.0),2));
        _k = static_cast<int>(_m*std::log(2.0)/static_cast<double>(_n));
        _bit_array = std::vector<bool>(_m, false);
        std::cout << "k: " <<_k << std::endl;
    }
    void insert(int key) {
        for (uint32_t i = 0; i<_k; ++i) {
            uint64_t hash[2];
            MurmurHash3_x64_128(&key, 4, i, hash);
            _bit_array[hash[0]%_m] = true;
        }
    }
    bool contains(int key) {
        for (uint32_t i = 0; i<_k; ++i) {
            uint64_t hash[2];
            MurmurHash3_x64_128(&key, 4, i, hash);
            if (!_bit_array[hash[0]%_m]) return false;
        }
        return true;
    }
private:
    unsigned long long _n,_m;
    size_t _k;
    double _f;
    std::vector<bool> _bit_array;
};
template<class T>
struct hash {
    unsigned long long operator()(const T key, const uint32_t seed) {
        uint64_t hash[2];
        MurmurHash3_x64_128(&key, sizeof(T), seed, hash);
        return static_cast<unsigned long long>(hash[0]);
    }
};

template <class T, class S = uint32_t, class HashFunc = hash<T>>
class bloom2 {
public:
    bloom2(unsigned long long m, std::initializer_list<S> salt) : _bit_array(m,false), _salt{salt} {}
    bloom2(unsigned long long m) : _bit_array(m,false) {
        
    }
    void insert(T key) {
        for (auto s : _salt) {
            _bit_array[HashFunc{}(key,s)%_bit_array.size()]=true;
        }
    }
    bool contains(T key) const {
        for (auto s : _salt) {
            if(!_bit_array[HashFunc{}(key,s)%_bit_array.size()]) return false;
        }
        return true;
    }
    
private:
    std::vector<bool> _bit_array;
    std::vector<S> _salt;
};
template <class T>
struct murmer {
    unsigned long long operator()(const T& key, uint32_t seed) {
        uint64_t hash[2];
        MurmurHash3_x64_128(&key, sizeof(T), seed, hash);
        return static_cast<unsigned long long>(hash[0]);
    }
};

template <class T, class S, class Hasher>
class seeded_hash_generator {
public:
    seeded_hash_generator(size_t k) : _seeds(k) {
        auto engine =std::default_random_engine{std::random_device{}()};
        auto dist = std::uniform_int_distribution<S>{std::numeric_limits<S>::min(), std::numeric_limits<S>::max()};
        std::unordered_set<S> previous_seeds{};
        for (auto& seed:  _seeds) {
            seed = dist(engine);
            while (previous_seeds.contains(seed)) seed = dist(engine);
            previous_seeds.insert(seed);
        }
    }
    auto hashes(const T& key) const {
        return _seeds | std::views::transform([&](S seed){
            return Hasher{}(key,seed);
        });
    }
private:
    std::vector<uint32_t> _seeds;
};

template <class T, class Hasher = pds::hash::seeded_hash_generator<T,uint32_t,pds::hash::murmer3_x64_128<T>>>
class bloom_filter {
public:
    bloom_filter(size_t m, Hasher hasher) : _bit_array(m,false), _hasher(hasher) {}
    bloom_filter(size_t m, size_t k) : _bit_array(m,false), _hasher(k) {}
    bloom_filter(size_t n, double f) : _bit_array(calculate_m(n,f),false), _hasher(calculate_k(n,f)) {}
    template<class InputIt>
    bloom_filter(InputIt first, InputIt last, double f) : _bit_array(calculate_m(std::distance(first,last),f),false), _hasher{calculate_k(std::distance(first,last),f)} {
        insert(first,last);
    }
    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        for(auto it = first; it!=last; ++it) {
            insert(*it);
        }
    }
    void insert(const T& key) {
        for (const auto hash : _hasher.hashes(key)) {
            _bit_array[hash%_bit_array.size()]=true;
        }
    }
    bool contains(const T& key) const {
        for (const auto hash : _hasher.hashes(key)) {
            if(!_bit_array[hash%_bit_array.size()]) return false;
        }
        return true;
    }
    
    size_t count() const {
        return _bit_array.count();
    }
    
private:
    //std::vector<bool> _bit_array;
    sul::dynamic_bitset<> _bit_array;
    Hasher _hasher;
    size_t calculate_m(size_t n, double f) {
        return static_cast<size_t>(-std::log(f)*n/std::pow(std::log(2.0),2));
    }
    size_t calculate_k(size_t n, double f) {
        return static_cast<size_t>(calculate_m(n,f)*std::log(2.0)/static_cast<double>(n));
    }
};




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
    pds::bloom_filter<int> bf2(NUM, 0.1);
    for (int i = 0; i<5*NUM; i++) {
        bf2.insert(i*2);
    }
    for (int i = 0; i<5*NUM; i++) {
        if(bf2.contains(2*i+1)) count++;
    }
    for (int i = 5*NUM; i<10*NUM; i++) {
        if(bf2.contains(i)) count++;
    }
    std::cout << count/1000.0 << std::endl;
    
    pds::bloom_filter<int> nbf(1000,(size_t)4);
    for ( auto x : v) {
        nbf.insert(x);
    }
    count = 0;
    for (size_t i = 0; i<NUM; i++) {
        if(!nbf.contains(v[i])) count++;
    }
    std::cout << count << std::endl;
    std::cout << nbf.load_factor() << std::endl;
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
    return 0;
}
