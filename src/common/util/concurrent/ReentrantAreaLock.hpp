#pragma once

#include "common/core/Types.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <unordered_map>

namespace mc::concurrent {

/**
 * @brief 将两个 int32 打包成一个 int64
 */
struct IntPairUtil {
    static constexpr i64 key(i32 left, i32 right) {
        return (static_cast<i64>(right) << 32) | (static_cast<u32>(left));
    }

    static constexpr i32 left(i64 key) {
        return static_cast<i32>(key);
    }

    static constexpr i32 right(i64 key) {
        return static_cast<i32>(key >> 32);
    }
};

/**
 * @brief 多线程安全队列
 *
 * 参考 Moonrise 的 MultiThreadedQueue，支持：
 * - MT-Safe 的入队/出队
 * - preventAdds/allowAdds 阻塞/恢复添加
 */
template<typename E>
class MultiThreadedQueue {
public:
    struct Node {
        E element;
        std::atomic<Node*> next{nullptr};

        explicit Node(E elem) : element(std::move(elem)) {}
        Node() = default;
    };

private:
    std::atomic<Node*> m_head;
    std::atomic<Node*> m_tail;
    std::atomic<bool> m_addsBlocked{false};

public:
    MultiThreadedQueue() {
        auto* dummy = new Node();
        m_head.store(dummy, std::memory_order_relaxed);
        m_tail.store(dummy, std::memory_order_relaxed);
    }

    ~MultiThreadedQueue() {
        clear();
        delete m_head.load(std::memory_order_relaxed);
    }

    /**
     * @brief 添加元素到队列尾部
     * @return true 如果成功添加，false 如果队列已阻止添加
     */
    bool add(E element) {
        if (m_addsBlocked.load(std::memory_order_acquire)) {
            return false;
        }

        auto* node = new Node(std::move(element));

        Node* tail = m_tail.load(std::memory_order_acquire);
        while (true) {
            Node* next = tail->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                if (tail->next.compare_exchange_weak(next, node,
                    std::memory_order_release, std::memory_order_relaxed)) {
                    m_tail.compare_exchange_weak(tail, node,
                        std::memory_order_release, std::memory_order_relaxed);
                    return true;
                }
            } else {
                m_tail.compare_exchange_weak(tail, next,
                    std::memory_order_release, std::memory_order_relaxed);
            }
        }
    }

    /**
     * @brief 从队列头部移除并返回元素
     * @return 头部元素，如果队列为空返回空值
     */
    bool poll(E& outElement) {
        Node* head = m_head.load(std::memory_order_acquire);
        while (true) {
            Node* next = head->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                return false;
            }

            E& element = next->element;
            if (m_head.compare_exchange_weak(head, next,
                std::memory_order_release, std::memory_order_relaxed)) {
                outElement = std::move(element);
                delete head;
                return true;
            }
        }
    }

    /**
     * @brief 阻止添加新元素
     * @return true 如果成功阻止，false 如果已经阻止
     */
    bool preventAdds() {
        bool expected = false;
        if (m_addsBlocked.compare_exchange_strong(expected, true,
            std::memory_order_release, std::memory_order_relaxed)) {
            return true;
        }
        return false;
    }

    /**
     * @brief 允许添加新元素
     */
    void allowAdds() {
        m_addsBlocked.store(false, std::memory_order_release);
    }

    /**
     * @brief 检查是否为空
     */
    bool isEmpty() const {
        Node* head = m_head.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);
        return next == nullptr;
    }

    /**
     * @brief 清空队列
     */
    void clear() {
        E element;
        while (poll(element)) {
            // 清空
        }
    }
};

/**
 * @brief 可重入区域锁
 *
 * 基于 Moonrise 的 ReentrantAreaLock 实现。
 * 允许锁定一个区域（由多个区块组成），支持可重入。
 *
 * 区域锁用于区块系统中的并发控制，例如：
 * - 锁定某个区块及其邻居范围进行操作
 * - 防止并发的区块状态修改
 *
 * 使用示例：
 * @code
 * ReentrantAreaLock lock(6);  // 64x64 区块为一个区域
 *
 * // 锁定单个区块
 * auto* node = lock.lock(chunkX, chunkZ);
 * // ... 操作 ...
 * lock.unlock(node);
 *
 * // 锁定区块范围（半径）
 * auto* node = lock.lock(centerX, centerZ, radius);
 * // ... 操作 ...
 * lock.unlock(node);
 * @endcode
 */
