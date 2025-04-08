#ifndef PDS_BLOOM_FILTER_HPP
#define PDS_BLOOM_FILTER_HPP

#include <bit>
#include <ranges>
#include <concepts>
#include <cstdint>
#include <vector>

#include "hash.hpp"
// use fastrange for faster modulo or libdivide,
// Options for prime number and power-of-2 sized bitvectors. Apparently you want
// to mod a hash by a prime. use chache local HashGenerator, use SIMD
// HashGenerator, libpopcnt for counting bits, highway for simd, page tasble
// awareness? Bloom Filter implementations
// What about sparse bloom filters?
// https://github.com/peterboncz/bloomfilter-bsd/tree/master
// https://github.com/apache/impala/blob/master/be/src/util/bloom-filter.h
// https://github.com/facebook/rocksdb/blob/88bc91f3cc2b492b8a45ba2c49650f527df97ad8/util/dynamic_bloom.h
// https://github.com/chromium/chromium/blob/faf8581c2f9cdcb590d3544530c88a00c043461b/components/optimization_guide/core/bloom_filter.cc
// https://github.com/daankolthof/bloomfilter
// https://save-buffer.github.io/bloom_filter.html
// https://github.com/save-buffer/bloomfilter_benchmarks
// https://github.com/ArashPartow/bloom
// https://llimllib.github.io/bloomfilter-tutorial/
// https://github.com/domodwyer/bloom2

namespace pds {

  namespace bloom_filter_policy {
    template <typename T>
    concept SizingPolicy = requires(T t, std::size_t n) {
      { T{}(n) } -> std::same_as<std::size_t>;
    };
    struct power_of_two {
      std::size_t operator()(std::size_t n) const {
        return std::bit_ceil(n);
      }
    };
    template <std::unsigned_integral WordType>
    struct word_multiple
    {
      static constexpr std::size_t bits_per_word =
      std::numeric_limits<WordType>::digits;
      std::size_t operator()(std::size_t n) const {
        return ((n + bits_per_word - 1) / bits_per_word) * bits_per_word;
      }
    };
    
