#include <gtest/gtest.h>
#include "common/world/chunk/ThreadedTicketLevelPropagator.hpp"
#include "common/util/concurrent/ReentrantAreaLock.hpp"
#include <thread>
#include <vector>
#include <atomic>

using namespace mc::world;
using namespace mc::concurrent;
using namespace mc;  // For i32, u64, u8 etc.

// ============================================================================
// Test Fixture
// ============================================================================

class ThreadedTicketLevelPropagatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        propagator_ = std::make_unique<ThreadedTicketLevelPropagator>();
    }

    void TearDown() override {
        propagator_.reset();
    }

    std::unique_ptr<ThreadedTicketLevelPropagator> propagator_;
};

// ============================================================================
// Basic Tests
// ============================================================================

TEST_F(ThreadedTicketLevelPropagatorTest, ConstantsAreCorrect) {
    EXPECT_EQ(ThreadedTicketLevelPropagator::SECTION_SHIFT, 6);
    EXPECT_EQ(ThreadedTicketLevelPropagator::SECTION_SIZE, 64);
    EXPECT_EQ(ThreadedTicketLevelPropagator::SECTION_MASK, 63);
    EXPECT_EQ(ThreadedTicketLevelPropagator::LEVEL_BITS, 6);
    EXPECT_EQ(ThreadedTicketLevelPropagator::LEVEL_COUNT, 64);
    EXPECT_EQ(ThreadedTicketLevelPropagator::MIN_SOURCE_LEVEL, 1);
    EXPECT_EQ(ThreadedTicketLevelPropagator::MAX_SOURCE_LEVEL, 62);
    EXPECT_EQ(ThreadedTicketLevelPropagator::MAX_LEVEL, 64);
}

TEST_F(ThreadedTicketLevelPropagatorTest, InitialLevelIsMax) {
    // Uninitialized positions should return MAX_LEVEL
    EXPECT_EQ(propagator_->getLevel(0, 0), ThreadedTicketLevelPropagator::MAX_LEVEL);
    EXPECT_EQ(propagator_->getLevel(100, 100), ThreadedTicketLevelPropagator::MAX_LEVEL);
    EXPECT_EQ(propagator_->getLevel(-50, 50), ThreadedTicketLevelPropagator::MAX_LEVEL);
}

TEST_F(ThreadedTicketLevelPropagatorTest, SetSourceCreatesSection) {
    propagator_->setSource(0, 0, 31);

    // Section (0, 0) should be created
    ThreadedTicketLevelPropagator::Section* section = propagator_->getSection(0, 0);
    ASSERT_NE(section, nullptr);
    EXPECT_EQ(section->sectionX, 0);
    EXPECT_EQ(section->sectionZ, 0);
}

TEST_F(ThreadedTicketLevelPropagatorTest, SetSourceWithinBounds) {
    // Valid source levels: 1-62
    propagator_->setSource(0, 0, 1);
    propagator_->setSource(0, 1, 31);
    propagator_->setSource(0, 2, 62);

    // Invalid source levels should be ignored
    propagator_->setSource(0, 3, 0);   // Too low
    propagator_->setSource(0, 4, 63);  // Too high

    ThreadedTicketLevelPropagator::Section* section = propagator_->getSection(0, 0);
    ASSERT_NE(section, nullptr);

    // Only valid sources should be queued
    EXPECT_EQ(section->queuedSources.size(), 3u);
}

TEST_F(ThreadedTicketLevelPropagatorTest, RemoveSourceFromNonExistentSection) {
    // Should not crash
    propagator_->removeSource(0, 0);
    propagator_->removeSource(100, 100);
}

TEST_F(ThreadedTicketLevelPropagatorTest, SetSameSourceTwice) {
    propagator_->setSource(0, 0, 31);
    propagator_->setSource(0, 0, 31);  // Same source again

    ThreadedTicketLevelPropagator::Section* section = propagator_->getSection(0, 0);
    ASSERT_NE(section, nullptr);

    // Should only queue one update (replaced)
    EXPECT_EQ(section->queuedSources.size(), 1u);
}

TEST_F(ThreadedTicketLevelPropagatorTest, HasPendingUpdatesAfterSetSource) {
    EXPECT_FALSE(propagator_->hasPendingUpdates());

    propagator_->setSource(0, 0, 31);

    EXPECT_TRUE(propagator_->hasPendingUpdates());
}

TEST_F(ThreadedTicketLevelPropagatorTest, HasNoPendingUpdatesAfterRemoveNonExistent) {
    EXPECT_FALSE(propagator_->hasPendingUpdates());

    propagator_->removeSource(0, 0);  // Non-existent section

    EXPECT_FALSE(propagator_->hasPendingUpdates());
}

// ============================================================================
// Section Tests
// ============================================================================

