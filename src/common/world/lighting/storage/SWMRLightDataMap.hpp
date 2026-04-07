#pragma once

#include "SWMRNibbleArray.hpp"
#include "../../../util/NibbleArray.hpp"
#include "../engine/LightEngineUtils.hpp"
#include <unordered_map>
#include <memory>

namespace mc {

/**
 * @brief SWMR 光照数据映射
 *
 * 使用 SWMRNibbleArray 存储光照数据，支持：
 * - 写时复制 (Copy-on-Write)
 * - 单写多读 (Single Writer Multi Reader)
 * - 延迟初始化
 * - 状态管理
 *
 * 参考 Starlight 的 SWMRNibbleArray 设计
 */
template<typename Derived>
class SWMRLightDataMap {
public:
    SWMRLightDataMap() = default;

    virtual ~SWMRLightDataMap() = default;

    // 禁止拷贝
    SWMRLightDataMap(const SWMRLightDataMap&) = delete;
    SWMRLightDataMap& operator=(const SWMRLightDataMap&) = delete;

    // 允许移动
    SWMRLightDataMap(SWMRLightDataMap&&) = default;
    SWMRLightDataMap& operator=(SWMRLightDataMap&&) = default;

    // ========================================================================
    // 数据访问 - 更新侧
    // ========================================================================

    /**
     * @brief 获取区块段的光照数组（更新侧）
     *
     * @param sectionPos 区块段位置编码
     * @return 光照数组指针，如果不存在返回nullptr
     */
    [[nodiscard]] SWMRNibbleArray* getArrayUpdating(i64 sectionPos) {
        auto it = m_arrays.find(sectionPos);
        return it != m_arrays.end() ? &it->second : nullptr;
    }

    [[nodiscard]] const SWMRNibbleArray* getArrayUpdating(i64 sectionPos) const {
        auto it = m_arrays.find(sectionPos);
        return it != m_arrays.end() ? &it->second : nullptr;
    }

    // ========================================================================
    // 数据访问 - 可见侧（线程安全）
    // ========================================================================

    /**
     * @brief 获取区块段的光照数组（可见侧，线程安全）
     */
    [[nodiscard]] u8 getLightVisible(i64 worldPos) const {
        i64 sectionPos = worldToSection(worldPos);
        auto it = m_arrays.find(sectionPos);
        if (it == m_arrays.end()) {
            return 0;
        }

        i32 x, localY, z;
        extractNibbleIndices(worldPos, x, localY, z);

        return it->second.getVisible(x, localY, z);
    }

    // ========================================================================
    // 数据访问 - 兼容接口
    // ========================================================================

    /**
     * @brief 获取区块段的光照数组
     */
    [[nodiscard]] SWMRNibbleArray* getArray(i64 sectionPos) {
        return getArrayUpdating(sectionPos);
    }

    [[nodiscard]] const SWMRNibbleArray* getArray(i64 sectionPos) const {
        return getArrayUpdating(sectionPos);
    }

    /**
     * @brief 检查区块段是否有光照数据
     */
    [[nodiscard]] bool hasArray(i64 sectionPos) const {
        return m_arrays.find(sectionPos) != m_arrays.end();
    }

    /**
     * @brief 设置区块段的光照数组
     */
    void setArray(i64 sectionPos, SWMRNibbleArray array) {
        m_arrays[sectionPos] = std::move(array);
    }

    /**
     * @brief 创建或获取区块段的光照数组
     */
    [[nodiscard]] SWMRNibbleArray& getOrCreateArray(i64 sectionPos) {
        auto it = m_arrays.find(sectionPos);
        if (it != m_arrays.end()) {
            return it->second;
        }
        // 使用 try_emplace 避免拷贝
        auto result = m_arrays.try_emplace(sectionPos);
        return result.first->second;
    }

    /**
     * @brief 移除区块段的光照数组
     */
    void removeArray(i64 sectionPos) {
        m_arrays.erase(sectionPos);
    }

    // ========================================================================
    // 光照读写 - 更新侧
    // ========================================================================

    /**
     * @brief 获取光照等级（更新侧）
     */
    [[nodiscard]] u8 getLight(i64 worldPos) const {
        i64 sectionPos = worldToSection(worldPos);
        auto it = m_arrays.find(sectionPos);
        if (it == m_arrays.end()) {
            return 0;
        }

        i32 x, localY, z;
        extractNibbleIndices(worldPos, x, localY, z);

        return it->second.getUpdating(x, localY, z);
    }

    /**
     * @brief 设置光照等级（更新侧）
     */
    void setLight(i64 worldPos, u8 light) {
        i64 sectionPos = worldToSection(worldPos);
        auto it = m_arrays.find(sectionPos);
        if (it == m_arrays.end()) {
            return;
        }

        i32 x, localY, z;
        extractNibbleIndices(worldPos, x, localY, z);

        it->second.set(x, localY, z, light);
    }

    // ========================================================================
    // 同步操作
    // ========================================================================

