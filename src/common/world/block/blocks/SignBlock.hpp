/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../../core/BlockRaycastResult.hpp"
#include "../../../core/Types.hpp"
#include "../../../item/core/ActionResult.hpp"
#include "../../../physics/collision/CollisionShape.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../../../util/property/Properties.hpp"
#include "../Block.hpp"
#include "../IWaterLoggable.hpp"
#include "../Material.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <unordered_map>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockPos;
class BlockEntity;
class Player;

namespace blocks {

/**
 * @brief 告示牌木材类型
 *
 * 支持多种木材类型的告示牌。
 */
enum class WoodType : u8 {
    Oak = 0,
    Spruce = 1,
    Birch = 2,
    Jungle = 3,
    Acacia = 4,
    DarkOak = 5,
    Crimson = 6,  // 1.16 下界木材
    Warped = 7,   // 1.16 下界木材
    Mangrove = 8, // 1.19 红树林木材
    Cherry = 9,   // 1.20 樱花木材
    Bamboo = 10,  // 1.20 竹木材
    PaleOak = 11  // 1.21.2 苍白橡木木材
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

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 交互 ==========

    /**
     * @brief 处理玩家右键点击告示牌
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 手
     * @param hit 射线检测结果
     * @return 交互结果类型
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== IWaterLoggable 接口实现 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 木材类型 ==========

    [[nodiscard]] WoodType getWoodType() const { return m_woodType; }

protected:
    /// 木材类型
    WoodType m_woodType;

    /**
     * @brief 获取涂蜡告示牌交互失败的音效
     *
     * 普通告示牌返回 BLOCK_SIGN_WAXED_INTERACT_FAIL，
     * 悬挂告示牌子类覆盖返回 BLOCK_HANGING_SIGN_WAXED_INTERACT_FAIL。
     * 对应 MC Java 的 SignBlockEntity.getSignInteractionFailedSoundEvent()。
     *
     * @return 音效事件的资源定位符
     */
    [[nodiscard]] virtual const ResourceLocation& getWaxedInteractFailSound() const;
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
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

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
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

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
