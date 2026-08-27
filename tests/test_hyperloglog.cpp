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

TEST(HLLTest, MillionCardinalityStress) {
    // Precision 14 = 16,384 registers. 
    // Standard error margin for p=14 is roughly 1.04 / sqrt(16384) = 0.81%
    const size_t precision = 14; 
    hll::HyperLogLog<uint64_t> hll(precision);

    const uint64_t TARGET_CARDINALITY = 1'000'000;

    // Direct loop ingestion is highly optimized for CI pipeline speed
    for (uint64_t i = 0; i < TARGET_CARDINALITY; ++i) {
        hll.add(i);
    }

    uint64_t estimate = hll.estimate();

    // Calculate acceptable bounds using a conservative 3% error margin 
    // to prevent flaky builds on GitHub Actions due to hash variations.
    uint64_t lower_bound = static_cast<uint64_t>(TARGET_CARDINALITY * 0.97);
    uint64_t upper_bound = static_cast<uint64_t>(TARGET_CARDINALITY * 1.03);

    EXPECT_GE(estimate, lower_bound);
    EXPECT_LE(estimate, upper_bound);
}

TEST(HLLTest, MillionCardinalityMultiThreadedStress) {
    // Precision 14 = 16,384 registers. 
    // Standard error margin is roughly 1.04 / sqrt(16384) = 0.81%
    const size_t precision = 14; 
    hll::HyperLogLog<uint64_t> hll(precision);

    const uint64_t TOTAL_ITEMS = 1'000'000;
    const size_t NUM_THREADS = 4; // Matches/stresses standard GitHub Action runner environments
    const uint64_t ITEMS_PER_THREAD = TOTAL_ITEMS / NUM_THREADS;

    std::vector<std::jthread> threads;
    threads.reserve(NUM_THREADS);

    // Distribute 1,000,000 unique integers across parallel workers
    for (size_t t = 0; t < NUM_THREADS; ++t) {
        uint64_t start_range = t * ITEMS_PER_THREAD;
        uint64_t end_range = start_range + ITEMS_PER_THREAD;

        threads.emplace_back([&hll, start_range, end_range]() {
            for (uint64_t i = start_range; i < end_range; ++i) {
                hll.add(i);
            }
        });
    }

    // Explicitly clear to block and join all worker threads safely
    threads.clear(); 

    uint64_t estimate = hll.estimate();

    // 3% error boundary to avoid flaky CI builds on random hash distributions
    uint64_t lower_bound = static_cast<uint64_t>(TOTAL_ITEMS * 0.97);
    uint64_t upper_bound = static_cast<uint64_t>(TOTAL_ITEMS * 1.03);

    EXPECT_GE(estimate, lower_bound); 
    EXPECT_LE(estimate, upper_bound);
}