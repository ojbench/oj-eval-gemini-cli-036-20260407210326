#ifndef DYNAMIC_BITSET_HPP
#define DYNAMIC_BITSET_HPP

#include <vector>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <string>

struct dynamic_bitset {
    dynamic_bitset() = default;
    ~dynamic_bitset() = default;
    dynamic_bitset(const dynamic_bitset &) = default;
    dynamic_bitset &operator = (const dynamic_bitset &) = default;

    dynamic_bitset(std::size_t n) {
        num_bits = n;
        blocks.assign((n + 63) / 64, 0);
    }

    dynamic_bitset(const std::string &str) {
        num_bits = str.length();
        blocks.assign((num_bits + 63) / 64, 0);
        for (std::size_t i = 0; i < num_bits; ++i) {
            if (str[i] == '1') {
                blocks[i / 64] |= (1ULL << (i % 64));
            }
        }
    }

    bool operator [] (std::size_t n) const {
        return (blocks[n / 64] >> (n % 64)) & 1;
    }

    dynamic_bitset &set(std::size_t n, bool val = true) {
        if (val) {
            blocks[n / 64] |= (1ULL << (n % 64));
        } else {
            blocks[n / 64] &= ~(1ULL << (n % 64));
        }
        return *this;
    }

    dynamic_bitset &push_back(bool val) {
        if (num_bits % 64 == 0) {
            blocks.push_back(0);
        }
        if (val) {
            blocks[num_bits / 64] |= (1ULL << (num_bits % 64));
        }
        num_bits++;
        return *this;
    }

    bool none() const {
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i] != 0) return false;
        }
        return true;
    }

    bool all() const {
        if (num_bits == 0) return true;
        std::size_t full_blocks = num_bits / 64;
        for (std::size_t i = 0; i < full_blocks; ++i) {
            if (blocks[i] != ~0ULL) return false;
        }
        std::size_t rem = num_bits % 64;
        if (rem > 0) {
            uint64_t mask = (1ULL << rem) - 1;
            if ((blocks.back() & mask) != mask) return false;
        }
        return true;
    }

    std::size_t size() const {
        return num_bits;
    }

    dynamic_bitset &operator |= (const dynamic_bitset &other) {
        std::size_t min_blocks = (std::min(num_bits, other.num_bits) + 63) / 64;
        for (std::size_t i = 0; i < min_blocks; ++i) {
            blocks[i] |= other.blocks[i];
        }
        clean_padding();
        return *this;
    }

    dynamic_bitset &operator &= (const dynamic_bitset &other) {
        std::size_t min_len = std::min(num_bits, other.num_bits);
        std::size_t full_blocks = min_len / 64;
        for (std::size_t i = 0; i < full_blocks; ++i) {
            blocks[i] &= other.blocks[i];
        }
        std::size_t rem = min_len % 64;
        if (rem > 0) {
            uint64_t mask = (1ULL << rem) - 1;
            blocks[full_blocks] = (blocks[full_blocks] & ~mask) | (blocks[full_blocks] & other.blocks[full_blocks] & mask);
        }
        return *this;
    }

    dynamic_bitset &operator ^= (const dynamic_bitset &other) {
        std::size_t min_blocks = (std::min(num_bits, other.num_bits) + 63) / 64;
        for (std::size_t i = 0; i < min_blocks; ++i) {
            blocks[i] ^= other.blocks[i];
        }
        clean_padding();
        return *this;
    }

    dynamic_bitset &operator <<= (std::size_t n) {
        if (n == 0) return *this;
        std::size_t new_num_bits = num_bits + n;
        std::vector<uint64_t> new_blocks((new_num_bits + 63) / 64, 0);
        
        std::size_t block_shift = n / 64;
        std::size_t bit_shift = n % 64;
        
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            if (bit_shift == 0) {
                if (i + block_shift < new_blocks.size()) {
                    new_blocks[i + block_shift] = blocks[i];
                }
            } else {
                if (i + block_shift < new_blocks.size()) {
                    new_blocks[i + block_shift] |= (blocks[i] << bit_shift);
                }
                if (i + block_shift + 1 < new_blocks.size()) {
                    new_blocks[i + block_shift + 1] |= (blocks[i] >> (64 - bit_shift));
                }
            }
        }
        
        blocks = std::move(new_blocks);
        num_bits = new_num_bits;
        clean_padding();
        return *this;
    }

    dynamic_bitset &operator >>= (std::size_t n) {
        if (n == 0) return *this;
        if (n >= num_bits) {
            num_bits = 0;
            blocks.clear();
            return *this;
        }
        
        std::size_t new_num_bits = num_bits - n;
        std::vector<uint64_t> new_blocks((new_num_bits + 63) / 64, 0);
        
        std::size_t block_shift = n / 64;
        std::size_t bit_shift = n % 64;
        
        for (std::size_t i = block_shift; i < blocks.size(); ++i) {
            if (bit_shift == 0) {
                if (i - block_shift < new_blocks.size()) {
                    new_blocks[i - block_shift] = blocks[i];
                }
            } else {
                if (i - block_shift < new_blocks.size()) {
                    new_blocks[i - block_shift] |= (blocks[i] >> bit_shift);
                }
                if (i > block_shift && i - block_shift - 1 < new_blocks.size()) {
                    new_blocks[i - block_shift - 1] |= (blocks[i] << (64 - bit_shift));
                }
            }
        }
        
        blocks = std::move(new_blocks);
        num_bits = new_num_bits;
        clean_padding();
        return *this;
    }

    dynamic_bitset &set() {
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            blocks[i] = ~0ULL;
        }
        clean_padding();
        return *this;
    }

    dynamic_bitset &flip() {
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            blocks[i] = ~blocks[i];
        }
        clean_padding();
        return *this;
    }

    dynamic_bitset &reset() {
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            blocks[i] = 0;
        }
        return *this;
    }

private:
    std::vector<uint64_t> blocks;
    std::size_t num_bits = 0;

    void clean_padding() {
        std::size_t rem = num_bits % 64;
        if (rem > 0 && !blocks.empty()) {
            blocks.back() &= (1ULL << rem) - 1;
        }
    }
};

#endif