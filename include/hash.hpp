#ifndef PDS_HASH_HPP
#define PDS_HASH_HPP

#include "MurmurHash3.h"
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <ranges>
#include <unordered_set>

namespace pds {
namespace hash {
template <typename T, typename Key>
concept HashGenerator = requires(T t, size_t n, Key k) {
  typename T::hash_type;
  T{n};
  { t.hashes_per_key() } -> std::convertible_to<std::size_t>;
  { t.hashes(k) } -> std::ranges::range;
  requires std::same_as<std::ranges::range_value_t<decltype(t.hashes(k))>,
                        typename T::hash_type>;
} && std::unsigned_integral<typename T::hash_type>;

template <typename T, typename Key>
concept HashFunction = requires(T t) {
  typename T::hash_type;
  typename T::seed_type;
  std::unsigned_integral<typename T::hash_type>;
  requires requires(Key &k, T::seed_type s) {
    { t.operator()(k, s) } -> std::same_as<typename T::hash_type>;
  };
};

template <typename Key, HashFunction<Key> Hash> class simple_hash_generator {
public:
  using seed_type = typename Hash::seed_type;
  using hash_type = typename Hash::hash_type;
  simple_hash_generator(size_t hashes_per_key)
      : _hashes_per_key(hashes_per_key) {}
  auto hashes(const Key &key) const {
    return std::views::iota(0u, _hashes_per_key) |
           std::views::transform(
               [&key](seed_type seed) { return Hash{}(key, seed); });
  }

  size_t hashes_per_key() const { return _hashes_per_key; }

private:
  size_t _hashes_per_key;
};

template <typename Key, HashFunction<Key> Hash,
          typename Allocator = std::allocator<typename Hash::seed_type>>
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
      while (previous_seeds.contains(seed))
        seed = dist(engine);
      previous_seeds.insert(seed);
    }
  }
  seeded_hash_generator(std::initializer_list<seed_type> seeds,
                        const Allocator &alloc = Allocator())
      : _seeds(seeds, alloc) {}
  auto hashes(const Key &key) const {
    return _seeds | std::views::transform(
                        [&](seed_type seed) { return Hash{}(key, seed); });
  }

  std::vector<seed_type, Allocator> get_seeds() const { return _seeds; }
  size_t hashes_per_key() const { return _seeds.size(); }

private:
  std::vector<seed_type, Allocator> _seeds;
};

template <typename Key> struct murmer3_x64_128 {
  using seed_type = uint32_t;
  using hash_type = uint64_t;
  hash_type operator()(const Key &key, seed_type seed) {
    uint64_t hash[2];
    MurmurHash3_x64_128(&key, sizeof(Key), seed, hash);
    return hash[0];
  }
};

template <typename Key> struct murmer3_x86_32 {
  using seed_type = uint32_t;
  using hash_type = uint32_t;
  hash_type operator()(const Key &key, seed_type seed) {
    uint32_t hash;
    MurmurHash3_x86_32(&key, sizeof(Key), seed, &hash);
    return hash;
  }
};
template <typename Key> using default_hash = murmer3_x86_32<Key>;

template <typename Key>
using default_hash_generator = simple_hash_generator<Key, default_hash<Key>>;
} // namespace hash
} // namespace pds

#endif
