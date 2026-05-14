#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 蘑菇方块基类
 *
 * 可放置在草地、泥土、菌岩等上的小型蘑菇。
 * 在黑暗环境中可以生长成巨型蘑菇。
 *
 * 参考: net.minecraft.block.MushroomBlock
 */
class MushroomBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit MushroomBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~MushroomBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

protected:
    /// 蘑菇形状
    CollisionShape m_shape;
};

/**
 * @brief 巨型蘑菇方块
 *
 * 巨型蘑菇的组成方块，有不同的纹理面。
 * 用于棕色和红色巨型蘑菇。
 *
 * 状态属性：
 * - DOWN/UP/NORTH/SOUTH/EAST/WEST: 各面的纹理类型
 *
 * 参考: net.minecraft.block.HugeMushroomBlock
 */
class HugeMushroomBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit HugeMushroomBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~HugeMushroomBlock() override = default;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;
};

} // namespace blocks
} // namespace mc