class ReentrantAreaLock {
public:
    /**
     * @brief 锁节点，代表一次锁定操作
     */
    class Node : public MultiThreadedQueue<std::thread::id> {
    public:
        friend class ReentrantAreaLock;

        Node() = default;

        [[nodiscard]] const std::vector<i64>& areaAffected() const { return m_areaAffected; }
        [[nodiscard]] std::thread::id thread() const { return m_thread; }

    private:
        explicit Node(ReentrantAreaLock* lock, std::vector<i64> areaAffected, std::thread::id thread)
            : m_lock(lock)
            , m_areaAffected(std::move(areaAffected))
            , m_thread(thread)
            , m_areaAffectedLen(0) {}

        ReentrantAreaLock* m_lock = nullptr;
        std::vector<i64> m_areaAffected;
        std::thread::id m_thread;
        size_t m_areaAffectedLen = 0;
    };

    /**
     * @brief 构造区域锁
     * @param coordinateShift 坐标位移位数，决定区域大小
     *        例如 shift=6 表示 64x64 区块为一个区域
     */
    explicit ReentrantAreaLock(int coordinateShift)
        : m_coordinateShift(coordinateShift) {}

    /**
     * @brief 检查当前线程是否持有指定位置的锁
     */
    [[nodiscard]] bool isHeldByCurrentThread(i32 x, i32 z) const {
        const auto currThread = std::this_thread::get_id();
        const i32 sectionX = x >> m_coordinateShift;
        const i32 sectionZ = z >> m_coordinateShift;
        const i64 coordinate = IntPairUtil::key(sectionX, sectionZ);

        std::lock_guard<std::mutex> lock(m_nodesMutex);
        auto it = m_nodes.find(coordinate);
        if (it == m_nodes.end()) {
            return false;
        }
        return it->second->m_thread == currThread;
    }

    /**
     * @brief 检查当前线程是否持有指定半径内的所有锁
     */
    [[nodiscard]] bool isHeldByCurrentThread(i32 centerX, i32 centerZ, i32 radius) const {
        return isHeldByCurrentThread(centerX - radius, centerZ - radius,
                                      centerX + radius, centerZ + radius);
    }

