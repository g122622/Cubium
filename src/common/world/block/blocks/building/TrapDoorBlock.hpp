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
class BlockRaycastResult;

namespace blocks {

/**
 * @brief 活板门方块
 *
 * 可被玩家或红石控制开关，可以放置在方块的顶部或底部。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 水平朝向 (NORTH, SOUTH, EAST, WEST)
 * - OPEN: 是否打开
 * - HALF: 上半/下半 (TOP, BOTTOM)
 * - POWERED: 是否被充能
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.TrapDoorBlock
 */
class TrapDoorBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param isIron 是否为铁活板门（只能红石控制）
     */
    TrapDoorBlock(const BlockProperties& properties, bool isIron = false);

    /**
     * @brief 析构函数
     */
    ~TrapDoorBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    void neighborChanged(IWorld& world, const BlockPos& pos,
                         Block& neighborBlock, const BlockPos& neighborPos,
                         bool isMoving) override;

    // ========== 交互 ==========

    [[nodiscard]] ActionResultType onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 红石 ==========

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 静态方法 ==========

    /**
     * @brief 检查活板门是否打开
     * @param state 方块状态
     * @return 如果打开返回true
     */
    [[nodiscard]] static bool isOpen(const BlockState& state);

    /**
     * @brief 切换活板门开关状态
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前状态
     * @param open 是否打开
     */
    static void toggle(IWorld& world, const BlockPos& pos, const BlockState& state, bool open);

    /**
     * @brief 检查是否为铁活板门
     * @return 如果是铁活板门返回true
     */
    [[nodiscard]] bool isIronTrapdoor() const { return m_isIron; }

    // ========== 推动反应 ==========

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override {
        MC_UNUSED(state);
        return Material::PushReaction::Destroy;
    }

private:
    /**
     * @brief 播放开关音效
     * @param world 世界
     * @param pos 方块位置
     * @param isOpening 是否正在打开
     */
    static void playSound(IWorld& world, const BlockPos& pos, bool isOpening);

    /**
     * @brief 获取形状索引
     * @param facing 朝向
     * @param open 是否打开
     * @param half 上半/下半
     * @return 形状索引
     */
    [[nodiscard]] static size_t getShapeIndex(Direction facing, bool open, BlockStateProperties::DoubleBlockHalf half);

    /// 是否为铁活板门
    bool m_isIron;

    /// 预计算的形状缓存 (4 facing * 2 open * 2 half = 16)
    std::array<CollisionShape, 16> m_shapes;
};

} // namespace blocks
} // namespace mc
