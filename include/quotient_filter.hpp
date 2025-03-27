#ifndef PDS_QUOTIENT_FILTER_HPP
#define PDS_QUOTIENT_FILTER_HPP

#include "hash.hpp"

namespace pds {
template <class Key, class Hash = pds::hash::default_hash<Key>,
          class Container = std::vector<unsigned int>>
class quotient_filter {
 public:
  // assert or trait that these are unsigned
  using seed_type = typename Hash::seed_type;
  using hash_type = typename Hash::hash_type;

  quotient_filter(size_t num_bits_quotient, size_t num_bits_remainder,
                  Seed seed)
      : _table(std::pow(2, num_bits_quotient), 0), _seed{seed} {
    assert(num_bits_quotient + num_bits_remainder ==
               std::numeric_limits<hash_type>::digits,
           "q+r must equal num bits in underlying type");
  }
  template <class InputIt>
  void insert(InputIt first, InputIt last) noexcept {
    for (auto it = first; it != last; ++it) {
      insert(*it);
    }
  }
  void insert(const Key &key) {
    assert(_size < _table.size());
    auto [quotient, remainder] = divide(Hash{}(key, _seed));
    auto quotient_seen = _bucket_occupied[quotient];
    if (!quotient_seen) _bucket_occupied[quotient] = true;

    if (!quotient_seen && !_is_shifted[quotient]) {
      // insert in canonical slot if it is empty
      _bucket_occupied[quotient] = true;
      _table[quotient] = remainder;
    } else if (quotient_seen && !_is_shifted[quotient]) {
      // start of cluster

    } else if (quotient_seen && _is_shifted[quotient]) {
      // find where run must start

    } else if (_bucket_occupied[quotient] && _is_shifted[quotient]) {
      // find location within run

    } else {
      // if canonical slot is occupied get run location
      auto i = (_bucket_occupied[quotient] && !_is_shifted[quotient])
                   ? quotient
                   : find_run(quotient);

      if (_bucket_occupied[quotient]) {
        // if _bucket_occupied then find location within run
        while (remainder > _table[i++]) {
          if (i == _table.size()) i = 0;
        }
        if (remainder == _table[i]) return;
      }
      if (_bucket_occupied[i] || _is_shifted[i]) {
        // if destination is occupied then shift contents
        auto end_i = end_of_cluster(i);
        shift(i, end_i);
      }
      _table[i] = remainder;
      _bucket_occupied[quotient] = true;
      if (i != quotient) _is_shifted[i] = true;
      if (i != run_begin)

        // otherwise it is where the run must begin

        if (!bucket_occupied[quotient]) {
          *run_begin = remai
        }
    }
  }
  bool contains(const Key &key) const noexcept {
    for (const auto hash : _hash_generator.hashes(key)) {
      if (!_bitset[hash % _bitset.size()]) return false;
    }
    return true;
  }
  void clear() noexcept { _bitset.reset(); }
  size_t size() const noexcept { return _bitset.size(); }

  float load_factor() const noexcept {
    return _bitset.count() / (float)_bitset.size();
  }

  HashGenerator hash_generator() const { return _hash_generator; }

 private:
  Container _table;
  vector<bool> _bucket_occupied, _run_continued, _is_shifted;
  unsigned int _r;
  size_t _size;
  Seed _seed;
  auto divide(hash_type) {
    return std::make_tuple(hash >> _r, hash & ((1 << r) - 1));
  }
};
}  // namespace pds
#endif
