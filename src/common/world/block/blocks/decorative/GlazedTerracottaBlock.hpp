#pragma once

#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 釉面陶瓦方块
 *
 * 釉面陶瓦是一种装饰性方块：
 * - 可以旋转（4个方向）
 * - 图案根据朝向旋转
 * - 16种颜色
 * - 不可被粘性活塞拉动
 *
 * 参考: net.minecraft.block.GlazedTerracottaBlock
 */
class GlazedTerracottaBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit GlazedTerracottaBlock(const BlockProperties& properties);

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

    /**
     * @brief 镜像方块
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;
};

} // namespace blocks
} // namespace mc
