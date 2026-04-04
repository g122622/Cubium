#pragma once

#include "../../Block.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 抽象按钮方块基类
 *
 * 按钮可以输出短脉冲红石信号。
 *
 * ## 特性
 * - 按压输出信号
 * - 自动复位（不同材质延迟不同）
 * - 可附着在不同面上
 * - 短脉冲输出
 *
 * ## 容易踩的坑
 * - 脉冲持续时间需要精确控制
 * - 附着面变化时需要检测支撑
 * - 木按钮和石按钮延迟不同
 *
 * 参考: net.minecraft.block.AbstractButtonBlock
 */
class AbstractButtonBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param ticksToStayPressed 按压持续时间（tick）
     */
    AbstractButtonBlock(const BlockProperties& properties, i32 ticksToStayPressed);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                        const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state, Direction facing,
        const BlockState& facingState, IWorld& world,
        const BlockPos& currentPos, const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side
    ) const override;

    [[nodiscard]] i32 getStrongPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side
    ) const override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 按钮特有方法 ==========

    /**
     * @brief 检查按钮是否被按下
     *
     * @param state 方块状态
     * @return true 如果被按下
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 设置按钮的按下状态
     *
     * @param state 方块状态
     * @param powered 是否按下
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPowered(BlockState state, bool powered);

    /**
     * @brief 获取按钮附着的方向
     *
     * @param state 方块状态
     * @return Direction 附着方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 按下按钮
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void press(IWorld& world, const BlockPos& pos, const BlockState& state);

protected:
    /// 按压持续时间（tick）
    i32 m_ticksToStayPressed;

    /**
     * @brief 播放点击音效
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param pressed true为按下，false为弹起
     */
    virtual void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const = 0;

    /**
     * @brief 检查是否可以附着在指定面
     *
     * @param facing 附着方向
     * @return true 如果可以附着
     */
    [[nodiscard]] bool canAttachToFace(Direction facing) const;

    /**
     * @brief 通知相邻方块更新
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param facing 附着方向
     */
    void notifyNeighbors(IWorld& world, const BlockPos& pos, Direction facing);
};

} // namespace blocks
} // namespace mc
