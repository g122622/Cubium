#pragma once

#include "common/core/Types.hpp"

namespace mc::test {

/**
 * @brief 测试实例方块实体（游戏内可视化载体）—— TODO 占位。
 *
 * 对齐基岩版 `TestInstanceBlockEntity`：在结构原点放置一个方块实体，承载测试名/状态/错误位置的游戏内
 * 可视化（信标光束颜色、讲台文本、错误标记）。
 *
 * TODO: 项目 `StructureBlockEntity` 类尚未实现（结构方块仅有 Block 子类，无 BlockEntity 子类），
 * 本类整体为占位。方块实体体系就绪后实现：
 * - 放置/移除方块实体到结构原点。
 * - 按 `GameTestState` 切换光束颜色（灰/绿/红/橙）。
 * - 失败时在错误相对坐标处放置标记方块。
 *
 * 第一阶段游戏内可视化由 `WorldVisualizationListener` 的 spdlog 输出临时承载。
 */
class TestInstanceBlockEntity {
public:
    // TODO: 实现放置/移除/状态切换 API。当前为占位，无成员。
    TestInstanceBlockEntity() = default;
};

} // namespace mc::test
