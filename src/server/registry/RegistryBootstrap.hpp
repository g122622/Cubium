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

namespace mc::resource {
class DataPackRepository;
}

namespace mc::loot {
class LootTableManager;
class LootPredicateManager;
} // namespace mc::loot

namespace mc::function {
class FunctionManager;
}

namespace mc::server {

/**
 * @brief Vanilla 注册表装配门面
 *
 * 承接原 MinecraftServer::initializeRegistries 的全部注册表装配职责：从硬编码兜底
 * 初始化到数据包驱动的 worldgen/loader 全链路，按严格的依赖顺序装配方块、物品、
 * 附魔、配方、战利品、函数、进度、噪声、密度函数、结构、生物群系、实体类型等
 * 注册表，以及 Java wire id 映射表。
 *
 * 不持有任何注册表所有权，仅持有四个既存管理器的引用，按顺序驱动它们的加载器。
 * 调用顺序敏感：各 Loader 之间存在显式依赖（如 biome 必须在 placed_feature 之后，
 * 结构集合必须在生物群系标签之后），严禁重排。
 */
class RegistryBootstrap {
public:
    /**
     * @brief 构造注册表装配门面
     *
     * @param dataPackList 数据包仓库（所有 Loader 的数据来源）
     * @param lootTableManager 战利品表管理器（承载战利品表 + 谓词反向关联）
     * @param predicateManager 战利品谓词管理器
     * @param functionManager 函数管理器（.mcfunction 加载目标）
     */
    RegistryBootstrap(mc::resource::DataPackRepository& dataPackList,
        mc::loot::LootTableManager& lootTableManager,
        mc::loot::LootPredicateManager& predicateManager,
        mc::function::FunctionManager& functionManager);

    /**
     * @brief 装配全部 Vanilla 注册表
     *
     * 严格按依赖顺序加载：方块 → 物品 → 唱片机歌曲 → 附魔 → 方块物品 → 物品标签
     * → 数据包物品标签 → 发射器行为 → 战利品表 → 战利品谓词 → 配方 → 函数 → 进度
     * → 噪声 → 密度函数 → noise_settings → 模板池 → 结构 → 处理器列表 → jigsaw
     * 模板管理 → 放置器 → 特征类型 → configured/placed feature → carver → biome
     * → biome 标签 → flat preset → world preset → 结构标签 → 结构集合 → 实体类型
     * （可选）→ 实体类型标签 → 伤害类型标签 → 日程 → 记忆模块 → 村民交易 → Java id 映射。
     *
     * @param registerEntities 是否注册实体类型（独立服 true，集成服 false）
     */
    void initializeAll(bool registerEntities);

private:
    mc::resource::DataPackRepository& m_dataPackList;
    mc::loot::LootTableManager& m_lootTableManager;
    mc::loot::LootPredicateManager& m_predicateManager;
    mc::function::FunctionManager& m_functionManager;
};

} // namespace mc::server