    struct exact {
      std::size_t operator()(std::size_t n) const {
        return n;
      }
    };
    struct prime {
      // These numbers come from libstdc++
      static constexpr std::size_t primes[] =
        {
        /* 0     */              5ul,
        /* 1     */              11ul, 
        /* 2     */              23ul, 
        /* 3     */              47ul, 
        /* 4     */              97ul, 
        /* 5     */              199ul, 
        /* 6     */              409ul, 
        /* 7     */              823ul, 
        /* 8     */              1741ul, 
        /* 9     */              3469ul, 
        /* 10    */              6949ul, 
        /* 11    */              14033ul, 
        /* 12    */              28411ul, 
        /* 13    */              57557ul, 
        /* 14    */              116731ul, 
        /* 15    */              236897ul,
        /* 16    */              480881ul, 
        /* 17    */              976369ul,
        /* 18    */              1982627ul, 
        /* 19    */              4026031ul,
        /* 20    */              8175383ul, 
        /* 21    */              16601593ul, 
        /* 22    */              33712729ul,
        /* 23    */              68460391ul, 
        /* 24    */              139022417ul, 
        /* 25    */              282312799ul, 
        /* 26    */              573292817ul, 
        /* 27    */              1164186217ul,
        /* 28    */              2364114217ul, 
        /* 29    */              4294967291ul,
        /* 30    */ (std::size_t)8589934583ull,
        /* 31    */ (std::size_t)17179869143ull,
        /* 32    */ (std::size_t)34359738337ull,
        /* 33    */ (std::size_t)68719476731ull,
        /* 34    */ (std::size_t)137438953447ull,
        /* 35    */ (std::size_t)274877906899ull,
        /* 36    */ (std::size_t)549755813881ull,
        /* 37    */ (std::size_t)1099511627689ull,
        /* 38    */ (std::size_t)2199023255531ull,
        /* 39    */ (std::size_t)4398046511093ull,
        /* 40    */ (std::size_t)8796093022151ull,
        /* 41    */ (std::size_t)17592186044399ull,
        /* 42    */ (std::size_t)35184372088777ull,
        /* 43    */ (std::size_t)70368744177643ull,
        /* 44    */ (std::size_t)140737488355213ull,
        /* 45    */ (std::size_t)281474976710597ull,
        /* 46    */ (std::size_t)562949953421231ull, 
        /* 47    */ (std::size_t)1125899906842597ull,
        /* 48    */ (std::size_t)2251799813685119ull, 
        /* 49    */ (std::size_t)4503599627370449ull,
        /* 50    */ (std::size_t)9007199254740881ull, 
        /* 51    */ (std::size_t)18014398509481951ull,
        /* 52    */ (std::size_t)36028797018963913ull, 
        /* 53    */ (std::size_t)72057594037927931ull,
        /* 54    */ (std::size_t)144115188075855859ull,
        /* 55    */ (std::size_t)288230376151711717ull,
        /* 56    */ (std::size_t)576460752303423433ull,
        /* 57    */ (std::size_t)1152921504606846883ull,
        /* 58    */ (std::size_t)2305843009213693951ull,
        /* 59    */ (std::size_t)4611686018427387847ull,
        /* 60    */ (std::size_t)9223372036854775783ull,
        /* 61    */ (std::size_t)18446744073709551557ull,
      };
      std::size_t operator()(std::size_t n) const {
        auto it = std::ranges::upper_bound(primes, n);
        if (it == std::ranges::end(primes)) {
          throw std::out_of_range("No prime number found");
        }
        return *it;
      }
    };
  }

template <typename Key,
          hash::HashGenerator<Key> HashGen =
              pds::hash::default_hash_generator<Key>,
          typename Allocator = std::allocator<unsigned long>, bloom_filter_policy::SizingPolicy SizingPolicy =
              bloom_filter_policy::power_of_two>
class bloom_filter {
  using word_type = unsigned long;
 public:
  using key_type = Key;
  using hash_type = HashGen::hash_type;
  using size_type = std::size_t;
  using allocator_type = Allocator;
  using hash_generator_type = HashGen;

  static constexpr size_type bits_per_word =
      std::numeric_limits<word_type>::digits;
  static constexpr size_type bits_per_word_log2 =
      std::countr_zero(bits_per_word);
  static constexpr size_type word_mask = bits_per_word-1u;

  static constexpr std::size_t optimal_num_bits(std::size_t input_size,
                                           double false_positive_probability) {
    return static_cast<std::size_t>(-std::log(false_positive_probability) *
                               input_size / std::pow(std::log(2.0), 2));
  }
  static constexpr std::size_t optimal_num_hashes(
      std::size_t input_size, double false_positive_probability) {
    return static_cast<std::size_t>(
        optimal_num_bits(input_size, false_positive_probability) *
        std::log(2.0) / input_size);
  }
  static constexpr std::size_t approximate_cardinality(std::size_t bit_capacity,
                                                  std::size_t num_set_bits,
                                                  std::size_t hashes_per_key) {
    return static_cast<std::size_t>(
        -(static_cast<double>(bit_capacity) /
          static_cast<double>(hashes_per_key)) *
        std::log(1 - static_cast<double>(num_set_bits) /
                         static_cast<double>(bit_capacity)));
  }
  static constexpr double false_positive_probability(std::size_t bit_capacity,
                                                     std::size_t input_size,
                                                     std::size_t hashes_per_key) {
    return std::pow(1.0 - std::exp(-hashes_per_key * input_size / bit_capacity),
                    hashes_per_key);
  }