    /**
     * @brief 同步所有更新到可见侧
     */
    void updateVisible() {
        for (auto& [pos, array] : m_arrays) {
            array.updateVisible();
        }
    }

    /**
     * @brief 同步指定区块段的更新到可见侧
     */
    void updateVisible(i64 sectionPos) {
        auto it = m_arrays.find(sectionPos);
        if (it != m_arrays.end()) {
            it->second.updateVisible();
        }
    }

    // ========================================================================
    // 批量操作
    // ========================================================================

    /**
     * @brief 填充区块段为指定光照等级
     */
    void fillSection(i64 sectionPos, u8 light) {
        auto it = m_arrays.find(sectionPos);
        if (it != m_arrays.end()) {
            if (light == 0) {
                it->second.setZero();
            } else if (light == 15) {
                it->second.setFull();
            } else {
                for (i32 i = 0; i < 4096; ++i) {
                    it->second.set(i, light);
                }
            }
        }
    }

    /**
     * @brief 将区块段设置为未初始化（全零但不分配内存）
     */
    void setSectionUninitialized(i64 sectionPos) {
        auto it = m_arrays.find(sectionPos);
        if (it != m_arrays.end()) {
            it->second.setUninitialized();
        }
    }

    /**
     * @brief 将区块段设置为 null
     */
    void setSectionNull(i64 sectionPos) {
        auto it = m_arrays.find(sectionPos);
        if (it != m_arrays.end()) {
            it->second.setNull();
        }
    }

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 检查区块段是否为 null
     */
    [[nodiscard]] bool isSectionNull(i64 sectionPos) const {
        auto it = m_arrays.find(sectionPos);
        return it == m_arrays.end() || it->second.isNullUpdating();
    }

    /**
     * @brief 检查区块段是否为未初始化
     */
    [[nodiscard]] bool isSectionUninitialized(i64 sectionPos) const {
        auto it = m_arrays.find(sectionPos);
        return it != m_arrays.end() && it->second.isUninitializedUpdating();
    }

    /**
     * @brief 检查区块段是否有数据
     */
    [[nodiscard]] bool hasSectionData(i64 sectionPos) const {
        auto it = m_arrays.find(sectionPos);
        return it != m_arrays.end() && it->second.isInitializedUpdating();
    }

    // ========================================================================
    // 复制和转换
    // ========================================================================

    /**
     * @brief 从 NibbleArray 创建
     */
    void setFromArray(i64 sectionPos, const NibbleArray& array) {
        if (array.isEmpty()) {
            // 空 NibbleArray 对应未初始化状态
            m_arrays[sectionPos] = SWMRNibbleArray::createUninitialized();
        } else {
            m_arrays[sectionPos] = SWMRNibbleArray::fromData(array.data());
        }
    }

    /**
     * @brief 转换为 NibbleArray（用于保存）
     */
    [[nodiscard]] NibbleArray toArray(i64 sectionPos) const {
        auto it = m_arrays.find(sectionPos);
        if (it == m_arrays.end() || it->second.isNullUpdating()) {
            return NibbleArray();
        }

        std::vector<u8> data = it->second.toByteArray();
        if (data.empty()) {
            return NibbleArray();
        }
        return NibbleArray(std::move(data));
    }

    /**
     * @brief 禁用缓存
     */
    void disableCaching() {
        // 兼容接口
    }

    /**
     * @brief 使缓存失效
     */
    void invalidateCaches() {
        // 兼容接口
    }

    // ========================================================================
    // 迭代器
    // ========================================================================

    auto begin() { return m_arrays.begin(); }
    auto end() { return m_arrays.end(); }
    auto begin() const { return m_arrays.begin(); }
    auto end() const { return m_arrays.end(); }

    /**
     * @brief 获取区块段数量
     */
    [[nodiscard]] size_t size() const { return m_arrays.size(); }

    /**
     * @brief 检查是否为空
     */
    [[nodiscard]] bool empty() const { return m_arrays.empty(); }

protected:
    std::unordered_map<i64, SWMRNibbleArray> m_arrays;

    /**
     * @brief 世界位置转区块段位置
     */
    [[nodiscard]] static i64 worldToSection(i64 worldPos) {
        return LightEngineUtils::worldToSectionPos(worldPos);
    }

    /**
     * @brief 区块段坐标转长整型
     */
    [[nodiscard]] static i64 sectionPosToLong(i32 x, i32 y, i32 z) {
        i64 lx = static_cast<i64>(x) & 0x3FFFFFLL;
        i64 lz = static_cast<i64>(z) & 0x3FFFFFLL;
        i64 ly = static_cast<i64>(y) & 0xFFFFFLL;
        return (lx << 42) | (lz << 20) | ly;
    }

    /**
     * @brief 从世界位置提取区块段内坐标
     */
    [[nodiscard]] static void extractNibbleIndices(i64 worldPos, i32& x, i32& localY, i32& z) {
        LightEngineUtils::extractNibbleIndices(worldPos, x, localY, z);
    }
};

} // namespace mc
