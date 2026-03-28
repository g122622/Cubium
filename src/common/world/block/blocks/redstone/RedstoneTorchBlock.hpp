#pragma once

#include "../../Block.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../../util/property/Properties.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

/**
 * @brief 红石火把方块
 *
 * 红石信号反转器，当下方有信号输入时熄灭。
 * 熄灭时输出0，点亮时输出15强度信号。
 *
 * ## 特性
 * - 信号反转：下方有信号时熄灭
 * - 输出强度：点亮时15，熄灭时0
 * - 烧毁机制：60 tick内翻转8次则烧毁，冷却160 tick
 * - 弱信号输出：从四周和上方输出
 *
 * ## 容易踩的坑
 * - 红石火把不向下输出信号
 * - 烧毁时需要记录历史翻转
 * - 更新时需要防止无限递归
 *
 * 参考: net.minecraft.block.RedstoneTorchBlock
 */
class RedstoneTorchBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneTorchBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                        const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 红石接口 ==========

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

    // ========== 红石火把特有方法 ==========

    /**
     * @brief 检查火把是否应该熄灭
     *
     * 当下方方块被充能时，火把应该熄灭。
     *
     * @param world 世界引用
     * @param pos 火把位置
     * @return true 如果应该熄灭
     */
    [[nodiscard]] bool shouldBeOff(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检查火把是否点亮
     *
     * @param state 方块状态
     * @return true 如果点亮
     */
    [[nodiscard]] static bool isLit(const BlockState& state);

protected:
    /**
     * @brief 更新火把状态
     *
     * 检查是否需要改变点亮/熄灭状态，
     * 如果需要则调度延迟更新。
     *
     * @param world 世界引用
     * @param pos 火把位置
     * @param state 当前方块状态
     */
    void updateState(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
