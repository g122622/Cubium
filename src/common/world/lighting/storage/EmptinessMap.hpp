#pragma once

#include "../../../core/Types.hpp"
#include <vector>
#include <cstring>

namespace mc {

// 前向声明
class IChunk;

/**
 * @brief 空区块段映射
 *
 * 追踪每个区块中哪些区块段是全空气的。
 * 用于光照引擎快速跳过空区块段，避免不必要的光照计算。
 *
 * 参考: Starlight 的 EmptinessMap 概念
 */
class EmptinessMap {
public:
    /**
     * @brief 构造函数
     *
     * @param minSection 最小区块段Y坐标
     * @param maxSection 最大区块段Y坐标
     */
    EmptinessMap(i32 minSection, i32 maxSection);

    /**
     * @brief 默认构造函数（需要后续调用 setHeightRange）
     */
    EmptinessMap() = default;

    /**
     * @brief 设置高度范围
     */
    void setHeightRange(i32 minSection, i32 maxSection);

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 检查区块段是否为空
     *
     * @param sectionY 区块段Y坐标（世界坐标）
     * @return 如果区块段全空气返回true
     */
    [[nodiscard]] bool isSectionEmpty(i32 sectionY) const;

    /**
     * @brief 检查区块段是否为空（使用索引）
     *
     * @param sectionIndex 区块段索引（从0开始）
     * @return 如果区块段全空气返回true
     */
    [[nodiscard]] bool isSectionEmptyByIndex(i32 sectionIndex) const;

    /**
     * @brief 设置区块段是否为空
     *
     * @param sectionY 区块段Y坐标
     * @param empty 是否为空
     */
    void setSectionEmpty(i32 sectionY, bool empty);

    /**
     * @brief 检查整个区块是否为空
     */
    [[nodiscard]] bool isChunkEmpty() const;

    // ========================================================================
    // 批量操作
    // ========================================================================

    /**
     * @brief 从区块更新空区块段映射
     *
     * @param chunk 区块指针
     * @return 如果映射发生变化返回true
     */
    bool updateFromChunk(const IChunk& chunk);

    /**
     * @brief 重置所有区块段为非空
     */
    void reset();

    /**
     * @brief 设置所有区块段为空
     */
    void setAllEmpty();

    // ========================================================================
    // 访问器
    // ========================================================================

    /**
     * @brief 获取最小区块段Y
     */
    [[nodiscard]] i32 getMinSection() const { return m_minSection; }

    /**
     * @brief 获取最大区块段Y
     */
    [[nodiscard]] i32 getMaxSection() const { return m_maxSection; }

    /**
     * @brief 获取区块段数量
     */
    [[nodiscard]] i32 getSectionCount() const { return m_sectionCount; }

private:
    i32 m_minSection = 0;
    i32 m_maxSection = 15;
    i32 m_sectionCount = 16;
    std::vector<u8> m_sectionEmpty;  // 0 = 有方块，1 = 空（使用 u8 代替 bool 以支持 data()）

    /**
     * @brief 检查区块段索引是否有效
     */
    [[nodiscard]] bool isValidSectionIndex(i32 sectionIndex) const {
        return sectionIndex >= 0 && sectionIndex < m_sectionCount;
    }

    /**
     * @brief 区块段Y转索引
     */
    [[nodiscard]] i32 sectionYToIndex(i32 sectionY) const {
        return sectionY - m_minSection;
    }
};

// ============================================================================
// 内联实现
// ============================================================================

inline bool EmptinessMap::isSectionEmpty(i32 sectionY) const {
    i32 index = sectionYToIndex(sectionY);
    if (!isValidSectionIndex(index)) {
        return true;  // 超出范围视为空
    }
    return m_sectionEmpty[static_cast<size_t>(index)] != 0;
}

inline bool EmptinessMap::isSectionEmptyByIndex(i32 sectionIndex) const {
    if (!isValidSectionIndex(sectionIndex)) {
        return true;
    }
    return m_sectionEmpty[static_cast<size_t>(sectionIndex)] != 0;
}

inline void EmptinessMap::setSectionEmpty(i32 sectionY, bool empty) {
    i32 index = sectionYToIndex(sectionY);
    if (isValidSectionIndex(index)) {
        m_sectionEmpty[static_cast<size_t>(index)] = empty ? 1 : 0;
    }
}

inline bool EmptinessMap::isChunkEmpty() const {
    for (u8 empty : m_sectionEmpty) {
        if (empty == 0) {
            return false;
        }
    }
    return true;
}

} // namespace mc
