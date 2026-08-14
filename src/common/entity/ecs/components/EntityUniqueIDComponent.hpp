#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 实体持久唯一 ID 组件
 *
 * 64 位全局唯一 ID，作为 entt::entity 运行时句柄的「持久化投影」。
 *
 * 用途：entt::entity 是 registry 局部的、销毁后版本号会变（虽扩大 version 位
 * 降回绕风险，但仍非跨 registry 持久）；网络包 entity id、存档 NBT 实体引用、
 * 跨维度传送等场景需要持久唯一身份，由本组件承载。
 *
 * 设计参考基岩版 ActorUniqueIDComponent（mc/entity/components/ActorUniqueIDComponent.h）。
 * 首批过渡期，m_uniqueId 直接取自现有 EntityInstanceId（u64 单调递增、永不复用），
 * 保持与现有网络/存档层兼容。
 */
struct EntityUniqueIDComponent {
    u64 m_uniqueId{0};
};

} // namespace mc::ecs
