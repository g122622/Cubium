#pragma once

#include "common/entity/ecs/context/EntityId.hpp"

#include <entt/entt.hpp>

namespace mc::ecs {

/**
 * @brief 绑定 EntityId 的一元 storage 类型转换器
 *
 * entt::storage_for 默认 Entity=entt::entity，本项目 registry 用 basic_registry<EntityId>，
 * 故需把组件类型转成 entity_type=EntityId 的 storage。本结构体作为 type_list_transform 的
 * 一元 Op，把 get_t<Comp...> 中的每个裸组件类型映射为 storage_for_t<Comp, EntityId>。
 */
template <typename Comp>
struct EntityStorageFor {
    using type = entt::storage_for_t<Comp, EntityId>;
};

/**
 * @brief 项目专用 view 别名（实体类型 = EntityId）
 *
 * entt::view alias 默认绑 entt::entity，与本项目 basic_registry<EntityId> 不匹配——
 * organizer 经 as_view 转换时 entity_type 不一致导致 static_cast 失败。本别名模仿 entt::view，
 * 但把 get_t/exclude_t 中的裸组件经 EntityStorageFor 转成 entity=EntityId 的 storage，
 * 使 view 的 entity_type 与 registry 一致，organizer 依赖推导方可用自定义 entity 类型。
 *
 * 用法（free function system 签名）：
 *   void mySystem(Registry& reg, EntityView<entt::get_t<A, B>> view);
 *   void mySystem(Registry& reg, EntityView<entt::get_t<A>, entt::exclude_t<C>> view);
 */
template <typename Get, typename Exclude = entt::exclude_t<>>
using EntityView = entt::basic_view<entt::type_list_transform_t<Get, EntityStorageFor>, Exclude>;

} // namespace mc::ecs
