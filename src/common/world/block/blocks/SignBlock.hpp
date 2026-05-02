#pragma once

#include "../Block.hpp"
#include "../IWaterLoggable.hpp"
#include "../Material.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockPos;
class BlockEntity;

namespace blocks {

/**
 * @brief 告示牌木材类型
 *
 * MC 1.16.5 支持多种木材类型的告示牌。
 */
enum class WoodType : u8 {
    Oak = 0,
    Spruce = 1,
    Birch = 2,
    Jungle = 3,
    Acacia = 4,
    DarkOak = 5,
    Crimson = 6,   // 1.16 下界木材
    Warped = 7     // 1.16 下界木材
};

/**
 * @brief 告示牌抽象基类
 *
 * 提供告示牌的通用功能，包括含水支持。
 * 子类：StandingSignBlock（站立告示牌）和 WallSignBlock（墙面告示牌）
 *
 * 参考: net.minecraft.block.AbstractSignBlock
 */
class AbstractSignBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param woodType 木材类型
     */
    AbstractSignBlock(const BlockProperties& properties, WoodType woodType);

    /**
     * @brief 析构函数
     */
    ~AbstractSignBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    // ========== IWaterLoggable 接口实现 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 木材类型 ==========

    [[nodiscard]] WoodType getWoodType() const { return m_woodType; }

protected:
    /// 木材类型
    WoodType m_woodType;
};

/**
 * @brief 站立告示牌
 *
 * 放置在地面上的告示牌，有16个旋转方向（每22.5度）。
 *
 * 状态属性：
 * - ROTATION: 0-15，表示16个旋转方向
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.StandingSignBlock
 */
class StandingSignBlock : public AbstractSignBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param woodType 木材类型
     */
    StandingSignBlock(const BlockProperties& properties, WoodType woodType);

    /**
     * @brief 析构函数
     */
    ~StandingSignBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    /// 站立告示牌碰撞形状（告示牌杆）
    CollisionShape m_shape;
};

/**
 * @brief 墙面告示牌
 *
 * 附着在墙面上的告示牌，有4个水平朝向。
 *
 * 状态属性：
 * - FACING: 北、南、东、西四个方向
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.WallSignBlock
 */
class WallSignBlock : public AbstractSignBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param woodType 木材类型
     */
    WallSignBlock(const BlockProperties& properties, WoodType woodType);

    /**
     * @brief 析构函数
     */
    ~WallSignBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    /// 各方向的碰撞形状
    std::unordered_map<Direction, CollisionShape> m_shapesByDirection;
};

} // namespace blocks
} // namespace mc
