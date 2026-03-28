#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 锁链方块
 *
 * 锁链是一种装饰性金属方块：
 * - 可以沿任意轴放置
 * - 可以水平或垂直放置
 * - 可以攀爬
 * - 水logged支持
 *
 * 参考: net.minecraft.block.ChainBlock
 */
class ChainBlock : public Block {
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
