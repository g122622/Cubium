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

#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/Material.hpp"
#include <memory>

namespace mc {

namespace fluid {
class Fluid;
} // namespace fluid

class World;
class BlockItemUseContext;
class Player;
class BlockRaycastResult;
class ItemStack;

namespace blocks {

/**
 * @brief 炼药锅方块
 *
 * 炼药锅是一个可以储存水的方块，没有方块实体，使用方块状态表示水位。
 *
 * TODO: 当前 CauldronBlock 同时承担空炼药锅和水炼药锅的职责。
 * MC 原版中，水炼药锅由 WaterCauldronBlock (LayeredCauldronBlock) 单独实现，
 * 空炼药锅不持有水位。当 WaterCauldronBlock/PowderSnowCauldronBlock 实现后，
 * 应将水位相关逻辑（handlePrecipitation、玻璃瓶/空桶取水等）迁移到对应子类。
 *
 * 状态属性：
 * - LEVEL: 水位 (0-3，0=空，3=满)
 *
 * 交互：
 * - 水桶：装水（水位变为3）
 * - 空桶：取水（水位-1）
 * - 玻璃瓶：取水（水位-1，变为水瓶）
 * - 水瓶：倒水（水位+1，变为玻璃瓶）
 * - 皮革盔甲：清洗（水位-1，移除颜色，触发 BLOCK_CHANGE）
 * - 旗帜/盾牌：清洗（水位-1，移除最顶层图案，触发 BLOCK_CHANGE）
 *
 * 降水处理：
 * - 雨天：5% 概率增加水位
 * - 雪天：10% 概率增加水位
 * 降水处理通过 handlePrecipitation 在 tickPrecipitation 中调用，
 * 而非 randomTick。
 */
class CauldronBlock : public Block {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CauldronBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~CauldronBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 邻居方块更新
     * @param world 世界
     * @param pos 当前方块位置
     * @param neighborBlock 邻居方块
     * @param neighborPos 邻居位置
     * @param isMoving 是否正在移动
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief 降水处理
     *
     * 在降水 tick 中被调用，用于炼药锅收集降水。
     * - 雨天：5% 概率增加水位（如果未满）
     * - 雪天：10% 概率增加水位（如果未满）
     *
     * @param world 世界
     * @param pos 方块位置
     * @param precipitation 降水类型（Rain / Snow）
     */
    void handlePrecipitation(
        IWorld& world, const BlockPos& pos, world::biome::BiomeClimate::Precipitation precipitation) override;

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
    [[nodiscard]] ActionResultType onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 形状 ==========

    /**
     * @brief 获取渲染形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取实体内部碰撞形状
     *
     * 返回炼药锅外部形状与水位内容区域的并集。实体只有进入内容区域
     * 才会触发 onEntityCollision（如灭火）。
     * 空炼药锅（水位0）返回完整方块形状，因为 MC 原版空炼药锅的
     * getEntityInsideCollisionShape 继承默认行为返回 Shapes.block()。
     * 有水时返回与水位对应的内容填充形状。
     *
     * 参考: net.minecraft.block.LayeredCauldronBlock#getEntityInsideCollisionShape
     * 原版使用 FILLED_SHAPES[level - 1]，其中 level 范围 1-3
     */
    [[nodiscard]] const CollisionShape& getEntityInsideCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取内部形状（用于水位渲染）
     * @param level 水位 (0-3)
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getContentShape(i32 level) const;

    // ========== 实体碰撞 ==========

    /**
     * @brief 实体进入水炼药锅时灭火并降低水位
     *
     * 当着火的实体进入有水的炼药锅时，实体会被灭火，同时水位降低 1 级。
     * 空炼药锅（水位0）不执行任何操作。
     *
     * 参考: net.minecraft.block.LayeredCauldronBlock#entityInside
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 红石 ==========

    /**
     * @brief 获取红石比较器信号
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 信号强度 (0-3对应水位)
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 检查是否有红石比较器输入覆盖
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 获取水位
     * @param state 方块状态
     * @return 水位 (0-3)
     */
    [[nodiscard]] static i32 getLevel(const BlockState& state);

