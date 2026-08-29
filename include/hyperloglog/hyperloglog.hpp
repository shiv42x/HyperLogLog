#pragma once
#include <algorithm>
#include <atomic>
#include <bit>
#include <concepts>
#include <cstdint>
#include <cmath> 
#include <gtest/gtest_prod.h>
#include <mutex>
#include <stdexcept>
#include <vector>
#include "MurmurHash3.h"

namespace hll {
template <typename T>
class HyperLogLog {
    public:
        static int clamp_b(int b) {
            if (b < 4) return 4;
            if (b > 16) return 16;
            return b;
        }

        HyperLogLog() = delete;
        explicit HyperLogLog(uint8_t b):   
            b_(clamp_b(b)), m_(static_cast<size_t>(1) << b), registers_(m_, 0) {
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
            
            uint8_t first_one_pos = lz_count + 1;
            
            std::atomic_ref<uint8_t> atomic_register(registers_[idx]);
            uint8_t expected = atomic_register.load(std::memory_order_relaxed);
            while (first_one_pos > expected 
                && !atomic_register.compare_exchange_weak(
                    expected,
                    first_one_pos,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed
                )) {};
        }
        
        void merge(const HyperLogLog& other) {
            if (registers_.size() != other.registers_.size()) {
               throw std::invalid_argument("Cannot merge HLL sketches with differing register sizes.");
            }
            
            std::ranges::transform(
                registers_, other.registers_, registers_.begin(),
                [](uint8_t a, uint8_t b) { return std::max(a, b); }
            );
        }
                
        double estimate()  const {
            // compute harmonic mean
            double sum = 0.0;
            size_t num_zeros = 0;

            for (uint8_t r : registers_) {
                if (r == 0) ++num_zeros;
                sum += std::ldexp(1.0, -r);
            }
            double estimate = alphamm_ / sum;
            if (estimate <= 2.5 * m_) {
                return m_ * std::log(static_cast<double>(m_) / num_zeros);
            }
            return estimate;
        }

        void clear() {
            std::ranges::fill(registers_.begin(), registers_.end(), 0);    
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

}; // namespace hll