#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 珊瑚颜色枚举
 */
enum class CoralColor : u8 {
    Tube = 0,      // 管状珊瑚（蓝色）
    Brain = 1,     // 脑珊瑚（粉色）
    Bubble = 2,    // 气泡珊瑚（紫色）
    Fire = 3,      // 火焰珊瑚（红色）
    Horn = 4       // 角珊瑚（黄色）
};

/**
 * @brief 珊瑚方块基类
 *
 * 水下的珊瑚方块，离开水会变成死珊瑚。
 *
 * 状态属性：
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.CoralBlock
 */
class CoralBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param color 珊瑚颜色
     * @param deadBlock 死珊瑚方块ID
     * @param properties 方块属性
     */
    CoralBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~CoralBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 水检测 ==========

    /**
     * @brief 检查是否在水中
     */
    [[nodiscard]] bool isInWater(const BlockState& state) const;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

protected:
    /**
     * @brief 检查周围是否有水
     */
    [[nodiscard]] bool isWaterNearby(IWorld& world, const BlockPos& pos) const;

    /// 珊瑚颜色
    CoralColor m_color;
    /// 死珊瑚方块ID
    u32 m_deadBlock;
};

/**
 * @brief 珊瑚扇方块
 *
 * 墙上的珊瑚扇，可以放置在墙面上。
 *
 * 状态属性：
 * - WATERLOGGED: 是否含水
 * - HORIZONTAL_FACING: 朝向
 *
 * 参考: net.minecraft.block.CoralFanBlock
 */
class CoralFanBlock : public Block {
public:
    CoralFanBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties);
    ~CoralFanBlock() override = default;

    // ========== 放置逻辑 ==========

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

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

protected:
    /**
     * @brief 检查是否可以附着到指定方向
     */
    [[nodiscard]] bool canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const;

    /// 珊瑚颜色
    CoralColor m_color;
    /// 死珊瑚方块ID
    u32 m_deadBlock;
};

/**
 * @brief 墙珊瑚扇方块
 *
 * 类似珊瑚扇，但专门用于墙面放置。
 *
 * 状态属性：
 * - WATERLOGGED: 是否含水
 * - FACING: 朝向
 *
 * 参考: net.minecraft.block.CoralWallFanBlock
 */
class CoralWallFanBlock : public Block {
public:
    CoralWallFanBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties);
    ~CoralWallFanBlock() override = default;

    // ========== 放置逻辑 ==========

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

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

protected:
    [[nodiscard]] bool canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const;

    CoralColor m_color;
    u32 m_deadBlock;
};

/**
 * @brief 珊瑚块方块
 *
 * 固体的珊瑚块，不会因缺水而死亡。
 *
 * 参考: net.minecraft.block.CoralBlockBlock
 */
class CoralBlockBlock : public Block {
public:
    explicit CoralBlockBlock(CoralColor color, const BlockProperties& properties);
    ~CoralBlockBlock() override = default;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    CoralColor m_color;
};

} // namespace blocks
} // namespace mc
