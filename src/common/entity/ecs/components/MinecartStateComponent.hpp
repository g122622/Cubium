#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"

namespace mc::ecs {

/**
 * @brief 矿车运行状态组件
 *
 * 承载 AbstractMinecartEntity 的 12 字段运行/配置状态。仅 AbstractMinecartEntity 子树
 * attach（Rideable/Chest/Furnace/Tnt/Hopper/CommandBlock/Spawner 共用）。
 *
 * 字段分类：
 * - 铁轨状态（m_onRail/m_railPos/m_railShape/m_flipped）：B 类运行时，_checkRailState/
 *   _moveAlongTrack 强耦合，逻辑留 OOP，数据搬组件。
 * - 速度配置（m_maxSpeed/m_maxSpeedAirLateral/m_maxSpeedAirVertical/m_dragAir）：
 *   vanilla AbstractMinecart 为 protected 可被子类 override 覆盖的速度参数，项目用成员
 *   + virtual getter 模式（getMaxSpeed 等返回成员）。组件化为真相源。
 * - 损坏（m_damage）：业务真相源。注意项目当前 DATA_DAMAGE_PARAM 注册了但业务从不 set
 *   （成员与 DataParameter 脱节），本批保持该现状——m_damage 作 B 类字段进组件无同步镜像，
 *   不引入行为变化。同步脱节是既有设计，非本批范围。
 * - 摇晃动画（m_rollingAmplitude/m_rollingDirection）：运行时，_updateRollingAnimation 用。
 *   vanilla 旧 rolling 字段为项目自创 ghost 已删（见 registerData 注释），此处仅项目内部动画。
 * - 可推动（m_canBePushed）：配置标志。
 *
 * m_type（Type 枚举）不进组件——构造期定值不变，是实体类型标识（类似 EntityType），
 * getMinecartType 只读返回，ECS 化无意义。
 */
struct MinecartStateComponent {
    // 铁轨状态
    bool m_onRail{false};
    BlockPos m_railPos{};
    blocks::RailShape m_railShape{blocks::RailShape::NorthSouth};
    bool m_flipped{false}; ///< 是否翻转（铁轨方向变化超 90° 时翻转，渲染器据此调整朝向）

    // 速度配置
    f32 m_maxSpeed{0.4f};             ///< 最大铁轨速度
    f32 m_maxSpeedAirLateral{0.4f};   ///< 空中最大横向速度
    f32 m_maxSpeedAirVertical{-1.0f}; ///< 空中最大纵向速度（-1 表示禁用）
    f32 m_dragAir{0.95f};             ///< 空气阻力

    // 损坏和动画
    i32 m_damage{0};           ///< 损坏值（撞击累积，超阈值摧毁）
    i32 m_rollingAmplitude{0}; ///< 摇晃幅度
    i32 m_rollingDirection{1}; ///< 摇晃方向

    // 可推动状态
    bool m_canBePushed{true};
};

} // namespace mc::ecs
