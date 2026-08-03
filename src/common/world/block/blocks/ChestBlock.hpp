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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <memory>

namespace mc {

class World;
class BlockItemUseContext;
class ChestEntity;

namespace blocks {

/**
 * @brief 箱子方块
 *
 * 方块状态属性：
 * - FACING: 朝向（北/南/东/西）
 * - TYPE: 箱子类型（SINGLE/LEFT/RIGHT）
 * - WATERLOGGED: 是否含水
 *
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 参考: net.minecraft.block.ChestBlock
 *
 * 双箱机制：
 * - 当两个箱子相邻放置时会自动合并
 * - 左箱子(ChestType::LEFT)向右连接
 * - 右箱子(ChestType::RIGHT)向左连接
 * - 合并后形成54格双箱
 */
class ChestBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ChestBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~ChestBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 获取放置时的方块状态
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 邻居方块更新
     * @param state 当前方块状态
     * @param world 世界
     * @param pos 当前位置
     * @param neighborBlock 邻居方块
     * @param neighborPos 邻居位置
     * @param isMoving 是否正在移动
     * @return 更新后的方块状态
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& neighborState,
        IWorld& world,
        const BlockPos& pos,
        const BlockPos& neighborPos) override;

    // ========== 方块实体 ==========

    /**
     * @brief 检查是否有方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建方块实体
     * @param pos 方块位置
     * @return 方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键点击
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 手
     * @param hit 射线检测结果
     * @return 交互结果
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 红石 ==========

    /**
     * @brief 检查是否可以提供红石信号
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        return false; // 普通箱子不提供信号
    }

    /**
     * @brief 检查是否有比较器输入覆盖
     *
     * 箱子可以通过比较器输出信号，因此需要返回 true。
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取红石比较器信号
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 旋转和镜像 ==========

    /**
     * @brief 旋转箱子方块
     *
     * 旋转箱子的朝向（HORIZONTAL_FACING），保持类型和含水状态。
     *
     * @param state 方块状态
     * @param rotation 旋转类型
     * @return 旋转后的方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像箱子方块
     *
     * 镜像箱子的朝向（HORIZONTAL_FACING），并交换左/右类型。
     *
     * @param state 方块状态
     * @param mirror 镜像类型
     * @return 镜像后的方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 移除处理 ==========

    /**
     * @brief 方块被移除时的处理
     *
     * 箱子被移除时需要掉落其内容物。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 静态工具方法 ==========

    /**
     * @brief 获取箱子连接的方向
     * @param state 方块状态
     * @return 连接方向，如果是单箱返回Direction::None
     */
    [[nodiscard]] static Direction getConnectedDirection(const BlockState& state);

    /**
     * @brief 检查位置是否被阻挡
     * @param world 世界
     * @param pos 位置
     * @return 如果被阻挡返回true
     */
    [[nodiscard]] static bool isBlocked(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查猫是否坐在箱子上
     * @param world 世界
     * @param pos 位置
     * @return 如果有猫坐着返回true
     */
    [[nodiscard]] static bool isCatSittingOn(IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取方块实体类型
     * @return 方块实体类型
     */
    [[nodiscard]] virtual BlockEntityType getBlockEntityType() const { return BlockEntityType::Chest; }

    // ========== 开合音效 ==========

    /**
     * @brief 获取箱子打开音效
     *
     * 默认返回普通箱子的 BLOCK_CHEST_OPEN。
     * 铜箱子子类（CopperChestBlock）重写此方法，根据氧化等级返回对应的声音事件。
     *
     * 参考: net.minecraft.world.level.block.ChestBlock#getOpenChestSound (MC 1.21.11)
     */
    [[nodiscard]] virtual const ResourceLocation& getOpenSound() const;

    /**
     * @brief 获取箱子关闭音效
     *
     * 默认返回普通箱子的 BLOCK_CHEST_CLOSE。
     * 铜箱子子类（CopperChestBlock）重写此方法，根据氧化等级返回对应的声音事件。
     *
     * 参考: net.minecraft.world.level.block.ChestBlock#getCloseChestSound (MC 1.21.11)
     */
    [[nodiscard]] virtual const ResourceLocation& getCloseSound() const;

    // ========== 双箱连接 ==========

    /**
     * @brief 检查相邻方块是否可以与当前箱子连接为双箱
     *
     * 默认实现：邻居方块类型与当前方块一致时返回 true。
     * 铜箱子重写此方法：邻居在 COPPER_CHESTS 标签中且拥有 CHEST_TYPE 属性时返回 true，
     * 允许跨氧化等级与涂蜡状态的双箱合并。
     *
     * 参考: net.minecraft.world.level.block.ChestBlock#chestCanConnectTo (MC 1.21.11)
     *
     * @param neighborState 相邻方块状态
     * @return 如果可以连接返回 true
     */
    [[nodiscard]] virtual bool chestCanConnectTo(const BlockState& neighborState) const;

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
    /**
     * @brief 合并两个箱子
     * @param state 当前方块状态
     * @param world 世界
     * @param pos 当前位置
     * @param facing 合并方向
     */
    void combineChests(const BlockState& state, IWorld& world, const BlockPos& pos, Direction facing);

    /**
     * @brief 检查相邻位置是否可以合并
     * @param world 世界
     * @param pos 当前位置
     * @param facing 检查方向
     * @param expectedFacing 期望的朝向
     * @return 如果可以合并返回true
     */
    [[nodiscard]] bool canCombineWithChestAt(
        IWorld& world, const BlockPos& pos, Direction facing, Direction expectedFacing) const;
};

} // namespace blocks
} // namespace mc
