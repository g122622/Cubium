#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"
#include "../../Material.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 锁链方块
 *
 * 锁链是一种装饰性金属方块：
 * - 可以沿任意轴放置
 * - 可以水平或垂直放置
 * - 可以攀爬
 * - 实现 IWaterLoggable 接口支持含水功能
 *
 * 参考: net.minecraft.block.ChainBlock
 */
class ChainBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ChainBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 获取放置状态
     */
    BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    /**
     * @brief 旋转方块
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 更新 ==========

    /**
     * @brief 邻居更新
     */
    BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

protected:
    /// X轴形状
    CollisionShape m_xShape;
    /// Y轴形状
    CollisionShape m_yShape;
    /// Z轴形状
    CollisionShape m_zShape;
};

} // namespace blocks
} // namespace mc
