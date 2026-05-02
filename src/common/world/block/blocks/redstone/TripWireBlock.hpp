#pragma once

#include "world/block/Block.hpp"
#include <vector>

namespace mc {
namespace blocks {

/**
 * @brief 绊线方块
 *
 * 绊线是一种由实体触发红石信号的方块，需要配合绊线钩使用。
 *
 * ## 特性
 * - 检测实体碰撞
 * - 与绊线钩配合使用
 * - 最大长度42格
 * - 被剪断时掉落线
 * - 需要支撑方块
 *
 * 参考: net.minecraft.block.TripWireBlock
 */
class TripWireBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TripWireBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                        const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state, Direction facing,
        const BlockState& facingState, IWorld& world,
        const BlockPos& currentPos, const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(const BlockState& state, IWorld& world,
                                   const BlockPos& pos, Direction side) const override;

    [[nodiscard]] i32 getStrongPower(const BlockState& state, IWorld& world,
                                     const BlockPos& pos, Direction side) const override;

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override {
        MC_UNUSED(state);
        return Material::PushReaction::Destroy;
    }

    // ========== 绊线特有方法 ==========

    /**
     * @brief 检查绊线是否被触发
     * @param state 方块状态
     * @return true 如果被触发
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 检查绊线是否连接
     * @param state 方块状态
     * @param direction 方向
     * @return true 如果连接
     */
    [[nodiscard]] static bool isConnected(const BlockState& state, Direction direction);

    /**
     * @brief 检查绊线是否被触发（实体检测）
     * @param state 方块状态
     * @return true 如果被触发
     */
    [[nodiscard]] static bool isActivated(const BlockState& state);

    /**
     * @brief 更新绊线状态
     *
     * 检查实体碰撞并更新状态
     *
     * @param world 世界引用
     * @param pos 绊线位置
     */
    void updateState(IWorld& world, const BlockPos& pos);

private:
    /**
     * @brief 检测实体碰撞
     * @param world 世界引用
     * @param pos 绊线位置
     * @return true 如果有实体碰撞
     */
    [[nodiscard]] bool checkEntityCollision(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 通知绊线钩更新
     * @param world 世界引用
     * @param pos 绊线位置
     */
    void notifyHooks(IWorld& world, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
