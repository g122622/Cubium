#include <gtest/gtest.h>
#include "common/util/concurrent/ReentrantAreaLock.hpp"
#include <thread>
#include <vector>
#include <atomic>

using namespace mc::concurrent;
using namespace mc;  // 引入 i64 等类型别名

// ============================================================================
// 测试夹具
// ============================================================================

class ReentrantAreaLockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // shift=6 意味着 64x64 区块为一个区域
        lock_ = std::make_unique<ReentrantAreaLock>(6);
    }

    void TearDown() override {
        lock_.reset();
    }

    std::unique_ptr<ReentrantAreaLock> lock_;
};

// ============================================================================
// 基础功能测试
// ============================================================================

TEST_F(ReentrantAreaLockTest, SingleLockUnlock) {
    // 锁定单个位置
    auto* node = lock_->lock(0, 0);
    ASSERT_NE(node, nullptr);

    // 检查是否持有锁
    EXPECT_TRUE(lock_->isHeldByCurrentThread(0, 0));

    // 解锁
    lock_->unlock(node);

    // 检查是否不再持有锁
    EXPECT_FALSE(lock_->isHeldByCurrentThread(0, 0));
}

TEST_F(ReentrantAreaLockTest, TryLockSuccess) {
    // 尝试锁定单个位置
    auto* node = lock_->tryLock(0, 0);
    ASSERT_NE(node, nullptr);

    // 检查是否持有锁
    EXPECT_TRUE(lock_->isHeldByCurrentThread(0, 0));

    // 解锁
    lock_->unlock(node);
}

TEST_F(ReentrantAreaLockTest, TryLockFailure) {
    // 在另一个线程中锁定
    std::atomic<bool> locked{false};
    std::atomic<bool> canUnlock{false};

    std::thread otherThread([&]() {
        auto* node = lock_->lock(0, 0);
        locked = true;

        // 等待主线程尝试获取锁
        while (!canUnlock.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        lock_->unlock(node);
    });

    // 等待另一个线程获取锁
    while (!locked.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    // 主线程尝试获取同一个位置的锁应该失败
    auto* node = lock_->tryLock(0, 0);
    EXPECT_EQ(node, nullptr);

    // 允许另一个线程解锁
    canUnlock = true;
    otherThread.join();

    // 现在应该可以获取锁
    node = lock_->tryLock(0, 0);
    ASSERT_NE(node, nullptr);
    lock_->unlock(node);
}

TEST_F(ReentrantAreaLockTest, RadiusLock) {
    // 锁定半径为 2 的范围
    // shift=6，所以 section 大小是 64x64
    // lock(10, 10, 2) 锁定区块 8-12 范围，都在 section (0, 0) 中
    auto* node = lock_->lock(10, 10, 2);
    ASSERT_NE(node, nullptr);

    // 检查范围内的所有位置是否持有锁
    EXPECT_TRUE(lock_->isHeldByCurrentThread(10, 10, 2));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(8, 8));   // 范围边界
    EXPECT_TRUE(lock_->isHeldByCurrentThread(12, 12)); // 范围边界

    // 由于 shift=6，section 边界是 64 的倍数
    // 8-12 都在 section (0, 0) 中，所以整个 section 被锁定
    // (7, 7) 也在 section (0, 0) 中，所以也被锁定
    EXPECT_TRUE(lock_->isHeldByCurrentThread(7, 7));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(0, 0));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(63, 63));

    // 检查不同 section 的位置
    // (64, 0) 在 section (1, 0) 中，不应该被锁定
    EXPECT_FALSE(lock_->isHeldByCurrentThread(64, 0));
    EXPECT_FALSE(lock_->isHeldByCurrentThread(0, 64));
    EXPECT_FALSE(lock_->isHeldByCurrentThread(64, 64));

    lock_->unlock(node);
}

TEST_F(ReentrantAreaLockTest, RectangleLock) {
    // 锁定矩形区域
    // shift=6，section 大小 64x64
    // lock(0, 0, 100, 100) 会锁定 section (0,0), (1,0), (0,1), (1,1)
    auto* node = lock_->lock(0, 0, 100, 100);
    ASSERT_NE(node, nullptr);

    // 检查范围内的位置
    EXPECT_TRUE(lock_->isHeldByCurrentThread(50, 50));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(0, 0));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(100, 100));

    // 检查 section 边界内的位置（都在锁定的 section 内）
    EXPECT_TRUE(lock_->isHeldByCurrentThread(127, 127)); // section (1,1) 的右下角

    // 检查 section 边界外的位置
    // 128 在 section (2,x) 或 (x,2) 中，不在锁定的 section 内
    EXPECT_FALSE(lock_->isHeldByCurrentThread(128, 50));
    EXPECT_FALSE(lock_->isHeldByCurrentThread(50, 128));

    lock_->unlock(node);
}

// ============================================================================
// 可重入测试
// ============================================================================

