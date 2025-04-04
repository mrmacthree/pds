#ifndef PDS_HASH_HPP
#define PDS_HASH_HPP

#include <limits>
#include <queue>
#include <random>
#include <ranges>
#include <concepts>
#include <cstdint>
#include <unordered_set>
#include <cassert>

#include "MurmurHash3.h"

namespace pds {
namespace hash {

template <typename T, typename Key>
concept HashFunction = requires(T t) {
  typename T::hash_type;
  typename T::seed_type;
  requires std::unsigned_integral<typename T::hash_type>;
  requires std::unsigned_integral<typename T::seed_type>;
  requires requires(Key &k, typename T::seed_type s) {
    requires std::same_as<decltype(s),typename T::seed_type>;
    { T{}(k, s) } -> std::same_as<typename T::hash_type>;
  };
};

template <typename T, typename Key>
concept HashGenerator = requires(T t, size_t n, Key k) {
  typename T::hash_type;
  requires std::unsigned_integral<typename T::hash_type>;
  T{n};
  { t.hashes_per_key() } -> std::convertible_to<std::size_t>;
  { t.hashes(k) } -> std::ranges::range;
  { t.range() } -> std::convertible_to<std::size_t>;
  requires std::same_as<std::ranges::range_value_t<decltype(t.hashes(k))>,
                        typename T::hash_type>;
};


template <typename T, typename HashType>
concept RangeFunction = std::unsigned_integral<HashType> && requires(T t, HashType h, std::size_t n) {
  { T{}(h, n) } -> std::same_as<HashType>;
};

template <std::unsigned_integral T = std::size_t>
struct mod_range {
  T operator()(T hash, T range) const {
    return hash % range;
  }
};
template <std::unsigned_integral T = std::size_t>
struct pow_2_range {
  T operator()(T hash, T range) const {
    return hash >> (std::countl_zero(range)+1);
  }
};
template <std::unsigned_integral T>
struct fast_range {
  T operator()(T hash, T range) const {
    return hash % range;
  }
};
template <>
struct fast_range<uint32_t> {
  uint32_t operator()(uint32_t hash, uint32_t range) const {
    return static_cast<uint32_t>((static_cast<uint64_t>(hash) * static_cast<uint64_t>(range)) >> 32);
  }
};
#ifdef __SIZEOF_INT128__ 
template <>
struct fast_range<uint64_t> {
  uint64_t operator()(uint64_t hash, uint64_t range) const {
    return static_cast<uint64_t>((static_cast<__uint128_t>(hash) * static_cast<__uint128_t>(range)) >> 64);
  }
};
#endif

template <typename Key, HashFunction<Key> Hash, RangeFunction<typename Hash::hash_type> Range = mod_range<typename Hash::hash_type>>
class simple_hash_generator {
 public:
  using seed_type = typename Hash::seed_type;
  using hash_type = typename Hash::hash_type;
  simple_hash_generator(size_t hashes_per_key, size_t range = std::numeric_limits<hash_type>::max())
      : _hashes_per_key(hashes_per_key), range_{range} {
        if constexpr (std::same_as<Range, pow_2_range<typename Hash::hash_type>>) {
          // Check if range is a power of 2
          assert((range & (range - 1)) == 0);
        }
      }
  auto hashes(const Key &key) const {
    return std::views::iota(0u, _hashes_per_key) |
           std::views::transform(
               [&](seed_type seed) { return Range{}(Hash{}(key, seed), range_); });
  }

  size_t hashes_per_key() const { return _hashes_per_key; }
  size_t range() const { return range_; }

 private:
  size_t _hashes_per_key, range_;
};

template <typename Key, HashFunction<Key> Hash,
          typename Allocator = std::allocator<typename Hash::seed_type>, RangeFunction<typename Hash::hash_type> Range = mod_range<typename Hash::hash_type>>
class seeded_hash_generator {
 public:
  using seed_type = typename Hash::seed_type;
  using hash_type = typename Hash::hash_type;
  seeded_hash_generator(size_t num_hashes,
                        unsigned int rng_seed = std::random_device{}(),
                        const Allocator &alloc = Allocator())
      : _seeds(num_hashes, alloc) {
    auto engine = std::mt19937_64{rng_seed};
    auto dist = std::uniform_int_distribution<seed_type>{
        std::numeric_limits<seed_type>::min(),
        std::numeric_limits<seed_type>::max()};
    std::unordered_set<seed_type, std::hash<Key>, std::equal_to<Key>, Allocator>
        previous_seeds(alloc);
    for (auto &seed : _seeds) {
      seed = dist(engine);
      while (previous_seeds.contains(seed)) seed = dist(engine);
      previous_seeds.insert(seed);
    }
  }
  seeded_hash_generator(std::initializer_list<seed_type> seeds,
                        const Allocator &alloc = Allocator())
      : _seeds(seeds, alloc) {}
  auto hashes(const Key &key) const {
    return _seeds | std::views::transform(
                        [&](seed_type seed) { return Range{}(Hash{}(key, seed)); });
  }

  std::vector<seed_type, Allocator> get_seeds() const { return _seeds; }
  size_t hashes_per_key() const { return _seeds.size(); }

 private:
  std::vector<seed_type, Allocator> _seeds;
};

template <typename Key>
struct murmer3_x64_128 {
  using seed_type = uint32_t;
  using hash_type = uint64_t;
  hash_type operator()(const Key &key, seed_type seed) {
    uint64_t hash[2];
    MurmurHash3_x64_128(&key, sizeof(Key), seed, hash);
    return hash[0];
  }
};

template <typename Key>
struct murmer3_x86_32 {
  using seed_type = uint32_t;
  using hash_type = uint32_t;
  hash_type operator()(const Key &key, seed_type seed) {
    uint32_t hash;
    MurmurHash3_x86_32(&key, sizeof(Key), seed, &hash);
    return hash;
  }
};



template <typename Key>
using default_hash = murmer3_x86_32<Key>;

template <typename Key>
using default_hash_generator = simple_hash_generator<Key, default_hash<Key>>;
}  // namespace hash
}  // namespace pds

#endif
