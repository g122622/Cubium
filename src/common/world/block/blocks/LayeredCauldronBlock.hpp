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
 * LIABILITY, CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include <array>
#include <memory>

namespace mc {

namespace fluid {
class Fluid;
} // namespace fluid

class World;
class Player;
class BlockRaycastResult;
class ItemStack;

namespace blocks {

/**
 * @brief 分层炼药锅方块（水炼药锅 / 细雪炼药锅）
 *
 * 分层炼药锅方块（水炼药锅 / 细雪炼药锅），表示含有水位（或细雪位）的炼药锅。
 * 当水位降至0时，方块应替换为空炼药锅 (CauldronBlock)。
 * 通过 m_precipitationType 区分水炼药锅（Rain）和细雪炼药锅（Snow）。
 *
 * 状态属性：
 * - LEVEL: 水位 (1-3，1=最低，3=满)
 *
 * 降水处理：
 * - 当降水类型与炼药锅类型匹配时（雨水→水炼药锅，雪→细雪炼药锅），
 *   有概率增加水位（雨天5%，雪天10%），仅在水位未满时触发
 *
 * 水炼药锅交互（m_precipitationType == Rain）：
 * - 水桶：装满水位至3
 * - 空桶：取水（仅满水位时可用，变为水桶+替换为空炼药锅）
 * - 细雪桶：替换为水炼药锅（水位3）
 * - 岩浆桶：替换为岩浆炼药锅
 * - 玻璃瓶：取水（水位-1，变为水瓶）
 * - 水瓶：倒水（水位+1，变为玻璃瓶）
 * - 皮革盔甲：清洗（水位-1，移除颜色）
 * - 旗帜/盾牌：清洗（水位-1，移除最顶层图案）
 *
 * 细雪炼药锅交互（m_precipitationType == Snow）：
 * - 空桶：取细雪（仅满水位时可用，变为细雪桶+替换为空炼药锅）
 * - 细雪桶：装满细雪水位至3
 * - 水桶：替换为水炼药锅（水位3）
 * - 岩浆桶：替换为岩浆炼药锅
 * - 玻璃瓶/水瓶/皮革盔甲/旗帜：不支持
 *
 * 实体碰撞：
 * - 水炼药锅：着火实体进入时灭火并降低水位
 * - 细雪炼药锅：着火实体进入时灭火，细雪先转为水（保持水位），然后降低水位
 *
 * 滴石填充：
 * - 仅水炼药锅可接收水滴（细雪炼药锅不接收）
 * - 水滴增加水位1级，满时不再增加
 */
class LayeredCauldronBlock : public Block {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param precipitationType 此炼药锅响应的降水类型
     *        Rain = 水炼药锅，Snow = 细雪炼药锅
     */
    explicit LayeredCauldronBlock(
        const BlockProperties& properties, world::biome::BiomeClimate::Precipitation precipitationType);

    /**
     * @brief 析构函数
     */
    ~LayeredCauldronBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 邻居方块更新（无操作）
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief 降水处理
     *
     * 当降水类型与炼药锅类型匹配且有未满水位时，有概率增加水位。
     * - 雨天（水炼药锅）：5% 概率
     * - 雪天（细雪炼药锅）：10% 概率
     */
    void handlePrecipitation(
        IWorld& world, const BlockPos& pos, world::biome::BiomeClimate::Precipitation precipitation) override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键点击
     *
     * 根据手持物品处理不同的交互：
     * - 水桶：装满炼药锅（水位→3）或取水（满时→空炼药锅）
     * - 空桶：取水（仅满水位，→空炼药锅）
     * - 玻璃瓶：取水（水位-1）
     * - 水瓶：倒水（水位+1）
     * - 皮革盔甲：清洗（水位-1）
     * - 旗帜/盾牌：清洗（水位-1）
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 形状 ==========

    /**
     * @brief 获取渲染形状（与空炼药锅相同的外部形状）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（外部形状）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取实体内部碰撞形状
     *
     * 返回炼药锅外部形状与水位内容区域的并集。
     */
    [[nodiscard]] const CollisionShape& getEntityInsideCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取内部形状（用于渲染）
     * @param level 水位 (1-3)
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getContentShape(i32 level) const;

    // ========== 实体碰撞 ==========

    /**
     * @brief 实体进入分层炼药锅时灭火并降低水位
     *
     * 当着火的实体进入分层炼药锅时，实体会被灭火，同时水位降低 1 级。
     * 如果水位降至0，替换为空炼药锅。
     * 对于细雪炼药锅，灭火时先转换为水炼药锅（保持相同水位），然后降低水位。
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 红石 ==========

    /**
     * @brief 获取红石比较器信号（= 水位 1-3）
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 始终有比较器输入覆盖
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
     * @return 水位 (1-3)
     */
    [[nodiscard]] static i32 getLevel(const BlockState& state);

    /**
     * @brief 设置水位
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param level 新水位 (1-3)
     */
    static void setLevel(IWorld& world, const BlockPos& pos, const BlockState& state, i32 level);

    /**
     * @brief 降低水位1级，水位降至0时替换为空炼药锅
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    static void lowerFillLevel(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 检查是否已满（水位=3）
     * @param state 方块状态
     * @return 如果水位为3返回true
     */
    [[nodiscard]] static bool isFull(const BlockState& state);

    // ========== 滴石填充 ==========

    /**
     * @brief 方块 tick 处理（滴石滴水）
     *
     * 重新验证上方钟乳石尖端，接收水滴填充。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 判断是否可以接收滴石滴水
     *
     * 仅水炼药锅（precipitationType==Rain）可接收水滴，且水位未满时。
     * 细雪炼药锅不接收滴水。
     */
    [[nodiscard]] bool canReceiveStalactiteDrip(const fluid::Fluid& fluid) const;

    /**
     * @brief 接收滴石滴水填充
     *
     * 水滴增加1级水位（如果未满）。
     */
    void receiveStalactiteDrip(IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid);

    // ========== 方块状态 ==========

    /**
     * @brief 分层炼药锅需要形状遮挡检测
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

private:
    /**
     * @brief 处理水桶交互
     */
    ActionResultType _handleBucketInteraction(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /**
     * @brief 处理玻璃瓶交互
     */
    ActionResultType _handleBottleInteraction(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /**
     * @brief 处理皮革盔甲清洗
     */
    ActionResultType _handleLeatherArmorCleaning(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /**
     * @brief 处理旗帜清洗
     */
    ActionResultType _handleBannerCleaning(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /// 此炼药锅响应的降水类型（Rain=水炼药锅，Snow=细雪炼药锅）
    world::biome::BiomeClimate::Precipitation m_precipitationType;

    /// 炼药锅外部形状（与空炼药锅共享）
    CollisionShape m_outerShape;

    /// 不同水位的内容形状（索引0=水位1，索引1=水位2，索引2=水位3）
    std::array<CollisionShape, 3> m_contentShapes;

    /// 不同水位的填充形状（外部形状 ∪ 内容形状，用于实体内部碰撞检测）
    std::array<CollisionShape, 3> m_filledShapes;
};

} // namespace blocks
} // namespace mc