TEST_F(ReentrantAreaLockTest, ReentrantLock) {
    // 第一次锁定
    auto* node1 = lock_->lock(0, 0);
    ASSERT_NE(node1, nullptr);

    // 可重入锁定同一位置应该成功
    auto* node2 = lock_->lock(0, 0);
    ASSERT_NE(node2, nullptr);

    // 仍然持有锁
    EXPECT_TRUE(lock_->isHeldByCurrentThread(0, 0));

    // 解锁（第二个节点是可重入节点）
    lock_->unlock(node2);

    // 仍然持有锁（因为第一个节点还未解锁）
    EXPECT_TRUE(lock_->isHeldByCurrentThread(0, 0));

    // 解锁第一个节点
    lock_->unlock(node1);

    // 现在不再持有锁
    EXPECT_FALSE(lock_->isHeldByCurrentThread(0, 0));
}

// ============================================================================
// 区域位移测试
// ============================================================================

TEST_F(ReentrantAreaLockTest, CoordinateShift) {
    // shift=6，所以 64x64 区块为一个区域
    // 锁定 (0, 0) 应该锁定整个 (0..63, 0..63) 范围
    auto* node = lock_->lock(0, 0);
    ASSERT_NE(node, nullptr);

    // 同一区域内的位置应该被锁定
    EXPECT_TRUE(lock_->isHeldByCurrentThread(0, 0));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(63, 63));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(32, 32));

    // 不同区域的位置应该未被锁定
    EXPECT_FALSE(lock_->isHeldByCurrentThread(64, 0));
    EXPECT_FALSE(lock_->isHeldByCurrentThread(0, 64));
    EXPECT_FALSE(lock_->isHeldByCurrentThread(64, 64));

    lock_->unlock(node);
}

TEST_F(ReentrantAreaLockTest, CrossRegionLock) {
    // 锁定跨越多个区域的范围
    // shift=6，section 大小 64x64
    // lock(0, 0, 128, 128) 会锁定 section (0,0), (1,0), (2,0), (0,1), (1,1), (2,1), (0,2), (1,2), (2,2)
    auto* node = lock_->lock(0, 0, 128, 128);
    ASSERT_NE(node, nullptr);

    // 所有范围内的位置都应该被锁定
    EXPECT_TRUE(lock_->isHeldByCurrentThread(0, 0));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(64, 64));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(128, 128));

    // 128 在 section (2, 2) 内（section 边界是 64 的倍数）
    // section 2 的范围是 128-191
    EXPECT_TRUE(lock_->isHeldByCurrentThread(128, 64));
    EXPECT_TRUE(lock_->isHeldByCurrentThread(64, 128));

    // 检查 section 边界外的位置
    // 192 在 section (3, x) 中，不在锁定的 section 内
    EXPECT_FALSE(lock_->isHeldByCurrentThread(192, 64));
    EXPECT_FALSE(lock_->isHeldByCurrentThread(64, 192));

    lock_->unlock(node);
}

// ============================================================================
// IntPairUtil 测试
// ============================================================================

TEST(IntPairUtilTest, KeyEncoding) {
    // 测试正数
    i64 key = IntPairUtil::key(100, 200);
    EXPECT_EQ(IntPairUtil::left(key), 100);
    EXPECT_EQ(IntPairUtil::right(key), 200);

    // 测试负数
    key = IntPairUtil::key(-100, -200);
    EXPECT_EQ(IntPairUtil::left(key), -100);
    EXPECT_EQ(IntPairUtil::right(key), -200);

    // 测试混合
    key = IntPairUtil::key(-100, 200);
    EXPECT_EQ(IntPairUtil::left(key), -100);
    EXPECT_EQ(IntPairUtil::right(key), 200);
}

// ============================================================================
// 多线程测试
// ============================================================================

TEST_F(ReentrantAreaLockTest, MultiThreadedLocking) {
    const int numThreads = 4;
    const int iterations = 100;
    std::atomic<int> successCount{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < iterations; ++i) {
                // 每个线程锁定不同的区域
                int baseX = t * 1000;
                int baseZ = i * 100;
                auto* node = lock_->lock(baseX, baseZ, baseX + 10, baseZ + 10);
                if (node != nullptr) {
                    // 确认持有锁
                    if (lock_->isHeldByCurrentThread(baseX, baseZ)) {
                        successCount++;
                    }
                    lock_->unlock(node);
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 所有线程都应该成功锁定
    EXPECT_EQ(successCount.load(), numThreads * iterations);
}

TEST_F(ReentrantAreaLockTest, ContendedLock) {
    std::atomic<bool> running{true};
    std::atomic<int> lockCount{0};

    // 线程1：持续锁定和解锁
    std::thread thread1([&]() {
        while (running.load(std::memory_order_acquire)) {
            auto* node = lock_->tryLock(0, 0);
            if (node != nullptr) {
                lockCount++;
                lock_->unlock(node);
            }
        }
    });

    // 线程2：持续锁定和解锁
    std::thread thread2([&]() {
        while (running.load(std::memory_order_acquire)) {
            auto* node = lock_->tryLock(0, 0);
            if (node != nullptr) {
                lockCount++;
                lock_->unlock(node);
            }
        }
    });

    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    thread1.join();
    thread2.join();

    // 应该有成功的锁定
    EXPECT_GT(lockCount.load(), 0);
}
