#ifndef PDS_BLOOM_FILTER_HPP
#define PDS_BLOOM_FILTER_HPP

#include "hash.hpp"
#include <sul/dynamic_bitset.hpp>

namespace pds {
template <class Key, class HashGenerator = pds::hash::default_hash_generator<Key>, class Bitset = sul::dynamic_bitset<unsigned long,std::allocator<unsigned long>>>
class bloom_filter {
public:
    bloom_filter(Bitset bitset, HashGenerator hash_generator) : _bitset(bitset), _hash_generator(hash_generator) {}
    bloom_filter(size_t num_bits, HashGenerator hash_generator) : _bitset(num_bits,false), _hash_generator(hash_generator) {}
    // add a parameter to feed in a seed
    bloom_filter(size_t num_bits, size_t num_hashes) : _bitset(num_bits,false), _hash_generator(num_hashes) {}
    bloom_filter(size_t input_size, double false_positive_rate) : _bitset(calculate_num_bits(input_size,false_positive_rate),false), _hash_generator(calculate_num_hashes(input_size,false_positive_rate)) {}
    template<class InputIt>
    bloom_filter(InputIt first, InputIt last, double false_positive_rate) : _bitset(calculate_num_bits(std::distance(first,last),false_positive_rate),false), _hash_generator{calculate_num_hashes(std::distance(first,last),false_positive_rate)} {
        insert(first,last);
    }
    template<class InputIt>
    void insert(InputIt first, InputIt last) noexcept {
        for(auto it = first; it!=last; ++it) {
            insert(*it);
        }
    }
    void insert(const Key& key) noexcept {
        for (const auto hash : _hash_generator.hashes(key)) {
            _bitset[hash%_bitset.size()]=true;
        }
    }
    bool contains(const Key& key) const noexcept {
        for (const auto hash : _hash_generator.hashes(key)) {
            if(!_bitset[hash%_bitset.size()]) return false;
        }
        return true;
    }
    void clear() noexcept {
        _bitset.reset();
    }
    size_t size() const noexcept {
        return _bitset.size();
    }
    
    float load_factor() const noexcept {
        return _bitset.count()/(float)_bitset.size();
    }
    
    HashGenerator hash_generator() const {
        return _hash_generator;
    }
    
private:
    Bitset _bitset;
    HashGenerator _hash_generator;
    static size_t calculate_num_bits(size_t input_size, double false_positive_rate) {
        return static_cast<size_t>(-std::log(false_positive_rate)*input_size/std::pow(std::log(2.0),2));
    }
    static size_t calculate_num_hashes(size_t input_size, double false_positive_rate) {
        return static_cast<size_t>(calculate_num_bits(input_size,false_positive_rate)*std::log(2.0)/input_size);
    }
};
}
#endif
