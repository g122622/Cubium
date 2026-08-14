#pragma once

#include "common/core/Types.hpp"

#include <entt/entt.hpp>

namespace mc::ecs {

class EntityId; // 前向声明，供 EntityIdTraits::value_type 引用

/**
 * @brief entt entity id 的位分配 traits
 *
 * 决定 EntityId 的 entity 部分与 version 部分各占多少位。
 * 采用 64 位 entity_type 方案：entity 32 位 + version 32 位。
 * 相比 entt 默认 32 位方案（entity 20 位 + version 12 位）和基岩版方案
 * （entity 18 位 + version 14 位），version 位扩大到 32 位，回绕风险极低，
 * 配合 EntityUniqueIDComponent 持久投影进一步消除跨 registry 误用。
 *
 * entity 32 位可容纳约 42 亿实体，单 registry 内足够；version 32 位意味着
 * 同一 entity 槽位需被回收约 42 亿次才会回绕到旧版本号，实际不可能触发。
 */
struct EntityIdTraits {
    using value_type = EntityId;
    using entity_type = u64;
    using version_type = u32;

    static constexpr entity_type entity_mask = 0xFFFFFFFF;
    static constexpr entity_type version_mask = 0xFFFFFFFF;
};

} // namespace mc::ecs

/**
 * @brief entt traits 特化（须在 basic_registry<EntityId> 实例化前可见）
 *
 * page_size 复用 entt 默认稀疏集分页大小。
 */
template <>
class entt::entt_traits<mc::ecs::EntityId> : public entt::basic_entt_traits<mc::ecs::EntityIdTraits> {
public:
    static constexpr entity_type page_size = ENTT_SPARSE_PAGE;
};

namespace mc::ecs {

/**
 * @brief 实体在 ECS registry 中的运行时句柄
 *
 * 包装一个 64 位 raw id（entity 部分低 32 位 + version 部分高 32 位）。
 * 这是 ECS 内部组件访问的唯一句柄；跨 registry / 网络 / 存档的持久身份
 * 另由 EntityUniqueIDComponent 提供，不要用 EntityId 做持久引用。
 *
 * 语义上等价 entt::entity，但通过特化 entt_traits 接入了自定义位分配。
 * 可隐式转 entity_type 以便 entt 内部使用。
 */
class EntityId : public entt::entt_traits<EntityId> {
public:
    entity_type mRawId{entt::null};

    [[nodiscard]] constexpr EntityId() = default;

    [[nodiscard]] constexpr EntityId(entity_type rawId) noexcept : mRawId(rawId) {}

    [[nodiscard]] constexpr bool isNull() const noexcept { return *this == entt::null; }

    [[nodiscard]] constexpr operator entity_type() const noexcept { return mRawId; }
};

} // namespace mc::ecs
