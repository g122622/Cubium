#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/EntityPose.hpp"
#include <memory>

namespace mc::text {
class ITextComponent;
}

namespace mc::ecs {

/**
 * @brief 实体轻量同步元数据组件
 *
 * 聚合 Entity 层 6 个低频同步字段（air/customName/customNameVisible/silent/noGravity/pose）。
 * 对齐基岩版 ActorDataSynched 组件族（轻量元数据聚合，mc/entity/components/）。
 *
 * 所有 Entity attach。各字段均为 C 类同步字段（含对应 DataParameter），本组件为真相源，
 * DataParameter 退为同步镜像：setXxx 同时写组件 + DataParameter，syncMetadataFromDataManager
 * 不再从镜像回填。延续第二批 freeze / 第三批 health/arrows 的同步镜像模式。
 *
 * 字段语义：
 * - m_air：空气值（默认 300 = maxAir）。水生实体入水消耗/出水恢复，DATA_AIR_PARAM(id1) 镜像。
 * - m_customName：自定义名称组件（unique_ptr<ITextComponent>）。承接原 m_customName 类型，
 *   保留样式/颜色等富文本信息；DataParameter 仅存 OptionalComponentValue（present + 纯文本）
 *   作有损同步投影。DATA_CUSTOM_NAME_PARAM(id2) 镜像。
 * - m_customNameVisible：自定义名称是否始终显示。DATA_CUSTOM_NAME_VISIBLE_PARAM(id3) 镜像。
 * - m_silent：是否静音（不播放声音）。DATA_SILENT_PARAM(id4) 镜像。
 * - m_noGravity：是否无重力。DATA_NO_GRAVITY_PARAM(id5) 镜像。
 * - m_pose：实体姿态。setPose 迁移后保留 refreshDimensions() 副作用（潜行变矮/游泳变扁的
 *   碰撞箱更新）。DATA_POSE_PARAM(id6) 镜像。
 *
 * 设计要点：m_customName 用 unique_ptr<ITextComponent>（不可拷贝可移动），与
 * AttributeComponent 用 unique_ptr<AttributeMap> 包裹范式一致，entt emplace 兼容。
 */
struct EntityStateComponent {
    i32 m_air{300};
    std::unique_ptr<text::ITextComponent> m_customName;
    bool m_customNameVisible{false};
    bool m_silent{false};
    bool m_noGravity{false};
    entity::EntityPose m_pose{entity::EntityPose::Standing};
};

} // namespace mc::ecs
