#pragma once

#include "../db/SectionKey.hpp"
#include <unordered_set>
#include <mutex>
#include <vector>

namespace mc::world::storage {

/**
 * @brief 脏Section追踪器
 *
 * 线程安全地追踪已修改但未保存的Section。
 * 与SectionManager配合使用，支持自动保存。
 */
class DirtyTracker {
public:
    /**
     * @brief 构造函数
     */
    DirtyTracker() = default;

    /**
     * @brief 析构函数
     */
    ~DirtyTracker() = default;

    // 禁止拷贝
    DirtyTracker(const DirtyTracker&) = delete;
    DirtyTracker& operator=(const DirtyTracker&) = delete;

    // 允许移动
    DirtyTracker(DirtyTracker&&) noexcept = default;
    DirtyTracker& operator=(DirtyTracker&&) noexcept = default;

    // ========== 脏标记操作 ==========

    /**
     * @brief 标记Section为脏
     * @param key Section标识
     * @return 是否为新标记（之前不脏）
     */
    bool markDirty(const SectionKey& key);

    /**
     * @brief 清除脏标记
     * @param key Section标识
     * @return 是否成功清除（之前是脏的）
     */
    bool clearDirty(const SectionKey& key);

    /**
     * @brief 检查Section是否为脏
     * @param key Section标识
     * @return 是否为脏
     */
    [[nodiscard]] bool isDirty(const SectionKey& key) const;

    /**
     * @brief 清除所有脏标记
     */
    void clearAll();

    // ========== 查询操作 ==========

    /**
     * @brief 获取脏Section数量
     */
    [[nodiscard]] size_t dirtyCount() const;

    /**
     * @brief 获取所有脏Section键
     */
    [[nodiscard]] std::vector<SectionKey> getDirtyKeys() const;

    /**
     * @brief 检查是否有脏Section
     */
    [[nodiscard]] bool hasDirty() const { return dirtyCount() > 0; }

    // ========== 批量操作 ==========

    /**
     * @brief 批量标记脏
     * @param keys Section标识列表
     * @return 新标记的数量
     */
    size_t markDirtyBatch(const std::vector<SectionKey>& keys);

    /**
     * @brief 批量清除脏标记
     * @param keys Section标识列表
     * @return 清除的数量
     */
    size_t clearDirtyBatch(const std::vector<SectionKey>& keys);

private:
    mutable std::mutex m_mutex;
    std::unordered_set<SectionKey, SectionKey::Hash> m_dirtySet;
};

} // namespace mc::world::storage
