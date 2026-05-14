#pragma once

#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;

namespace blocks {

/**
 * @brief 床方块
 *
 * 双方块结构（头部和脚部），支持16种颜色。
 * 在下界和末地会爆炸，在主世界可以设置重生点。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST)
 * - BED_PART: 部分 (HEAD, FOOT)
 * - OCCUPIED: 是否被占用
 *
 * 参考: net.minecraft.block.BedBlock
 */
class BedBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param color 颜色ID (0-15)
     * @param properties 方块属性
     */
    BedBlock(u32 color, const BlockProperties& properties);
    ~BedBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 硬度 ==========

    [[nodiscard]] f32 getExplosionResistance(const BlockState& state) const override
    {
        MC_UNUSED(state);
        // 在下界爆炸
        return 0.2f;
    }

    // ========== 工具方法 ==========

    /**
     * @brief 获取床的颜色
     */
    [[nodiscard]] u32 getColor() const { return m_color; }

    /**
     * @brief 设置床被占用状态
     */
    static void setOccupied(IWorld& world, const BlockPos& pos, BlockState& state, bool occupied);

    /**
     * @brief 检查床是否可用
     */
    [[nodiscard]] static bool isBed(IWorld& world, const BlockPos& pos);

    /**
     * @brief 右键交互 - 睡眠或爆炸
     *
     * MC Java: 在主世界可以睡眠设置重生点，在下界和末地会爆炸。
     */
    [[nodiscard]] ActionResultType onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

protected:
    /// 床颜色 (0-15, 对应16种染料颜色)
    u32 m_color;

    /// 各朝向的形状缓存
    std::array<CollisionShape, 6> m_shapesByFacing;
};

} // namespace blocks
} // namespace mc
