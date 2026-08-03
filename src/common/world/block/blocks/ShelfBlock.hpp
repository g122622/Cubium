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

#include "HorizontalBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IWaterLoggable.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace blockentity {
class ShelfBlockEntity; // 前向声明
}

class PlayerInventory; // 前向声明

namespace blocks {

/**
 * @brief 书架方块
 *
 * 1.21.4+ 新增的木质书架方块，可放置3个物品（任意物品，不限于书籍）。
 * 继承自 HorizontalBlock，实现 IWaterLoggable 接口。
 *
 * 方块状态属性：
 * - FACING: 朝向（北/南/东/西），继承自 HorizontalBlock
 * - POWERED: 红石充能状态
 * - SIDE_CHAIN_PART: 侧链连接部分（Unconnected/Left/Center/Right）
 * - WATERLOGGED: 是否含水
 *
 * 红石机制：
 * - 当书架接收到红石信号时 POWERED=true，失去信号时 POWERED=false
 * - POWERED=true 时，相邻的同类书架会形成侧链连接（最多3个一组）
 * - 侧链连接时 SIDE_CHAIN_PART 自动更新为 Left/Center/Right
 * - POWERED=false 时断开所有侧链连接，SIDE_CHAIN_PART 重置为 Unconnected
 *
 * 交互机制：
 * - 未充能时：点击书架正面可放入/取出单个物品
 * - 充能时：点击书架正面会进行热栏整体交换（最多9个槽位 = 3书架 × 3槽位）
 *
 * 红石比较器输出：
 * - 3位二进制编码，每个槽位占用对应1位（0-7）
 * - 比较器必须从书架背面读取（与FACING相反的方向）
 *
 * 注意：Shelf 方块不能为附魔台提供附魔能量（与 Bookshelf 不同）。
 *
 * 参考: net.minecraft.block.ShelfBlock (MC 1.21.11)
 */
class ShelfBlock : public HorizontalBlock, public IWaterLoggable {
public:
    /// 书架行数（1行）
    static constexpr i32 ROWS = 1;
    /// 书架列数（3列，3个槽位）
    static constexpr i32 COLUMNS = 3;
    /// 侧链最大长度（最多3个书架连接）
    static constexpr i32 MAX_CHAIN_LENGTH = 3;

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ShelfBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~ShelfBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据玩家朝向设置 FACING，根据红石信号设置 POWERED，
     * 根据水中位置设置 WATERLOGGED。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 邻居方块形状/状态更新
     *
     * 处理 WATERLOGGED 的流体 tick 调度。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 方块被添加到世界时的回调
     *
     * 当 POWERED=true 时，执行侧链连接计算。
     * 当 POWERED=false 时，断开邻居侧链连接。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居方块变化时的回调
     *
     * 检测红石信号变化并更新 POWERED 状态。
     * 当 POWERED 变为 true 时连接侧链，变为 false 时断开侧链。
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief 方块被移除时的回调
     *
     * 掉落书架内物品，并断开侧链连接。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 碰撞箱 ==========

    /**
     * @brief 获取方块的碰撞形状
     *
     * 书架使用基于朝向的非完整方块碰撞箱。
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡
     *
     * 书架使用非完整方块的碰撞箱，需要基于形状计算光照遮挡。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 红石 ==========

    /**
     * @brief 检查是否有比较器输入覆盖
     *
     * 书架支持比较器读取3个槽位的占用状态。
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取红石比较器信号
     *
     * 使用3位二进制编码，从 ShelfBlockEntity 读取每个槽位的占用状态：
     * - 槽位0占用: bit 0 = 1
     * - 槽位1占用: bit 1 = 2
     * - 槽位2占用: bit 2 = 4
     * - 最大输出: 7
     *
     * 比较器必须从书架背面读取（与FACING相反的方向）。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 信号强度 (0-7)
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

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

    // ========== 交互 ==========

    /**
     * @brief 玩家右键交互
     *
     * 未充能时：点击书架正面可放入/取出单个物品。
     * 充能时：点击书架正面会进行热栏整体交换。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 交互手
     * @param hit 射线追踪结果
     * @return 交互结果
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 旋转和镜像 ==========

    /**
     * @brief 旋转书架方块
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像书架方块
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 侧链连接 ==========

    /**
     * @brief 判断给定的方块状态是否可参与侧链连接
     *
     * 条件：方块在 WOODEN_SHELVES 标签中，拥有 POWERED 属性，且 POWERED=true。
     */
    [[nodiscard]] static bool isConnectable(const BlockState& state);

