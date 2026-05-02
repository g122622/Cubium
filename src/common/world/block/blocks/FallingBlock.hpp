#pragma once

#include "../Block.hpp"

namespace mc {

// 前向声明
namespace entity {
class FallingBlockEntity;
}

namespace blocks {

/**
 * @brief 可下落方块基类
 *
 * 用于沙子、红沙、砾石等会受重力影响的方块。
 * 当下方方块无法支撑时，调度计划刻并生成下落方块实体。
 *
 * 参考: net.minecraft.block.FallingBlock
 */
class FallingBlock : public Block {
public:
    explicit FallingBlock(const BlockProperties& properties);
    ~FallingBlock() override = default;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(IWorld& world, const BlockPos& pos,
                         Block& neighborBlock, const BlockPos& neighborPos,
                         bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 方块更新后处理
     *
     * 当邻居方块更新时也调度 tick。
     */
    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 虚方法（子类可覆盖） ==========

    /**
     * @brief 获取下落延迟
     *
     * 默认返回 2 tick。铁砧等子类可覆盖。
     *
     * @return 延迟 tick 数
     */
    [[nodiscard]] virtual i32 getFallDelay() const { return FALL_DELAY_TICKS; }

    /**
     * @brief 开始下落时的回调
     *
     * 子类可覆盖以执行特殊行为。
     *
     * @param world 世界
     * @param pos 位置
     * @param entity 下落方块实体
     */
    virtual void onStartFalling(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity) {
        (void)world;
        (void)pos;
        (void)entity;
    }

    /**
     * @brief 落地时的回调
     *
     * 当下落方块落到地面时调用。
     * 子类可覆盖（如混凝土粉末遇水固化）。
     *
     * @param world 世界
     * @param pos 落地位置
     * @param fallingState 下落时的方块状态
     * @param hitState 落地点的方块状态
     * @param entity 下落方块实体
     */
    virtual void onEndFalling(IWorld& world, const BlockPos& pos,
                              const BlockState& fallingState, const BlockState& hitState,
                              entity::FallingBlockEntity& entity) {
        (void)world;
        (void)pos;
        (void)fallingState;
        (void)hitState;
        (void)entity;
    }

    /**
     * @brief 破碎时的回调
     *
     * 当下落方块无法放置时调用（如落到不合适的方块上）。
     * 子类可覆盖（如铁砧损坏）。
     *
     * @param world 世界
     * @param pos 位置
     * @param entity 下落方块实体
     */
    virtual void onBroken(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity) {
        (void)world;
        (void)pos;
        (void)entity;
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查方块状态是否可穿透
     *
     * 用于判断下落方块是否可以穿过指定方块。
     * 可穿透的方块包括：空气、液体、火焰、可替换材质。
     *
     * @param state 方块状态
     * @return 如果可穿透返回 true
     */
    [[nodiscard]] static bool canFallThrough(const BlockState* state);

protected:
    static constexpr i32 FALL_DELAY_TICKS = 2;
};

} // namespace blocks
} // namespace mc
