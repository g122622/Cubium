#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockEntity;

namespace blocks {

/**
 * @brief 唱片机方块
 *
 * 可以播放音乐唱片的功能方块。
 *
 * 状态属性：
 * - HAS_RECORD: 是否有唱片
 *
 * 参考: net.minecraft.block.JukeboxBlock
 */
class JukeboxBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit JukeboxBlock(const BlockProperties& properties);
    ~JukeboxBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 红石 ==========

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
     * @brief 检查是否有唱片
     */
    [[nodiscard]] static bool hasRecord(const BlockState& state) {
        return state.get(BlockStateProperties::HAS_RECORD());
    }

    /**
     * @brief 设置唱片状态
     */
    static void setRecord(IWorld& world, const BlockPos& pos, BlockState& state, bool hasRecord);

protected:
    /// 唱片机形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