    /**
     * @brief 获取指定位置连接的所有书架位置（从左到右排列）
     *
     * 从指定位置的书架出发，沿着其 facing 方向的左右两侧搜索连接的书架，
     * 返回按从左到右顺序排列的所有 BlockPos 列表。
     * 如果指定位置不可连接（非书架或未充能），返回空列表。
     */
    [[nodiscard]] static std::vector<BlockPos> getAllBlocksConnectedTo(IWorld& world, const BlockPos& pos);

private:
    /// 基于朝向的碰撞形状
    std::unordered_map<Direction, CollisionShape> m_shapesByDirection;

    // ========== 侧链连接内部方法 ==========

    /**
     * @brief 当书架充能时，更新自身及邻居的侧链连接
     *
     * 检查左右邻居是否可连接，根据侧链长度限制（最多3个）决定连接方式。
     */
    void updateSelfAndNeighborsOnPoweringUp(IWorld& world, const BlockPos& pos, const BlockState& currentState);

    /**
     * @brief 当书架断电时，通知邻居断开侧链连接
     */
    void updateNeighborsAfterPoweringDown(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 设置指定位置的 SIDE_CHAIN_PART 值
     *
     * 仅当新值与当前值不同时才更新方块状态。
     */
    static void setSideChainPart(IWorld& world, const BlockPos& pos, BlockStateProperties::SideChainPart part);

    /**
     * @brief 判断两个侧链是否可以连接
     * @param neighborChainSize 邻居侧链的大小
     * @param currentChainSize 当前侧链的大小
     * @return 是否可以连接
     */
    [[nodiscard]] static bool canConnect(i32 neighborChainSize, i32 currentChainSize);

    // ========== 交互内部方法 ==========

    /**
     * @brief 计算玩家点击书架正面时的命中槽位
     * @param hit 射线追踪结果
     * @param facing 书架朝向
     * @return 命中的槽位索引 (0-2)，如果未命中正面则返回 -1
     */
    [[nodiscard]] static i32 getHitSlot(const BlockRaycastResult& hit, Direction facing);

    /**
     * @brief 交换单个物品（未充能模式）
     *
     * 将玩家手持物品与书架指定槽位的物品交换。
     *
     * @return true 如果是从书架取出了物品（交换/取出），false 如果是放入物品
     */
    [[nodiscard]] static bool swapSingleItem(ItemStack& heldItem,
        Player& player,
        blockentity::ShelfBlockEntity& shelfEntity,
        i32 slotIndex,
        PlayerInventory& inventory);

    /**
     * @brief 交换热栏物品（充能模式）
     *
     * 将整个侧链中所有书架的物品与玩家热栏对应槽位交换。
     * 映射关系：左侧书架槽位 → 热栏低位，右侧书架槽位 → 热栏高位。
     *
     * @return true 如果发生了任何交换
     */
    [[nodiscard]] bool swapHotbar(IWorld& world, const BlockPos& pos, PlayerInventory& inventory) const;

    /**
     * @brief 播放书架音效
     */
    static void playSound(IWorld& world, const BlockPos& pos, const ResourceLocation& soundEvent);

    // ========== 状态容器 ==========

    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

} // namespace blocks
} // namespace mc
