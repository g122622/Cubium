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

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/villager/WorkAtJobSiteGoal.hpp"
#include "common/world/block/BlockPos.hpp"

#include <optional>
#include <string>

namespace mc {

// 前向声明
class Block;
class Item;

namespace entity {
namespace ai {
namespace goal {
namespace villager {

/**
 * @brief 农民工作目标
 *
 * 农民特有的工作行为：种植、收获、堆肥。
 * 参考 MC 1.21.11 HarvestFarmland + WorkAtComposter 行为实现。
 * 农民在3x3x3范围内搜索成熟作物或空耕地，执行收获、种植和堆肥操作。
 */
class FarmerWorkGoal : public WorkAtJobSiteGoal {
public:
    explicit FarmerWorkGoal(VillagerEntity* villager);
    void tick() override;
    [[nodiscard]] std::string getTypeName() const override { return "FarmerWorkGoal"; }

private:
    /**
     * @brief 尝试收获成熟作物
     *
     * 在村民周围3x3x3区域搜索成熟CropBlock，收获后掉落物放入背包或丢在地上
     */
    void _tryHarvest();

    /**
     * @brief 尝试在空耕地上种植作物
     *
     * 搜索空气+下方耕地的位置，从背包中取种子种植
     */
    void _tryPlant();

    /**
     * @brief 尝试使用堆肥桶堆肥多余种子
     *
     * 查找附近堆肥桶，将多余种子（小麦种子、甜菜种子）堆肥，满桶时取出骨粉
     */
    void _tryCompost();

    /**
     * @brief 检查村民是否有可种植的种子
     *
     * 遍历村民背包，查找小麦种子、胡萝卜、马铃薯、甜菜种子等可种植物品
     */
    [[nodiscard]] bool _hasFarmSeeds() const;

    /**
     * @brief 根据种子物品获取对应的作物方块
     *
     * 通过 VanillaBlocks 静态引用直接映射种子物品到作物方块。
     * 作物方块不在 BlockItemRegistry 中注册（作物不能由物品直接放置，
     * 只能通过种子种植），因此使用 VanillaBlocks 直接引用而非
     * BlockItemRegistry::instance().getBlock(itemId)。
     *
     * @param seedItem 种子物品
     * @return 对应的作物方块指针，未找到返回 nullptr
     */
    [[nodiscard]] static const Block* _getCropBlockForSeed(const Item* seedItem);

    /**
     * @brief 检查指定位置的方块是否是成熟的作物
     */
    [[nodiscard]] bool _isCropMatureAt(const BlockPos& pos) const;

    /**
     * @brief 检查指定位置上方是否可以种植作物（空气+下方耕地）
     */
    [[nodiscard]] bool _canPlantAt(const BlockPos& pos) const;

    /**
     * @brief 判断方块位置是否有效（成熟作物 或 空地+下方耕地）
     *
     * 对应 MC HarvestFarmland.validPos() 的逻辑
     */
    [[nodiscard]] bool _isValidFarmPos(const BlockPos& pos) const;

    /**
     * @brief 从有效农田位置中随机选取一个
     *
     * 使用蓄水池抽样算法在3x3x3范围内随机选取
     */
    [[nodiscard]] std::optional<BlockPos> _pickValidFarmland() const;

    /**
     * @brief 收获指定位置的成熟作物
     *
     * 生成掉落物（放入背包或丢在地上），然后移除作物方块
     */
    void _harvestCrop(const BlockPos& pos);

private:
    i32 m_farmerWorkTicks = 0;
    static constexpr i32 FARMER_WORK_INTERVAL = 20; // 工作间隔
    static constexpr i32 FARMER_SEARCH_RANGE = 1;   // 搜索半径（3x3x3区域）
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
