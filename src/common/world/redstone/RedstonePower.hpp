#pragma once

#include "../../core/Types.hpp"
#include "../../util/Direction.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;
class Block;

namespace world {
namespace redstone {

/**
 * @brief 红石信号强度计算工具
 *
 * 提供静态方法计算各种红石信号强度。
 * 区分强信号（Strong Power）和弱信号（Weak Power）。
 *
 * ## 强信号 vs 弱信号
 * - **强信号**：直接从方块侧面输出的信号（如红石火把、中继器输出端）
 *   可以充能相邻的实体方块，使之输出弱信号
 * - **弱信号**：通过方块传导的信号（如被充能的方块、红石线）
 *   只能检测是否存在，不能充能其他方块
 *
 * ## 信号强度范围
 * - 0：无信号
 * - 1-14：中间强度，红石线每传输一格衰减1
 * - 15：最大强度
 *
 * 参考 MC 1.16.5 RedstoneRedstonePowerLogic
 */
class RedstonePower {
public:
    /// 红石信号最大强度
    static constexpr i32 MAX_POWER = 15;

    /// 红石信号最小强度
    static constexpr i32 MIN_POWER = 0;

    // ========== 强信号 ==========

    /**
     * @brief 获取方块的强信号输出
     *
     * 强信号直接从方块侧面输出，可以被红石线检测。
     * 例如：红石火把、中继器输出端、比较器输出端。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param side 输出方向（信号从这个方向传出）
     * @return i32 强信号强度 0-15
     */
    [[nodiscard]] static i32 getStrongPower(IWorld& world,
                                             const BlockPos& pos,
                                             Direction side);

    /**
     * @brief 获取方块所有方向的最大强信号
     *
     * 遍历六个方向，返回最强的强信号。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 最大强信号强度 0-15
     */
    [[nodiscard]] static i32 getStrongPower(IWorld& world, const BlockPos& pos);

    // ========== 弱信号 ==========

    /**
     * @brief 获取方块的弱信号输出
     *
     * 弱信号通过方块传导，强度不叠加。
     * 例如：被充能的方块、红石线。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param side 输出方向
     * @return i32 弱信号强度 0-15
     */
    [[nodiscard]] static i32 getWeakPower(IWorld& world,
                                           const BlockPos& pos,
                                           Direction side);

    /**
     * @brief 获取方块所有方向的最大弱信号
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 最大弱信号强度 0-15
     */
    [[nodiscard]] static i32 getWeakPower(IWorld& world, const BlockPos& pos);

    // ========== 充能检测 ==========

    /**
     * @brief 检查方块是否被红石信号充能
     *
     * 当任意方向的强信号或弱信号 > 0 时返回 true。
     * 注意：被充能的方块本身不输出信号，需要通过 getWeakPower 获取。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return true 如果被充能
     */
    [[nodiscard]] static bool isPowered(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方块是否被间接充能
     *
     * 检查相邻方块是否有强信号输出。
     * 用于判断实体方块是否被充能。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return true 如果被间接充能
     */
    [[nodiscard]] static bool isIndirectlyPowered(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方块侧面是否被充能
     *
     * 检查指定方向的相邻方块是否输出强信号。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param side 检查方向
     * @return true 如果该方向被充能
     */
    [[nodiscard]] static bool isSidePowered(IWorld& world,
                                            const BlockPos& pos,
                                            Direction side);

    // ========== 特殊信号计算 ==========

    /**
     * @brief 获取红石线的输入信号强度
     *
     * 计算红石线从相邻方块接收的信号强度，
     * 包括：
     * - 相邻信号源的强信号
     * - 相邻红石线的信号（衰减1）
     * - 向上/向下连接的红石线信号
     *
     * @param world 世界引用
     * @param pos 红石线位置
     * @return i32 输入信号强度 0-15
     */
    [[nodiscard]] static i32 getWireInputPower(IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取比较器输入信号
     *
     * 检测容器信号输出或红石线信号。
     * 容器信号基于填充程度计算（每满一定比例增加1强度）。
     *
     * @param world 世界引用
     * @param pos 比较器位置
     * @param facing 比较器朝向（输入端方向）
     * @return i32 输入信号强度 0-15
     */
    [[nodiscard]] static i32 getComparatorInput(IWorld& world,
                                                 const BlockPos& pos,
                                                 Direction facing);

    /**
     * @brief 获取相邻方块的最大红石信号
     *
     * 遍历六个方向，获取每个方向方块输出的红石信号最大值。
     * 不包括来自红石线的信号（使用 getWireInputPower 获取）。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 最大信号强度 0-15
     */
    [[nodiscard]] static i32 getRedstonePowerFromNeighbors(IWorld& world,
                                                            const BlockPos& pos);

private:
    /**
     * @brief 检查方块是否可以连接红石
     *
     * @param state 方块状态
     * @return true 如果可以连接
     */
    [[nodiscard]] static bool canConnectRedstone(const BlockState& state);
};

} // namespace redstone
} // namespace world
} // namespace mc
