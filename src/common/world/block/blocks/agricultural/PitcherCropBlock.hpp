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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT OF THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include <array>
#include <utility>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 瓶草作物
 *
 * 瓶草作物有 5 个生长阶段（AGE_0_4），并使用 HALF 属性管理双格结构：
 * - AGE 0-2：单格作物（只有下半部分）
 * - AGE 3-4：双格作物（下半部分 + 上半部分）
 *
 * 碰撞形状：
 * - AGE 0（鳞茎阶段）：窄柱形（半径 6px，高 3px）
 * - AGE 1-4（作物阶段）：宽柱形（半径 10px，高 5px），仅下半部分有碰撞
 *
 * 生长机制：
 * - 随机刻生长，使用 CropBlock.getGrowthChance() 计算概率
 * - 骨粉每次增加 1 个生长阶段
 * - 从 AGE 2 生长到 AGE 3 时，上方自动放置上半部分
 *
 * 掉落：
 * - 未成熟（AGE < 4）：掉落瓶草荚果
 * - 成熟（AGE = 4）：可能额外掉落瓶草荚果（由战利品表控制）
 *
 * 参考: net.minecraft.world.level.block.PitcherCropBlock
 */
class PitcherCropBlock : public DoublePlantBlock, public IGrowable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit PitcherCropBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~PitcherCropBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取当前年龄
     */
    [[nodiscard]] i32 getAge(const BlockState& state) const;

    /**
     * @brief 获取最大年龄（4）
     */
    [[nodiscard]] i32 getMaxAge() const { return MAX_AGE; }

    /**
     * @brief 是否为最大年龄
     */
    [[nodiscard]] bool isMaxAge(const BlockState& state) const;

    /**
     * @brief 创建指定年龄的状态
     */
    [[nodiscard]] const BlockState& withAge(i32 age) const;

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置状态（返回默认状态 AGE=0, HALF=Lower）
     *
     * 瓶草作物只能通过种子放置，初始为下半部分、年龄 0。
     *
     * TODO: 此方法重写 Block::getStateForPlacement(BlockItemUseContext&)，
     * 但 SeedsItem 的放置路径通过 BlockItem::getStateForPlacement 返回 defaultState()，
     * 由于 defaultState() 已经是 AGE=0, HALF=Lower，功能上无影响。
     * 若将来需要根据放置上下文返回不同状态，需同步修改 BlockItem 放置路径。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查是否可以放置
     *
     * 下半部分：检查下方是否为耕地且光照充足
     * 上半部分：检查下方是否为同类型方块的下半部分
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
     *
     * 双格状态时委托给 DoublePlantBlock 的逻辑；
     * 单格状态时仅检查存活条件。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 生长逻辑 ==========

    /**
     * @brief 是否需要随机 tick（下半部分需要，上半部分不需要）
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    /**
     * @brief 随机刻（用于生长）
     *
     * 仅下半部分进行生长检查。使用 CropBlock.getGrowthChance() 计算生长概率。
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== IGrowable 接口实现 ==========

    /**
     * @brief 检查是否可以生长（未成熟时可以）
     *
     * 查找下半部分并检查其年龄是否未达到最大值。
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 骨粉是否有效（总是有效）
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长
     *
     * 找到下半部分，增加 1 个生长阶段。如果新年龄使植物变为双格，
     * 则自动在上方放置上半部分。
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     *
     * 根据年龄和半部分返回不同形状。
     * 上半部分无碰撞；下半部分根据年龄返回不同大小的柱形。
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状
     *
     * 仅下半部分有碰撞，上半部分返回空形状。
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 掉落物 ==========

    /**
     * @brief 获取作物物品（成熟时掉落瓶草植物）
     *
     * 注意：瓶草作物（PitcherCropBlock）继承自 DoublePlantBlock 而非 CropBlock，
     * 这是与 MC 1.21.11 原版一致的设计。FarmerWorkGoal 的收获逻辑通过
     * dynamic_cast<CropBlock*> 判断可收获方块，因此村民不会收获瓶草作物
     * （这也与原版行为一致：MC 中 HarvestFarmland 仅处理 CropBlock 类型）。
     *
     * 瓶草作物的掉落由战利品表驱动（minecraft:blocks/pitcher_crop），
     * getCropItem/getSeedItem 当前未被 FarmerWorkGoal 调用，保留供未来扩展使用
     * （例如其他 AI 或统计需要查询作物产物）。
     */
    [[nodiscard]] u32 getCropItem() const;

    /**
     * @brief 获取种子物品（瓶草荚果）
     *
     * 同 getCropItem 的说明：当前未被 FarmerWorkGoal 调用。
     * 村民种植瓶草荚果通过 VILLAGER_PLANTABLE_SEEDS 标签 + BlockItem 路径，
     * 不依赖此方法。
     */
    [[nodiscard]] u32 getSeedItem() const;

    // ========== 其他 ==========

    /**
     * @brief 检查下方是否可支撑（必须是耕地）
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;

    /**
     * @brief 玩家破坏时处理双格结构
     */
    void playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player) override;

    /**
     * @brief 实体进入方块碰撞区域时的回调（掠夺者破坏作物逻辑）
     *
     * 当 Ravager 实体进入瓶草作物方块且 mobGriefing 游戏规则开启时，
     * 方块会被破坏并掉落物品。仅在服务端执行。
     *
     * 参考: net.minecraft.world.level.block.PitcherCropBlock#entityInside
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    /**
     * @brief 方块是否可被替换（瓶草作物不可被替换放置）
     */
    [[nodiscard]] bool isReplaceable(const BlockState& state, const BlockItemUseContext& context) const override;

    /**
     * @brief 获取植物类型
     */
    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 判断给定年龄是否为双格状态
     */
    [[nodiscard]] static bool isDouble(i32 age);

    /**
     * @brief 在指定位置放置瓶草作物（同时放置上半部分如果需要）
     *
     * @param world 世界
     * @param pos 下半部分位置
     * @param age 年龄
     * @param flags 更新标志
     * @return 是否成功放置
     */
    static bool placeAt(IWorld& world, const BlockPos& pos, i32 age, i32 flags);

    /**
     * @brief 最大年龄常量
     */
    static constexpr i32 MAX_AGE = 4;

    /**
     * @brief 双格植物的年龄阈值（AGE >= 此值时变为双格）
     */
    static constexpr i32 DOUBLE_PLANT_AGE_INTERSECTION = 3;

private:
    /**
     * @brief 检查指定位置是否可以生长到目标年龄
     *
     * 如果新年龄使植物变为双格（age >= 3），则检查上方是否有空间。
     */
    [[nodiscard]] bool canGrowInto(IWorld& world, const BlockPos& pos, i32 newAge) const;

    /**
     * @brief 检查光照是否足够（原始亮度 >= 8）
     */
    [[nodiscard]] static bool hasSufficientLight(IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取下半部分的位置和状态
     *
     * 如果当前是上半部分，则查找下方；如果是下半部分，则直接返回。
     */
    [[nodiscard]] static std::pair<BlockPos, const BlockState*> getLowerHalf(
        IWorld& world, const BlockPos& pos, const BlockState& state);

    /// 形状缓存：[半部分索引][年龄]，Lower=0, Upper=1
    std::array<std::array<CollisionShape, MAX_AGE + 1>, 2> m_shapesByHalfAndAge;

    /// 鳞茎阶段碰撞形状（AGE=0）
    CollisionShape m_bulbCollisionShape;
    /// 作物阶段碰撞形状（AGE=1-4）
    CollisionShape m_cropCollisionShape;
};

} // namespace blocks
} // namespace mc
