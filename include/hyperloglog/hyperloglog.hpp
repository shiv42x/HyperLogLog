#pragma once
#include <vector>
#include <cstdint>

namespace hll {

template <typename T>
class HyperLogLog {
    public:
        HyperLogLog() = delete;
        explicit HyperLogLog(uint8_t b):   
            b_(b), m_(static_cast<size_t>(1) << b), registers_(m_, 0 ) {
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

        void add() {
        }
        uint64_t estimate()  const {
        }
        void merge(const HyperLogLog& other) {
        }
        void clear() {
        }

    private:
        uint8_t                 b_;
        size_t                  m_;
        double                  alphamm_;
        std::vector<uint8_t>    registers_;
}

};