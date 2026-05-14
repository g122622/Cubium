#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;

namespace blocks {

/**
 * @brief 耕地方块
 *
 * 用于种植农作物的土地。具有湿润等级属性（0-7）。
 * 靠近水源会提高湿润等级。
 *
 * 状态属性：
 * - MOISTURE: 湿润等级 (0-7)
 *
 * 参考: net.minecraft.block.FarmlandBlock
 */
class FarmlandBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FarmlandBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~FarmlandBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 移动和交互 ==========

    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    void onFallenUpon(
        IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance) override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 工具方法 ==========

    /**
     * @brief 转变为泥土
     */
    static void turnToDirt(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 检查是否湿润
     */
    [[nodiscard]] static bool isMoist(const BlockState& state)
    {
        return state.get(BlockStateProperties::MOISTURE_0_7()) > 0;
    }

    /**
     * @brief 检查位置是否有水
     */
    [[nodiscard]] static bool hasWater(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查上方是否有作物
     */
    [[nodiscard]] static bool hasCrops(IWorld& world, const BlockPos& pos);

private:
    /// 耕地形状（高度15像素）
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
