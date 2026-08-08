#pragma once

#include "common/entity/attribute/AttributeMap.hpp"
#include <memory>

namespace mc::ecs {

/**
 * @brief 属性状态组件
 *
 * 承载 LivingEntity::m_attributes（AttributeMap）。
 * 对齐基岩版 AttributesComponent（mc/entity/components/AttributesComponent.h）。
 *
 * 仅 LivingEntity（含其子类 MobEntity/Player）attach，普通 Entity 不持有此组件。
 *
 * 设计要点：AttributeMap 含 mutable std::mutex 不可移动/拷贝，而 entt 组件池要求
 * 可移动（swap-and-pop 重排）。故用 std::unique_ptr<AttributeMap> 包裹：组件移动只
 * 搬指针，AttributeMap 本体不移动。这与直接内嵌 AttributeMap（pinned type，依赖
 * "永不 remove/sort/compact" 运行时契约）相比，不引入首个 pinned 组件，容错性更高。
 *
 * 访问模式：LivingEntity::attributes() getter 返回 *m_attributes 解引用，全仓子类
 * 原 protected m_attributes.xxx 访问改为 attributes().xxx。组件 attach 后由
 * registerAttributes() 填充默认属性，子类 registerAttributes() 重写追加实体专属属性。
 */
struct AttributeComponent {
    std::unique_ptr<entity::attribute::AttributeMap> m_attributes;

    AttributeComponent()
        : m_attributes(std::make_unique<entity::attribute::AttributeMap>())
    {}
};

} // namespace mc::ecs
