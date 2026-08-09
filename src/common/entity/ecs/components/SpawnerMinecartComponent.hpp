#pragma once

#include "common/world/blockentity/spawner/SpawnerLogic.hpp"

namespace mc::ecs {

/**
 * @brief 刷怪笼矿车组件
 *
 * 承载 SpawnerMinecartEntity 的刷怪逻辑。仅 SpawnerMinecartEntity attach。
 *
 * SpawnerLogic 含 std::function（ParticleEventCallback）/vector/optional/ResourceLocation，
 * 全成员 noexcept 可移动，整体隐式 noexcept 可移动，满足 entt 组件池要求。
 * 直接存值（沿用原 OOP m_spawnerLogic 值成员语义，栈内嵌无间接寻址，构造默认构造）。
 *
 * SpawnerLogic 的 ParticleEventCallback 在 serverTick 时由 SpawnerMinecartEntity::tick
 * 临时构造 lambda 传入（不存储于 SpawnerLogic），故 entt pool 移动组件不影响回调。
 *
 * 持久化：SpawnerMinecart 的 addAdditionalSaveData/readAdditionalSaveData 当前是 OOP override
 * 直接调 m_spawnerLogic.saveToNBT/loadFromNBT。B7.2-Step3 将搬入组件序列化器注册表
 * （MinecartComponentSerialization），届时删除该 OOP override。
 */
struct SpawnerMinecartComponent {
    blockentity::SpawnerLogic m_spawnerLogic;
};

} // namespace mc::ecs
