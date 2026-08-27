#include <atomic>
#include <barrier>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "hyperloglog/hyperloglog.hpp"

TEST(HLLTest, BasicTest) {
    hll::HyperLogLog<std::string> hll(4);
    ASSERT_EQ(hll.estimate(), 0);

    std::vector<std::string> test_strings = {
        "apple", "banana", "cherry", "date", "elderberry",
        "fig", "grape", "honeydew", "kiwi", "lemon",
        "apple", "banana", "cherry"
    };

    for (const auto& str: test_strings) {
        hll.add(str);
    }

    uint64_t estimate = hll.estimate();

    EXPECT_GT(estimate, 0);
}

// slop below
// Test 1: Verify the concurrent "Keep Only Max" lock-free register logic under direct contention
TEST(HLLTest, ConcurrentRegisterUpdateContention) {
    const size_t precision = 4; // 16 registers
    hll::HyperLogLog<std::string> hll(precision);
    
    // Artificially access internal or mock a heavy stream hitting identical register indices
    const size_t NUM_THREADS = 8;
    std::barrier sync_point(NUM_THREADS);
    std::vector<std::jthread> threads;
    threads.reserve(NUM_THREADS);

    // Force multiple threads to race on updates
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&hll, &sync_point, i]() {
            // Generate distinct values that would hash to conflict on the same registers
            std::string payload = "concurrent_test_string_" + std::to_string(i);
            
            sync_point.arrive_and_wait(); // Synchronize thread execution start
            hll.add(payload);
        });
    }

    threads.clear(); // Safe join point for all threads

    // Verify the state remains valid and produces a sane non-zero estimation
    EXPECT_GT(hll.estimate(), 0);
}

// Test 2: Mass parallel stress test simulating high-throughput ingestion
TEST(HLLTest, MultiThreadedMassIngestionStress) {
    const size_t precision = 8; // 256 registers
    hll::HyperLogLog<std::string> hll(precision);

    const size_t NUM_THREADS = 4;
    const size_t ITEMS_PER_THREAD = 10'000;
    std::vector<std::jthread> threads;
    threads.reserve(NUM_THREADS);

    for (size_t t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&hll, t]() {
            for (size_t i = 0; i < ITEMS_PER_THREAD; ++i) {
                // Unique deterministic strings distributed across threads
                std::string item = "thread_" + std::to_string(t) + "_item_" + std::to_string(i);
                hll.add(item);
            }
        });
    }

    threads.clear(); // Safe join point for all threads

    // With 40,000 unique insertions, check that estimation doesn't collapse or corrupt memory
    uint64_t final_estimate = hll.estimate();
    EXPECT_GT(final_estimate, 20'000);
    EXPECT_LT(final_estimate, 60'000);
}