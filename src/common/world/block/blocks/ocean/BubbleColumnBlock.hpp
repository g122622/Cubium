#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;

namespace blocks {

/**
 * @brief 气泡柱方块
 *
 * 由岩浆块或灵魂沙产生的水下气泡柱。
 * 可以推动实体向上或向下。
 *
 * ## 状态属性
 * - DRAG: 是否为下拖
 *   - false: 向上推动（灵魂沙产生）
 *   - true: 向下拖拽（岩浆块产生）
 *
 * ## 推动机制 (MC 1.16.5)
 * - 灵魂沙: 产生上升气泡柱 (DRAG=false)，推动速度 +0.1
 * - 岩浆块: 产生下降气泡柱 (DRAG=true)，拖拽速度 -0.03
 *
 * ## 延伸机制
 * - 气泡柱会向上延伸直到水面或空气
 * - tick 方法处理向上传播
 *
 * 参考: net.minecraft.block.BubbleColumnBlock
 */
class BubbleColumnBlock : public Block {
public:
    explicit BubbleColumnBlock(const BlockProperties& properties);
    ~BubbleColumnBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 检查是否为下拖模式
     * @param state 方块状态
     * @return true 下拖（岩浆块产生），false 上推（灵魂沙产生）
     */
    [[nodiscard]] bool isDrag(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 实体交互 ==========

    /**
     * @brief 实体碰撞时推动实体
     *
     * MC 1.16.5 逻辑：
     * - DRAG=false: 向上推动 +0.1 Y 速度
     * - DRAG=true: 向下拖拽 -0.03 Y 速度
     * - 重置摔落距离
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param entity 碰撞的实体
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== Tick ==========

    /**
     * @brief 方块 tick，处理气泡柱向上延伸
     *
     * MC 1.16.5 逻辑：
     * - 检查上方是否为水源方块
     * - 如果是水，将其转换为气泡柱并继承 DRAG 状态
     * - 如果上方已是气泡柱，更新其 DRAG 状态
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param random 随机数生成器
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    /**
     * @brief 检查下方是否产生气泡
     *
     * 检查下方方块类型来确定 DRAG 状态：
     * - 岩浆块: return true (下拖)
     * - 灵魂沙: return false (上推)
     * - 气泡柱: 继承其 DRAG 状态
     *
     * @param world 世界引用
     * @param pos 当前位置
     * @return bool DRAG 状态
     */
    [[nodiscard]] bool checkSource(const IWorld& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
