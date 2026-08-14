#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 可装箱马类中间层组件（A 类本地字段）
 *
 * 承载 AbstractChestedHorseEntity 的箱子装备状态。仅 AbstractChestedHorseEntity 子树
 * attach（Donkey/Mule/Llama/TraderLlama 经中间层构造自动获得，Horse/SkeletonHorse/
 * ZombieHorse 不继承此中间层不 attach）。
 *
 * 字段分类：A 类纯本地无同步——m_hasChest 无 DataParameter。
 *
 * getInventorySize() 依赖此字段决定库存规模：未装箱回落基类 2 槽（鞍+马铠），
 * 装箱后 2 + 3 * getInventoryColumns()。initHorseChest() 据此重建库存。
 *
 * 持久化：vanilla 1.21.11 AbstractChestedHorse 持久化 ChestnutItem 等容器内容走
 * LootableContainer 体系；m_hasChest 标志本身项目当前未存盘（1.16.5 残留缺陷，TODO）。
 * 本组件不注册序列化器。
 */
struct ChestedHorseComponent {
    bool m_hasChest{false}; ///< 是否装备了箱子
};

} // namespace mc::ecs