  bloom_filter(std::size_t num_bits, std::size_t num_hashes,
               const Allocator &alloc = Allocator())
      : num_bits_{SizingPolicy{}(num_bits)}, bit_array_((num_bits_ + bits_per_word - 1) / bits_per_word, 0, alloc),
        hash_generator_(num_hashes,num_bits_) {}
  bloom_filter(std::size_t input_size, double false_positive_probability = 0.03,
               const Allocator &alloc = Allocator())
      : num_bits_{SizingPolicy{}(optimal_num_bits(input_size, false_positive_probability))}, bit_array_((num_bits_ + bits_per_word - 1) / bits_per_word,
                   0, alloc),
        hash_generator_(
            optimal_num_hashes(input_size, false_positive_probability),num_bits_) {}
  bloom_filter(HashGen hash_generator,
               const Allocator &alloc = Allocator())
      : num_bits_{hash_generator.range()}, bit_array_((num_bits_ + bits_per_word - 1) / bits_per_word, 0, alloc),
        hash_generator_(std::move(hash_generator)) {}

  template <typename InputIt>
  void insert(InputIt first, InputIt last) noexcept {
    for (auto it = first; it != last; ++it) {
      insert(*it);
    }
  }
  void insert(const Key &key) noexcept {
    for (auto hash : hash_generator_.hashes(key)) {
      bit_array_[hash >> bits_per_word_log2] |= 1
                                                     << (hash & word_mask);
    }
  }
  bool contains(const Key &key) const noexcept {
    for (auto hash : hash_generator_.hashes(key)) {
      if (!(bit_array_[hash >> bits_per_word_log2] &
            (1 << (hash & word_mask))))
        return false;
    }
    return true;
  }
  void clear() noexcept { std::fill(bit_array_.begin(), bit_array_.end(), 0); }

  // Returns the number of bits.
  std::size_t bit_capacity() const noexcept { return num_bits_; }

  // Returns the number of set bits in the underlying bit array.
  std::size_t num_set_bits() const noexcept {
    auto rv = bit_array_ | std::views::transform(std::popcount<word_type>);
    return std::accumulate(rv.begin(), rv.end(), 0);
  }

  HashGen hash_generator() const noexcept {
    return hash_generator_;
  }

  bool empty() const noexcept {
    return std::all_of(bit_array_.begin(), bit_array_.end(),
                       [](word_type x) { return x == 0; });
  }

  void swap(bloom_filter<Key, HashGen, Allocator> &other) noexcept {
    bit_array_.swap(other.bit_array_);
  }

  std::size_t hashes_per_key() const noexcept {
    return hash_generator_.hashes_per_key();
  }

  std::size_t approximate_cardinality() const noexcept {
    return approximate_cardinality(bit_capacity(), num_set_bits(),
                                   hashes_per_key());
  }

  double approximate_fpp() const noexcept {
    return false_positive_probability(bit_capacity(), approximate_cardinality(),
                                      hashes_per_key());
  }

  const std::vector<word_type> &data() { return bit_array_; }

  bloom_filter<Key, HashGen, Allocator> &operator&=(
      const bloom_filter<Key, HashGen, Allocator> &other) {
    assert(other.bit_capacity() == bit_capacity());
    for (std::size_t i = 0; i < bit_array_.size(); ++i) {
      bit_array_[i] &= other.bit_array_[i];
    }
    return *this;
  }

  bloom_filter<Key, HashGen, Allocator> operator&(
      const bloom_filter<Key, HashGen, Allocator> &other) {
    bloom_filter<Key, HashGen, Allocator> res(*this);
    res &= other;
    return res;
  }

  bloom_filter<Key, HashGen, Allocator> &operator|=(
      const bloom_filter<Key, HashGen, Allocator> &other) {
    assert(other.bit_capacity() == bit_capacity());
    for (std::size_t i = 0; i < bit_array_.size(); ++i) {
      bit_array_[i] |= other.bit_array_[i];
    }
    return *this;
  }

  bloom_filter<Key, HashGen, Allocator> operator|(
      const bloom_filter<Key, HashGen, Allocator> &other) {
    bloom_filter<Key, HashGen, Allocator> res(*this);
    res |= other;
    return res;
  }

 private:
  std::size_t num_bits_;
  std::vector<word_type, allocator_type> bit_array_;
  hash_generator_type hash_generator_;
  
};

}  // namespace pds
#endif
