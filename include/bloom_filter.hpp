#ifndef PDS_BLOOM_FILTER_HPP
#define PDS_BLOOM_FILTER_HPP

#include <bit>
#include <ranges>
#include <concepts>
#include <cstdint>
#include <vector>

#include "hash.hpp"
// use fastrange for faster modulo or libdivide
// Options for prime number and power-of-2 sized bitvectors. Apparently you want
// to mod a hash by a prime. use chache local HashGenerator use SIMD
// HashGenerator libpopcnt for counting bits highway for simd page tasble
// awareness? Bloom Filter implementations
// https://github.com/peterboncz/bloomfilter-bsd/tree/master
// https://github.com/apache/impala/blob/master/be/src/util/bloom-filter.h
// https://github.com/facebook/rocksdb/blob/88bc91f3cc2b492b8a45ba2c49650f527df97ad8/util/dynamic_bloom.h
// https://github.com/chromium/chromium/blob/faf8581c2f9cdcb590d3544530c88a00c043461b/components/optimization_guide/core/bloom_filter.cc
// https://github.com/daankolthof/bloomfilter
// https://save-buffer.github.io/bloom_filter.html
// https://github.com/save-buffer/bloomfilter_benchmarks
// https://github.com/ArashPartow/bloom
// https://llimllib.github.io/bloomfilter-tutorial/

namespace pds {

template <typename Key,
          hash::HashGenerator<Key> HashGen =
              pds::hash::default_hash_generator<Key>,
          std::unsigned_integral WordType = uint8_t,
          typename Allocator = std::allocator<WordType>>
class bloom_filter {
 public:
  using key_type = Key;
  using hash_type = HashGen::hash_type;
  using size_type = size_t;
  using word_type = WordType;
  using allocator_type = Allocator;
  using hash_generator_type = HashGen;

  static constexpr size_type bits_per_word =
      std::numeric_limits<word_type>::digits;
  static constexpr size_type bits_per_word_log2 =
      std::countr_zero(bits_per_word);
  static constexpr size_type word_mask = bits_per_word-1u;

  static constexpr size_t optimal_num_bits(size_t input_size,
                                           double false_positive_probability) {
    return static_cast<size_t>(-std::log(false_positive_probability) *
                               input_size / std::pow(std::log(2.0), 2));
  }
  static constexpr size_t optimal_num_hashes(
      size_t input_size, double false_positive_probability) {
    return static_cast<size_t>(
        optimal_num_bits(input_size, false_positive_probability) *
        std::log(2.0) / input_size);
  }
  static constexpr size_t approximate_cardinality(size_t bit_capacity,
                                                  size_t num_set_bits,
                                                  size_t hashes_per_key) {
    return static_cast<size_t>(
        -(static_cast<double>(bit_capacity) /
          static_cast<double>(hashes_per_key)) *
        std::log(1 - static_cast<double>(num_set_bits) /
                         static_cast<double>(bit_capacity)));
  }
  static constexpr double false_positive_probability(size_t bit_capacity,
                                                     size_t input_size,
                                                     size_t hashes_per_key) {
    return std::pow(1.0 - std::exp(-hashes_per_key * input_size / bit_capacity),
                    hashes_per_key);
  }

  bloom_filter(size_t num_bits, size_t num_hashes,
               const Allocator &alloc = Allocator())
      : bit_array_((num_bits + bits_per_word - 1) / bits_per_word, 0, alloc),
        hash_generator_(num_hashes) {}
  bloom_filter(size_t input_size, double false_positive_probability = 0.03,
               const Allocator &alloc = Allocator())
      : bit_array_(optimal_num_bits(input_size, false_positive_probability) >>
                       bits_per_word_log2,
                   0, alloc),
        hash_generator_(
            optimal_num_hashes(input_size, false_positive_probability)) {}
  bloom_filter(size_t num_bits, HashGen hash_generator,
               const Allocator &alloc = Allocator())
      : bit_array_((num_bits + bits_per_word - 1) / bits_per_word, 0, alloc),
        hash_generator_(hash_generator) {}

  template <typename InputIt>
  void insert(InputIt first, InputIt last) noexcept {
    for (auto it = first; it != last; ++it) {
      insert(*it);
    }
  }
  void insert(const Key &key) noexcept {
    for (auto hash : hash_generator_.hashes(key)) {
      auto bit_index = hash % (bit_array_.size() * bits_per_word);
      bit_array_[bit_index >> bits_per_word_log2] |= 1
                                                     << (bit_index & word_mask);
    }
  }
  bool contains(const Key &key) const noexcept {
    for (auto hash : hash_generator_.hashes(key)) {
      auto bit_index = hash % (bit_array_.size() * bits_per_word);
      if (!(bit_array_[bit_index >> bits_per_word_log2] &
            (1 << (bit_index & word_mask))))
        return false;
    }
    return true;
  }
  void clear() noexcept { std::fill(bit_array_.begin(), bit_array_.end(), 0); }

  // Returns the number of bits in the underlying bit array.
  size_t bit_capacity() const noexcept { return bit_array_.size(); }

  // Returns the number of set bits in the underlying bit array.
  size_t num_set_bits() const noexcept {
    auto rv = bit_array_ | std::views::transform(std::popcount<word_type>);
    return std::accumulate(rv.begin(), rv.end(), 0);
  }

  bool empty() const noexcept {
    return std::all_of(bit_array_.begin(), bit_array_.end(),
                       [](word_type x) { return x == 0; });
  }

  void swap(
      const bloom_filter<Key, HashGen, WordType, Allocator> &other) noexcept {
    bit_array_.swap(other.bit_array_);
  }

  size_t hashes_per_key() const noexcept {
    return hash_generator_.hashes_per_key();
  }

  size_t approximate_cardinality() const noexcept {
    return approximate_cardinality(bit_capacity(), num_set_bits(),
                                   hashes_per_key());
  }

  double approximate_fpp() const noexcept {
    return false_positive_probability(bit_capacity(), approximate_cardinality(),
                                      hashes_per_key());
  }

  const std::vector<word_type> &data() { return bit_array_; }

  bloom_filter<Key, HashGen, WordType, Allocator> &operator&=(
      const bloom_filter<Key, HashGen, WordType, Allocator> &other) {
    assert(other.bit_capacity() == bit_capacity());
    for (size_t i = 0; i < bit_array_.size(); ++i) {
      bit_array_[i] &= other.bit_array_[i];
    }
    return *this;
  }

  bloom_filter<Key, HashGen, WordType, Allocator> operator&(
      const bloom_filter<Key, HashGen, WordType, Allocator> &other) {
    bloom_filter<Key, HashGen, WordType, Allocator> res(*this);
    res &= other;
    return res;
  }

  bloom_filter<Key, HashGen, WordType, Allocator> &operator|=(
      const bloom_filter<Key, HashGen, WordType, Allocator> &other) {
    assert(other.bit_capacity() == bit_capacity());
    for (size_t i = 0; i < bit_array_.size(); ++i) {
      bit_array_[i] |= other.bit_array_[i];
    }
    return *this;
  }

  bloom_filter<Key, HashGen, WordType, Allocator> operator|(
      const bloom_filter<Key, HashGen, WordType, Allocator> &other) {
    bloom_filter<Key, HashGen, Allocator> res(*this);
    res |= other;
    return res;
  }

 private:
  std::vector<word_type, allocator_type> bit_array_;
  hash_generator_type hash_generator_;
};
}  // namespace pds
#endif
