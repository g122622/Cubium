#pragma once

#include "../../Block.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 抽象压力板方块基类
 *
 * 压力板可以检测实体并输出红石信号。
 *
 * ## 特性
 * - 实体检测
 * - 根据实体数量或类型输出信号
 * - 不同材质压力板有不同的检测规则
 *
 * ## 容易踩的坑
 * - 信号强度计算需要正确的实体计数
 * - 需要处理玩家/生物/物品的不同检测
 * - 支撑方块移除时压力板掉落
 *
 * 参考: net.minecraft.block.AbstractPressurePlateBlock
 */
class AbstractPressurePlateBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    AbstractPressurePlateBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                        const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

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

    // ========== 压力板特有方法 ==========

    /**
     * @brief 获取当前信号强度
     *
     * @param state 方块状态
     * @return i32 信号强度（0-15）
     */
    [[nodiscard]] static i32 getPower(const BlockState& state);

    /**
     * @brief 设置信号强度
     *
     * @param state 方块状态
     * @param power 信号强度
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPower(BlockState state, i32 power);

protected:
    /**
     * @brief 计算当前信号强度
     *
     * 由子类实现具体的检测逻辑。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 计算出的信号强度
     */
    [[nodiscard]] virtual i32 calculateSignalStrength(IWorld& world, const BlockPos& pos) const = 0;

    /**
     * @brief 获取信号转换为tick的延迟
     *
     * @param oldSignal 旧信号
     * @param newSignal 新信号
     * @return i32 tick延迟
     */
    [[nodiscard]] virtual i32 getTickDelay(i32 oldSignal, i32 newSignal) const = 0;

    /**
     * @brief 播放点击音效
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param pressed true为按下，false为弹起
     */
    virtual void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const = 0;

    /**
     * @brief 检查实体是否可以触发压力板
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return true 如果有有效实体
     */
    [[nodiscard]] bool hasEntityOnPlate(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 更新压力板状态
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void updateState(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
