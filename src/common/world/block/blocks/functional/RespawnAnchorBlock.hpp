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
namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 重生锚方块
 *
 * 在下界设置重生点的功能方块。
 * 使用萤石充能，有4个充能等级。
 *
 * 状态属性：
 * - CHARGES: 充能等级 (0-4)
 *
 * 参考: net.minecraft.block.RespawnAnchorBlock
 */
class RespawnAnchorBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RespawnAnchorBlock(const BlockProperties& properties);
    ~RespawnAnchorBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 红石 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] int getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 工具方法 ==========

    /**
     * @brief 获取充能等级
     */
    [[nodiscard]] static int getCharges(const BlockState& state)
    {
        return state.get(BlockStateProperties::CHARGES_0_4());
    }

    /**
     * @brief 是否已充满
     */
    [[nodiscard]] static bool isFullyCharged(const BlockState& state) { return getCharges(state) >= 4; }

    /**
     * @brief 充能
     * @return 新的方块状态
     */
    static BlockState charge(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 消耗一次充能
     */
    static void discharge(IWorld& world, const BlockPos& pos, BlockState& state);

    // ========== 动态光照 ==========

    /**
     * @brief 获取动态光照等级
     *
     * 重生锚的光照等级根据充能等级变化：
     * - 0级: 0
     * - 1级: 3
     * - 2级: 7
     * - 3级: 11
     * - 4级: 15
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    /**
     * @brief 右键交互 - 充能或设置重生点
     *
     * MC Java:
     * - 使用萤石充能（需要物品检查）
     * - 在非下界维度使用会爆炸
     * - 充能后在非下界设置重生点
     */
    [[nodiscard]] ActionResultType onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

protected:
    /// 重生锚形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
