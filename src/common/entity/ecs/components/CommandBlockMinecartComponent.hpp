#pragma once

#include "common/core/Types.hpp"
#include <string>

namespace mc::ecs {

/**
 * @brief 命令方块矿车组件
 *
 * 承载 CommandBlockMinecartEntity 的命令文本、上次输出、成功次数与激活边沿状态。
 * 仅 CommandBlockMinecartEntity attach。
 *
 * 字段语义：
 * - m_command：命令文本（不含前缀 /，executeCommand 时由 IWorld 解析）。
 * - m_lastOutput：上次执行结果描述（成功/失败文本）。
 * - m_successCount：上次执行成功次数，getComparatorOutput 据此输出红石信号（上限 15）。
 * - mPowered：当前是否被激活铁轨充能。onActivatorRailPass 用它做上升沿检测
 *   （从不激活变激活才执行命令，对齐 vanilla CommandBlockMinecart 逻辑）。
 *
 * 纯 string+i32+bool，std::string 可移动，整体隐式可移动满足 entt 要求。
 * 注意：mPowered 保留驼峰命名（沿用原 OOP 成员名，对齐 vanilla MinecartCommandBlock
 * 的 powered 字段语义），与项目其余 m_ 前缀风格略异但为历史一致性保留。
 */
struct CommandBlockMinecartComponent {
    std::string m_command;
    std::string m_lastOutput;
    i32 m_successCount{0};
    bool mPowered{false}; ///< 当前是否被激活（上升沿触发命令执行）
};

} // namespace mc::ecs
