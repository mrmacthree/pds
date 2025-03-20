#ifndef PDS_HASH_HPP
#define PDS_HASH_HPP

#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <unordered_set>
#include <ranges>
#include "MurmurHash3.h"

namespace pds {
namespace hash {


template <class Key, class Hash, class Allocator = std::allocator<typename Hash::seed_type>>
class seeded_hash_generator {
public:
    using seed_type = typename Hash::seed_type;
    using hash_type = typename Hash::hash_type;
    //seeded_hash_generator(size_t num_hashes, unsigned int rng_seed = std::random_device{}(), const Allocator& alloc = Allocator()) : _seeds(num_hashes, alloc) {
    seeded_hash_generator(size_t num_hashes, unsigned int rng_seed = 1, const Allocator& alloc = Allocator()) : _seeds(num_hashes, alloc) {
        auto engine =std::mt19937_64{rng_seed};
        auto dist = std::uniform_int_distribution<seed_type>{std::numeric_limits<seed_type>::min(), std::numeric_limits<seed_type>::max()};
        std::unordered_set<seed_type,std::hash<Key>,std::equal_to<Key>,Allocator> previous_seeds(alloc);
        for (auto& seed:  _seeds) {
            seed = dist(engine);
            while (previous_seeds.contains(seed)) seed = dist(engine);
            previous_seeds.insert(seed);
        }
    }
    seeded_hash_generator(std::initializer_list<seed_type> seeds, const Allocator& alloc = Allocator()) : _seeds(seeds, alloc) {}
    auto hashes(const Key& key) const {
        return _seeds | std::views::transform([&](seed_type seed){
            return Hash{}(key,seed);
        });
    }
    
    std::vector<seed_type, Allocator> get_seeds() const {
        return _seeds;
    }
    size_t hashes_per_key() const {
        return _seeds.size();
    }
    
private:
    std::vector<seed_type, Allocator> _seeds;
};

template <class Key>
struct murmer3_x64_128 {
    using seed_type = uint32_t;
    using hash_type = uint64_t;
    unsigned long long operator()(const Key& key, seed_type seed) {
        uint64_t hash[2];
        MurmurHash3_x64_128(&key, sizeof(Key), seed, hash);
        return static_cast<unsigned long long>(hash[0]);
    }
};

template <class Key>
struct murmer3_x86_32 {
    using seed_type = uint32_t;
    using hash_type = uint32_t;
    unsigned long long operator()(const Key& key, seed_type seed) {
        uint32_t hash;
        MurmurHash3_x86_32(&key, sizeof(Key), seed, &hash);
        return static_cast<unsigned long long>(hash);
    }
};
template<class Key>
using default_hash = murmer3_x86_32<Key>;

template<class Key>
using default_hash_generator = seeded_hash_generator<Key,default_hash<Key>>;
}
}

#endif
