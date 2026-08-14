#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 矿车显示方块组件
 *
 * 承载 AbstractMinecartEntity 的 3 字段显示方块状态（m_displayTile/m_displayTileOffset/
 * m_showBlock）。仅 AbstractMinecartEntity 子树 attach。
 *
 * 状态：项目当前这 3 字段是 TODO 待接入 wire 的本地字段——vanilla 1.21.11 走
 * DATA_CUSTOM_DISPLAY_BLOCK_PARAM（OptionalBlockStateValue, wire id 11）+ DATA_DISPLAY_OFFSET_PARAM
 * （i32, wire id 12）同步显示方块到客户端，但项目业务尚未接通（成员声明保留供未来接入，
 * 见 MinecartEntity.hpp registerData 注释）。本批仅把成员搬入组件作真相源，不改变
 * "尚未接通 wire"现状，待后续矿车内显示方块业务（熔炉/刷怪笼/命令方块矿车渲染）接入时
 * 经 DataParameter 镜像同步。
 *
 * 字段语义：
 * - m_displayTile：方块状态ID（待接入 wire，对应 vanilla OptionalBlockState 的状态值）。
 * - m_displayTileOffset：显示偏移（待接入 wire，默认 6 对齐 vanilla DATA_DISPLAY_OFFSET）。
 * - m_showBlock：是否显示方块（待接入 wire，对应 OptionalBlockState 的 present 标志）。
 */
struct MinecartDisplayComponent {
    i32 m_displayTile{0};       ///< 方块状态ID（待接入 wire）
    i32 m_displayTileOffset{6}; ///< 显示偏移（待接入 wire，对齐 vanilla 默认 6）
    bool m_showBlock{false};    ///< 是否显示方块（待接入 wire，对应 Optional present）
};

} // namespace mc::ecs
