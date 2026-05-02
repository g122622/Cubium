#pragma once

#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 台阶方块
 *
 * 支持单层和双层状态，双层时变成完整方块。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 注意：双层台阶不能含水，只有单层台阶可以含水。
 *
 * 状态属性：
 * - TYPE: 台阶类型 (BOTTOM, TOP, DOUBLE)
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.SlabBlock
 */
class SlabBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit SlabBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~SlabBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    /**
     * @brief 检查方块是否可被替换
     *
     * 单层台阶可被同类型台阶替换以形成双层台阶。
     * 双层台阶不可被替换。
     *
     * 参考: net.minecraft.block.SlabBlock#isReplaceable
     */
    [[nodiscard]] bool isReplaceable(
        const BlockState& state,
        BlockItemUseContext& context) const override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 其他 ==========

    /**
     * @brief 检查是否为双层
     * @param state 方块状态
     * @return 如果是双层返回true
     */
    [[nodiscard]] static bool isDouble(const BlockState& state);

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     *
     * 如果方块含水且不是双层，返回水的流体状态。
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /**
     * @brief 检查是否可以容纳流体
     *
     * 双层台阶不能含水。
     */
    [[nodiscard]] bool canContainFluid(
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const fluid::Fluid& fluid) const override;

    /**
     * @brief 接收流体
     *
     * 双层台阶不能接收流体。
     */
    bool receiveFluid(
        IWorld& world,
        const BlockPos& pos,
        const BlockState* state,
        const fluid::FluidState& fluidState) override;

private:
    /// 下半台阶形状
    CollisionShape m_bottomShape;

    /// 上半台阶形状
    CollisionShape m_topShape;

    /// 完整方块形状
    CollisionShape m_fullCubeShape;
};

} // namespace blocks
} // namespace mc