TEST_F(ThreadedTicketLevelPropagatorTest, SectionLocalIndex) {
    // Test local index calculation
    // Index = (x & 63) | ((z & 63) << 6)

    EXPECT_EQ(ThreadedTicketLevelPropagator::Section::getLocalIndex(0, 0), 0);
    EXPECT_EQ(ThreadedTicketLevelPropagator::Section::getLocalIndex(1, 0), 1);
    EXPECT_EQ(ThreadedTicketLevelPropagator::Section::getLocalIndex(0, 1), 64);  // 0 | (1 << 6)

    // Within section bounds
    EXPECT_EQ(ThreadedTicketLevelPropagator::Section::getLocalIndex(63, 0), 63);
    EXPECT_EQ(ThreadedTicketLevelPropagator::Section::getLocalIndex(0, 63), 4032);  // 0 | (63 << 6)
    EXPECT_EQ(ThreadedTicketLevelPropagator::Section::getLocalIndex(63, 63), 4095); // 63 | (63 << 6)

    // Outside section wraps within section (masking)
    EXPECT_EQ(ThreadedTicketLevelPropagator::Section::getLocalIndex(64, 0), 0);     // 64 & 63 = 0
    EXPECT_EQ(ThreadedTicketLevelPropagator::Section::getLocalIndex(0, 64), 0);     // 64 & 63 = 0, shifted
    EXPECT_EQ(ThreadedTicketLevelPropagator::Section::getLocalIndex(128, 128), 0);  // Both wrap
}

TEST_F(ThreadedTicketLevelPropagatorTest, SectionLevelStorage) {
    ThreadedTicketLevelPropagator::Section section(0, 0);

    // Test level get/set
    section.setLevel(0, 31);
    EXPECT_EQ(section.getLevel(0), 31u);

    section.setLevel(100, 45);
    EXPECT_EQ(section.getLevel(100), 45u);

    // Test source level get/set
    section.setSourceLevel(0, 20);
    EXPECT_EQ(section.getSourceLevel(0), 20u);

    // Test combined
    section.setLevelAndSource(50, 40, 25);
    EXPECT_EQ(section.getLevel(50), 40u);
    EXPECT_EQ(section.getSourceLevel(50), 25u);
}

// ============================================================================
// Propagation Tests
// ============================================================================

TEST_F(ThreadedTicketLevelPropagatorTest, SimplePropagation) {
    // Set up a source at level 31
    propagator_->setSource(0, 0, 31);

    // Create locks
    ReentrantAreaLock schedulingLock(6);

    // Perform update
    std::vector<std::pair<u64, u8>> updatedPositions;
    bool updated = propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    EXPECT_TRUE(updated);
    EXPECT_FALSE(updatedPositions.empty());

    // The source position should have level 31
    EXPECT_EQ(propagator_->getLevel(0, 0), 31);

    // Neighbors should have level 32 (31 + 1)
    EXPECT_EQ(propagator_->getLevel(1, 0), 32);
    EXPECT_EQ(propagator_->getLevel(-1, 0), 32);
    EXPECT_EQ(propagator_->getLevel(0, 1), 32);
    EXPECT_EQ(propagator_->getLevel(0, -1), 32);

    // Diagonal neighbors should have level 32
    EXPECT_EQ(propagator_->getLevel(1, 1), 32);
    EXPECT_EQ(propagator_->getLevel(-1, 1), 32);
    EXPECT_EQ(propagator_->getLevel(1, -1), 32);
    EXPECT_EQ(propagator_->getLevel(-1, -1), 32);
}

TEST_F(ThreadedTicketLevelPropagatorTest, TwoSourcesTakeMinimum) {
    // Two sources at different positions and levels
    propagator_->setSource(0, 0, 31);
    propagator_->setSource(2, 0, 30);  // Stronger source

    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;

    // Update section (0, 0) first
    propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    // Position (1, 0) is distance 1 from (0, 0) and distance 1 from (2, 0)
    // From (0, 0): level 32
    // From (2, 0): level 31
    // Should take minimum = 31
    EXPECT_EQ(propagator_->getLevel(1, 0), 31);
}

TEST_F(ThreadedTicketLevelPropagatorTest, RemoveSourceCausesDecrease) {
    // Set up a source
    propagator_->setSource(0, 0, 31);

    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;

    propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);
    EXPECT_EQ(propagator_->getLevel(0, 0), 31);

    // Remove the source
    propagator_->removeSource(0, 0);

    updatedPositions.clear();
    propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    // After removal, level should be MAX_LEVEL (no sources)
    EXPECT_EQ(propagator_->getLevel(0, 0), ThreadedTicketLevelPropagator::MAX_LEVEL);
}

TEST_F(ThreadedTicketLevelPropagatorTest, LevelChangeCallback) {
    std::vector<std::tuple<i32, i32, i32, i32>> changes;

    propagator_->setLevelChangeCallback([&changes](i32 x, i32 z, i32 oldLevel, i32 newLevel) {
        changes.emplace_back(x, z, oldLevel, newLevel);
    });

    propagator_->setSource(0, 0, 31);

    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;
    propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    EXPECT_FALSE(changes.empty());
}

