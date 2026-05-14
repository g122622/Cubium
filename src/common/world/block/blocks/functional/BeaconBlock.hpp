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
 * @brief 信标方块
 *
 * 提供增益效果的功能方块。
 * 需要金字塔基座才能激活，基座层数决定可用效果。
 *
 * 参考: net.minecraft.block.BeaconBlock
 */
class BeaconBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BeaconBlock(const BlockProperties& properties);
    ~BeaconBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 光照 ==========

    /**
     * @brief 获取光照等级
     *
     * 信标始终发出15级光照。
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级 (15)
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return 15;
    }

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 红石 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] int getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

protected:
    /// 信标形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
