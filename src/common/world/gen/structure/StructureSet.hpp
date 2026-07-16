/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/gen/structure/placement/ConcentricRingsStructurePlacement.hpp"
#include "common/world/gen/structure/placement/RandomSpreadStructurePlacement.hpp"
#include "common/world/gen/structure/placement/StructurePlacement.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::world::gen::structure {

/**
 * @brief 结构集合条目
 *
 * 结构集合中的加权条目，用于在同一放置位置选择结构。
 */
struct StructureSelectionEntry {
    ResourceLocation structureId; ///< 结构资源位置（如 minecraft:village_plains）
    i32 weight;                   ///< 权重（用于加权随机选择）

    StructureSelectionEntry(ResourceLocation id, i32 w)
        : structureId(std::move(id))
        , weight(w)
    {}
};

/**
 * @brief 结构集合
 *
 * 将多个结构组合在一起，共享同一个放置规则。
 * 在候选区块中，按权重随机选择一个结构生成。
 */
class StructureSet {
public:
    /**
     * @brief 构造结构集合
     * @param id 集合资源位置
     * @param entries 加权条目列表
     * @param placement 放置规则
     */
    StructureSet(ResourceLocation id,
        std::vector<StructureSelectionEntry> entries,
        std::unique_ptr<placement::StructurePlacement> placement);

    /** 获取集合 ID */
    [[nodiscard]] const ResourceLocation& id() const { return m_id; }

    /** 获取所有条目 */
    [[nodiscard]] const std::vector<StructureSelectionEntry>& entries() const { return m_entries; }

    /** 获取放置规则 */
    [[nodiscard]] const placement::StructurePlacement& placement() const { return *m_placement; }

    /** 获取放置规则（可变） */
    [[nodiscard]] placement::StructurePlacement& placement() { return *m_placement; }

    /**
     * @brief 按权重随机选择一个条目
     * @param rng 随机数生成器
     * @return 选中的条目，如果集合为空则返回 nullptr
     */
    [[nodiscard]] const StructureSelectionEntry* selectEntry(math::Random& rng) const;

    /** 计算总权重 */
    [[nodiscard]] i32 totalWeight() const;

private:
    ResourceLocation m_id;
    std::vector<StructureSelectionEntry> m_entries;
    std::unique_ptr<placement::StructurePlacement> m_placement;
};

/**
 * @brief 结构集合注册表
 *
 * 管理所有注册的结构集合，支持按 ID 查询。
 */
class StructureSetRegistry {
public:
    /** 获取单例实例 */
    static StructureSetRegistry& instance();

    /** 初始化所有原版结构集合 */
    void initialize();

    /** 注册结构集合 */
    void registerSet(std::unique_ptr<StructureSet> set);

    /** 按 ID 查询结构集合 */
    [[nodiscard]] const StructureSet* get(const ResourceLocation& id) const;

    /**
     * @brief 按结构 ID 查询所属结构集合
     *
     * 遍历所有结构集合，找到包含指定结构 ID 的第一个集合。
     * 用于 /locate 命令和探险地图等功能，根据结构 ID 查找对应的放置规则。
     *
     * @param structureId 结构资源位置（如 minecraft:village_plains）
     * @return 包含该结构的结构集合指针，找不到返回 nullptr
     */
    [[nodiscard]] const StructureSet* findByStructure(const ResourceLocation& structureId) const;

    /** 获取所有结构集合 */
    [[nodiscard]] const std::vector<std::unique_ptr<StructureSet>>& getAll() const { return m_sets; }

    /** 清除所有注册 */
    void clear();

    /**
     * @brief 是否已初始化
     *
     * 数据驱动加载（MinecraftServer::initializeRegistries 调 StructureSetLoader）完成后
     * 或硬编码 initialize() 兜底后置位。区块生成器据此判断是否需要回退硬编码注册。
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 标记为已初始化（数据驱动加载完成后调用）
     *
     * StructureSetLoader 通过 registerSet 注册结构集合但不会置 m_initialized，
     * 加载完成后调用本方法置位，使区块生成器的兜底守卫不再触发硬编码注册。
     */
    void markInitialized() { m_initialized = true; }

private:
    StructureSetRegistry() = default;
    std::vector<std::unique_ptr<StructureSet>> m_sets;
    std::unordered_map<ResourceLocation, StructureSet*> m_byId;
    std::unordered_map<ResourceLocation, StructureSet*> m_byStructureId; ///< 结构 ID → 所属结构集合的反向索引
    bool m_initialized = false;
};

} // namespace mc::world::gen::structure
