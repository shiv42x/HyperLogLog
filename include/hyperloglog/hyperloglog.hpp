#pragma once
#include <atomic>
#include <bit>
#include <concepts>
#include <cstdint>
#include <cmath> 
#include <gtest/gtest_prod.h>
#include <vector>
#include "MurmurHash3.h"

namespace hll {
template <typename T>
class HyperLogLog {
    public:
        HyperLogLog() = delete;
        explicit HyperLogLog(uint8_t b):   
            b_(b), m_(static_cast<size_t>(1) << b), registers_(m_, 0) {
                double alpha;
                switch (m_) {
                    case 16:
                        alpha = 0.673;
                        break;
                    case 32:
                        alpha = 0.697;
                        break;
                    case 64:
                        alpha = 0.709;
                        break;
                    default:
                        alpha = 0.7213 / (1 + 1.079 / static_cast<double>(m_));
                        break;
                }
                alphamm_ = alpha * m_ * m_;
        }

        void add(const T& item) {
            uint64_t hash = safe_hash(item, 42);
            size_t idx = hash >> (64 - b_);
            
            uint8_t max_meaningful_zeros = 64 - b_;
            uint8_t lz_count = std::min(
                static_cast<uint8_t>(std::countl_zero(hash << b_)),
                max_meaningful_zeros
            );
            
            // +1 since we're storing position (1-indexed) of the first 1, not leading zeros count
            registers_[idx] = std::max(registers_[idx], static_cast<uint8_t>(lz_count + 1));
        }
        
        uint64_t estimate()  const {
            // compute harmonic mean
            double estimate;
            double sum = 0.0;

            size_t num_zeros = 0;

            for (auto reg : registers_) {
                if (reg == 0) ++num_zeros;
                sum += 1.0 / (1 << reg);
            }

            if (num_zeros == m_) return 0;

            estimate = alphamm_ / sum;
            return static_cast<uint64_t>(std::floor(estimate));
        }

        void clear() {
        }

    private:
        uint8_t                             b_;         // no. of bits to index registers
        size_t                              m_;         // no. of registers
        double                              alphamm_;   // cache alpha * m * m (for estimate formula)
        std::vector<uint8_t>                registers_; 
       
        uint64_t    safe_hash(const T& val, uint32_t seed) requires (std::integral<T> && !std::same_as<T, bool>) {
            return hash_bytes(&val, sizeof(T), seed);
        }

        uint64_t    safe_hash(const std::string& str, uint32_t seed) requires (std::same_as<T, std::string>) {
            return hash_bytes(str.data(), str.size(), seed);
        }
        
        uint64_t    hash_bytes(const void* data, size_t len, uint32_t seed) {
            std::uint64_t out[2];
            MurmurHash3_x64_128(data, len, seed, out);
            return out[0];
        }
};

};