// ============================================================================
// Multi-Section Tests
// ============================================================================

TEST_F(ThreadedTicketLevelPropagatorTest, CrossSectionPropagation) {
    // Source near section boundary
    propagator_->setSource(63, 63, 31);  // Corner of section (0, 0)

    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;

    propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    // Should propagate to neighbor sections
    // Position (64, 63) is in section (1, 0)
    // Position (63, 64) is in section (0, 1)
    // Position (64, 64) is in section (1, 1)

    EXPECT_EQ(propagator_->getLevel(63, 63), 31);
    EXPECT_EQ(propagator_->getLevel(64, 63), 32);
    EXPECT_EQ(propagator_->getLevel(63, 64), 32);
    EXPECT_EQ(propagator_->getLevel(64, 64), 32);
}

TEST_F(ThreadedTicketLevelPropagatorTest, MultipleSectionsCreated) {
    // Sources in different sections
    propagator_->setSource(0, 0, 31);      // Section (0, 0)
    propagator_->setSource(64, 0, 31);     // Section (1, 0)
    propagator_->setSource(0, 64, 31);     // Section (0, 1)
    propagator_->setSource(64, 64, 31);    // Section (1, 1)

    EXPECT_NE(propagator_->getSection(0, 0), nullptr);
    EXPECT_NE(propagator_->getSection(1, 0), nullptr);
    EXPECT_NE(propagator_->getSection(0, 1), nullptr);
    EXPECT_NE(propagator_->getSection(1, 1), nullptr);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ThreadedTicketLevelPropagatorTest, NegativeCoordinates) {
    // Sources at negative coordinates
    propagator_->setSource(-1, -1, 31);
    propagator_->setSource(-100, -100, 30);

    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;

    // Section for (-1, -1) is (-1, -1)
    propagator_->performUpdate(-1, -1, schedulingLock, updatedPositions);

    EXPECT_EQ(propagator_->getLevel(-1, -1), 31);
}

TEST_F(ThreadedTicketLevelPropagatorTest, MaxSourceLevel) {
    // Maximum source level (62)
    propagator_->setSource(0, 0, 62);

    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;

    propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    EXPECT_EQ(propagator_->getLevel(0, 0), 62);
}

TEST_F(ThreadedTicketLevelPropagatorTest, MinSourceLevel) {
    // Minimum source level (1)
    propagator_->setSource(0, 0, 1);

    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;

    propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    EXPECT_EQ(propagator_->getLevel(0, 0), 1);
}

TEST_F(ThreadedTicketLevelPropagatorTest, PerformUpdateOnEmptySection) {
    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;

    // No sources set, should return false
    bool updated = propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    EXPECT_FALSE(updated);
    EXPECT_TRUE(updatedPositions.empty());
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(ThreadedTicketLevelPropagatorTest, ConcurrentSetSource) {
    const int numThreads = 4;
    const int iterations = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                int x = t * 100 + i;
                int z = i;
                propagator_->setSource(x, z, 31);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Should not crash, and all sections should be created
    for (int t = 0; t < numThreads; ++t) {
        int x = t * 100;
        int sectionX = x >> 6;
        EXPECT_NE(propagator_->getSection(sectionX, 0), nullptr);
    }
}

TEST_F(ThreadedTicketLevelPropagatorTest, ConcurrentGetLevel) {
    // Set up some sources
    propagator_->setSource(0, 0, 31);

    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;
    propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    const int numThreads = 4;
    const int iterations = 1000;
    std::atomic<int> successCount{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, &successCount, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                i32 level = propagator_->getLevel(0, 0);
                if (level == 31) {
                    successCount++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successCount.load(), numThreads * iterations);
}

// ============================================================================
// Performance Characteristics Test
// ============================================================================

TEST_F(ThreadedTicketLevelPropagatorTest, LargeAreaPropagation) {
    // Set a source that should propagate to a large area
    propagator_->setSource(0, 0, 31);

    ReentrantAreaLock schedulingLock(6);
    std::vector<std::pair<u64, u8>> updatedPositions;

    propagator_->performUpdate(0, 0, schedulingLock, updatedPositions);

    // Level 31 should propagate to radius 31
    // Positions within radius 31 should have level <= 31 + 31 = 62
    // Positions beyond should have MAX_LEVEL

    // Position at radius 31 should have level 62
    EXPECT_EQ(propagator_->getLevel(31, 0), 62);
    EXPECT_EQ(propagator_->getLevel(0, 31), 62);

    // Position at radius 32 should have level 63 or MAX_LEVEL
    // (depends on whether propagation reaches there)
    // With source level 31, propagation extends to level 63 at distance 32
    EXPECT_EQ(propagator_->getLevel(32, 0), 63);
}
