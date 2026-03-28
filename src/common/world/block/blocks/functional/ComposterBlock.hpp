#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;
namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 堆肥桶方块
 *
 * 用于将植物材料转化为骨粉的功能方块。
 * 具有8个填充等级（0-7），满级后可产出骨粉。
 *
 * 状态属性：
 * - LEVEL_0_8: 填充等级 (0-8，8表示完成)
 *
 * 参考: net.minecraft.block.ComposterBlock
 */
class ComposterBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ComposterBlock(const BlockProperties& properties);
    ~ComposterBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

    [[nodiscard]] bool ticksRandomly() const override { return false; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] int getComparatorInputOverride(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos) const override;

    // ========== 工具方法 ==========

    /**
     * @brief 获取填充等级
     */
    [[nodiscard]] static int getLevel(const BlockState& state) {
        return state.get(BlockStateProperties::LEVEL_0_8());
    }

    /**
     * @brief 尝试堆肥
     * @return 新的方块状态（可能改变等级）
     */
    static BlockState attemptCompost(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        u32 itemId);

    /**
     * @brief 清空堆肥桶
     */
    static BlockState empty(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 检查物品是否可堆肥
     */
    [[nodiscard]] static bool isCompostable(u32 itemId);

    /**
     * @brief 获取物品的堆肥概率
     * @return 0.0-1.0之间的概率，-1表示不可堆肥
     */
    [[nodiscard]] static float getCompostChance(u32 itemId);

protected:
    /// 各等级的形状缓存
    std::array<CollisionShape, 9> m_shapesByLevel;

    /// 碰撞形状（等级0的形状）
    CollisionShape m_collisionShape;
};

} // namespace blocks
} // namespace mc
