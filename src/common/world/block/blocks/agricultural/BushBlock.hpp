#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 灌木/植物方块基类
 *
 * 所有植物类方块的基类，提供基本的放置逻辑和形状。
 * 植物只能在特定地面（草地、泥土、耕地等）上放置。
 *
 * 参考: net.minecraft.block.BushBlock
 */
class BushBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BushBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~BushBlock() override = default;

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查位置是否有效
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
     */
    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（植物无碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取遮挡形状（植物不遮挡光线）
     */
    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    /**
     * @brief 是否不透明（植物透明）
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

protected:
    /**
     * @brief 检查下方方块是否可以支撑此植物
     * @param groundState 下方方块状态
     * @param world 世界
     * @param groundPos 下方位置
     * @return 如果可以支撑返回true
     */
    [[nodiscard]] virtual bool canSustain(
        const BlockState& groundState,
        IWorld& world,
        const BlockPos& groundPos) const;

    /// 植物形状（默认为完整方块大小）
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