    /**
     * @brief 设置水位
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param level 新水位 (0-3)
     */
    static void setLevel(IWorld& world, const BlockPos& pos, const BlockState& state, i32 level);

    /**
     * @brief 检查是否为空
     * @param state 方块状态
     * @return 如果水位为0返回true
     */
    [[nodiscard]] static bool isEmpty(const BlockState& state);

    /**
     * @brief 检查是否已满
     * @param state 方块状态
     * @return 如果水位为3返回true
     */
    [[nodiscard]] static bool isFull(const BlockState& state);

    // ========== 滴石填充 ==========

    /**
     * @brief 方块 tick 处理
     *
     * 由 PointedDripstoneBlock::maybeTransferFluid() 调度，用于接收滴石滴水填充。
     * 在 tick 中调用 PointedDripstoneBlock::findStalactiteTipAboveCauldron 和
     * PointedDripstoneBlock::getCauldronFillFluidType 重新验证滴石尖端和流体类型，
     * 然后调用 receiveStalactiteDrip 填充炼药锅。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 判断是否可以接收滴石滴水
     *
     * 空炼药锅可以接收任何流体（水和岩浆）的滴石滴水。
     *
     * @param fluid 流体类型
     * @return 始终返回 true
     */
    [[nodiscard]] static bool canReceiveStalactiteDrip(const fluid::Fluid& fluid);

    /**
     * @brief 接收滴石滴水填充
     *
     * 根据流体类型处理炼药锅填充：
     * - 水滴 → 每次增加1级水位
     * - 岩浆滴 → 替换为岩浆炼药锅
     *
     * TODO: 当实现 WaterCauldronBlock (LayeredCauldronBlock) 后，
     * 水滴填充应将空炼药锅替换为水位1的 WaterCauldronBlock，
     * 而非在当前 CauldronBlock 上递增水位。岩浆滴替换为 LavaCauldronBlock 保持不变。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param fluid 流体类型
     */
    static void receiveStalactiteDrip(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid);

private:
    /**
     * @brief 处理水桶交互
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param player 玩家
     * @param heldItem 手持物品
     * @return 交互结果
     */
    ActionResultType _handleBucketInteraction(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /**
     * @brief 处理玻璃瓶交互
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param player 玩家
     * @param heldItem 手持物品
     * @return 交互结果
     */
    ActionResultType _handleBottleInteraction(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /**
     * @brief 处理皮革盔甲清洗
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param player 玩家
     * @param heldItem 手持物品
     * @return 交互结果
     */
    ActionResultType _handleLeatherArmorCleaning(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /**
     * @brief 处理旗帜清洗
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param player 玩家
     * @param heldItem 手持物品
     * @return 交互结果
     */
    ActionResultType _handleBannerCleaning(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /**
     * @brief 播放加水音效
     * @param world 世界
     * @param pos 方块位置
     */
    void _playFillSound(IWorld& world, const BlockPos& pos);

    /**
     * @brief 播放取水音效
     * @param world 世界
     * @param pos 方块位置
     */
    void _playEmptySound(IWorld& world, const BlockPos& pos);

    /// 炼药锅外部形状
    CollisionShape m_outerShape;

    /// 不同水位的内容形状（用于渲染）
    std::array<CollisionShape, 4> m_contentShapes;

    /// 不同水位的填充形状（外部形状 ∪ 内容形状，用于实体内部碰撞检测）
    /// 参考 MC 原版: LayeredCauldronBlock.FILLED_SHAPES[level - 1]
    /// 索引 0 = 水位1, 索引 1 = 水位2, 索引 2 = 水位3
    std::array<CollisionShape, 3> m_filledShapes;
};

} // namespace blocks
} // namespace mc