    /**
     * @brief 检查当前线程是否持有指定矩形区域内的所有锁
     */
    [[nodiscard]] bool isHeldByCurrentThread(i32 fromX, i32 fromZ, i32 toX, i32 toZ) const {
        if (fromX > toX || fromZ > toZ) {
            return false;
        }

        const auto currThread = std::this_thread::get_id();
        const i32 fromSectionX = fromX >> m_coordinateShift;
        const i32 fromSectionZ = fromZ >> m_coordinateShift;
        const i32 toSectionX = toX >> m_coordinateShift;
        const i32 toSectionZ = toZ >> m_coordinateShift;

        std::lock_guard<std::mutex> lock(m_nodesMutex);
        for (i32 currZ = fromSectionZ; currZ <= toSectionZ; ++currZ) {
            for (i32 currX = fromSectionX; currX <= toSectionX; ++currX) {
                const i64 coordinate = IntPairUtil::key(currX, currZ);
                auto it = m_nodes.find(coordinate);
                if (it == m_nodes.end() || it->second->m_thread != currThread) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * @brief 尝试锁定单个位置
     * @return 锁节点，如果成功；nullptr 如果失败
     */
    Node* tryLock(i32 x, i32 z) {
        return tryLock(x, z, x, z);
    }

    /**
     * @brief 尝试锁定指定半径范围
     * @return 锁节点，如果成功；nullptr 如果失败
     */
    Node* tryLock(i32 centerX, i32 centerZ, i32 radius) {
        return tryLock(centerX - radius, centerZ - radius,
                       centerX + radius, centerZ + radius);
    }

    /**
     * @brief 尝试锁定指定矩形区域
     * @return 锁节点，如果成功；nullptr 如果失败
     */
    Node* tryLock(i32 fromX, i32 fromZ, i32 toX, i32 toZ) {
        if (fromX > toX || fromZ > toZ) {
            return nullptr;
        }

        const auto currThread = std::this_thread::get_id();
        const i32 fromSectionX = fromX >> m_coordinateShift;
        const i32 fromSectionZ = fromZ >> m_coordinateShift;
        const i32 toSectionX = toX >> m_coordinateShift;
        const i32 toSectionZ = toZ >> m_coordinateShift;

        const size_t areaSize = static_cast<size_t>(toSectionX - fromSectionX + 1) *
                                static_cast<size_t>(toSectionZ - fromSectionZ + 1);
        auto* ret = new Node(this, std::vector<i64>(areaSize), currThread);
        ret->m_areaAffectedLen = 0;

        std::lock_guard<std::mutex> lock(m_nodesMutex);

        bool failed = false;

        // 尝试快速获取区域
        for (i32 currZ = fromSectionZ; currZ <= toSectionZ && !failed; ++currZ) {
            for (i32 currX = fromSectionX; currX <= toSectionX && !failed; ++currX) {
                const i64 coordinate = IntPairUtil::key(currX, currZ);

                auto [it, inserted] = m_nodes.try_emplace(coordinate, ret);
                if (inserted) {
                    ret->m_areaAffected[ret->m_areaAffectedLen++] = coordinate;
                    continue;
                }

                if (it->second->m_thread != currThread) {
                    failed = true;
                }
            }
        }

        if (!failed) {
            return ret;
        }

        // 失败，回滚
        for (size_t i = 0; i < ret->m_areaAffectedLen; ++i) {
            m_nodes.erase(ret->m_areaAffected[i]);
        }
        ret->m_areaAffectedLen = 0;

        // 唤醒等待者
        std::thread::id waiter;
        while (ret->poll(waiter)) {
            // 唤醒所有等待者
        }

        delete ret;
        return nullptr;
    }

    /**
     * @brief 锁定单个位置（阻塞直到成功）
     * @return 锁节点
     */
    Node* lock(i32 x, i32 z) {
        const auto currThread = std::this_thread::get_id();
        const i32 sectionX = x >> m_coordinateShift;
        const i32 sectionZ = z >> m_coordinateShift;
        const i64 coordinate = IntPairUtil::key(sectionX, sectionZ);

        auto* ret = new Node(this, std::vector<i64>(1), currThread);
        ret->m_areaAffectedLen = 0;

        for (u64 failures = 0; ; ++failures) {
            std::unique_lock<std::mutex> lock(m_nodesMutex);

            auto [it, inserted] = m_nodes.try_emplace(coordinate, ret);
            if (inserted) {
                ret->m_areaAffected[0] = coordinate;
                ret->m_areaAffectedLen = 1;
                return ret;
            }

            if (it->second->m_thread == currThread) {
                // 已经持有该位置的锁（可重入）
                delete ret;
                return new Node(this, {}, currThread);  // 返回空节点表示可重入
            }

            Node* park = it->second;
            lock.unlock();

            // 等待策略
            if (failures > 128) {
                std::this_thread::yield();
                std::this_thread::sleep_for(std::chrono::microseconds(100 * failures));
            } else if (failures > 32) {
                std::this_thread::yield();
            } else {
                // 自旋等待
                for (u64 i = 0; i < failures * 2; ++i) {
                    // CPU 自旋提示
                }
            }
        }
    }

    /**
     * @brief 锁定指定半径范围（阻塞直到成功）
     * @return 锁节点
     */
    Node* lock(i32 centerX, i32 centerZ, i32 radius) {
        return lock(centerX - radius, centerZ - radius,
                    centerX + radius, centerZ + radius);
    }

    /**
     * @brief 锁定指定矩形区域（阻塞直到成功）
     * @return 锁节点
     */
    Node* lock(i32 fromX, i32 fromZ, i32 toX, i32 toZ) {
        if (fromX > toX || fromZ > toZ) {
            return nullptr;
        }

        const auto currThread = std::this_thread::get_id();
        const i32 fromSectionX = fromX >> m_coordinateShift;
        const i32 fromSectionZ = fromZ >> m_coordinateShift;
        const i32 toSectionX = toX >> m_coordinateShift;
        const i32 toSectionZ = toZ >> m_coordinateShift;

        // 单个区域使用简单路径
        if (fromSectionX == toSectionX && fromSectionZ == toSectionZ) {
            return lock(fromX, fromZ);
        }

        const size_t areaSize = static_cast<size_t>(toSectionX - fromSectionX + 1) *
                                static_cast<size_t>(toSectionZ - fromSectionZ + 1);
        auto* ret = new Node(this, std::vector<i64>(areaSize), currThread);
        ret->m_areaAffectedLen = 0;

        for (u64 failures = 0; ; ++failures) {
            std::unique_lock<std::mutex> lock(m_nodesMutex);

            Node* park = nullptr;
            bool addedToArea = false;
            bool hasContention = false;  // True if any section is locked by another thread
            bool allAlreadyOwned = true; // True if all sections are already owned by current thread (reentrant)

            // 尝试快速获取区域
            for (i32 currZ = fromSectionZ; currZ <= toSectionZ; ++currZ) {
                for (i32 currX = fromSectionX; currX <= toSectionX; ++currX) {
                    const i64 coordinate = IntPairUtil::key(currX, currZ);

                    auto [it, inserted] = m_nodes.try_emplace(coordinate, ret);
                    if (inserted) {
                        addedToArea = true;
                        allAlreadyOwned = false;
                        ret->m_areaAffected[ret->m_areaAffectedLen++] = coordinate;
                        continue;
                    }

                    if (it->second->m_thread != currThread) {
                        park = it->second;
                        hasContention = true;
                        break;
                    }
                    // Already owned by current thread (reentrant), continue checking
                }
                if (park != nullptr) break;
            }

            // 检查是否失败（有冲突且有新锁定区域）
            if (hasContention && addedToArea) {
                // 失败，回滚
                for (size_t i = 0; i < ret->m_areaAffectedLen; ++i) {
                    m_nodes.erase(ret->m_areaAffected[i]);
                }
                ret->m_areaAffectedLen = 0;

                // 唤醒等待者
                std::thread::id waiter;
                while (ret->poll(waiter)) {
                    // 唤醒所有等待者
                }
            }

            if (park == nullptr) {
                // 成功获取锁
                // allAlreadyOwned=true 表示纯重入情况（无新锁定区域）
                return ret;
            }

            lock.unlock();

            // 等待策略
            if (failures > 128) {
                std::this_thread::yield();
                std::this_thread::sleep_for(std::chrono::microseconds(100 * failures));
            } else if (failures > 32) {
                std::this_thread::yield();
            } else {
                for (u64 i = 0; i < failures * 2; ++i) {
                    // CPU 自旋
                }
            }

            if (addedToArea) {
                ret->allowAdds();
            }
        }
    }

    /**
     * @brief 解锁
     */
    void unlock(Node* node) {
        if (node == nullptr) {
            return;
        }

        if (node->m_lock != this) {
            assert(false && "Unlock target lock mismatch");
            return;
        }

        const size_t areaAffectedLen = node->m_areaAffectedLen;
        if (areaAffectedLen == 0) {
            // 可重入情况，不需要解锁
            delete node;
            return;
        }

        std::lock_guard<std::mutex> lock(m_nodesMutex);

        // 从节点映射中移除
        for (size_t i = 0; i < areaAffectedLen; ++i) {
            m_nodes.erase(node->m_areaAffected[i]);
        }

        delete node;
    }

    /**
     * @brief 获取坐标位移
     */
    [[nodiscard]] int coordinateShift() const { return m_coordinateShift; }

private:
    int m_coordinateShift;
    mutable std::mutex m_nodesMutex;
    std::unordered_map<i64, Node*> m_nodes;
};

} // namespace mc::concurrent
