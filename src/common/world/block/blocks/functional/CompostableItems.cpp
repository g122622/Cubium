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

#include "CompostableItems.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// 静态成员初始化
std::unordered_map<const Item*, float> CompostableItems::s_chances;
bool CompostableItems::s_initialized = false;

// ============================================================================
// 公开接口
// ============================================================================

void CompostableItems::initialize()
{
    if (s_initialized) {
        return;
    }

    // 注册各概率等级的物品
    registerChance30();
    registerChance50();
    registerChance65();
    registerChance85();
    registerChance100();

    s_initialized = true;
}

float CompostableItems::getCompostChance(const Item* item)
{
    if (item == nullptr) {
        return 0.0f;
    }

    auto it = s_chances.find(item);
    if (it != s_chances.end()) {
        return it->second;
    }
    return 0.0f;
}

bool CompostableItems::isCompostable(const Item* item)
{
    return item != nullptr && s_chances.find(item) != s_chances.end();
}

// ============================================================================
// 私有方法
// ============================================================================

void CompostableItems::registerCompostable(const Item* item, float chance)
{
    if (item != nullptr) {
        s_chances[item] = chance;
    }
}

// ============================================================================
// 30% 概率物品
// 参考 MC 1.16.5 ComposterBlock.registerCompostable(0.3F, ...)
// ============================================================================
void CompostableItems::registerChance30()
{
    // 树叶 (所有6种)
    registerCompostable(Items::OAK_LEAVES, 0.3f);
    registerCompostable(Items::SPRUCE_LEAVES, 0.3f);
    registerCompostable(Items::BIRCH_LEAVES, 0.3f);
    registerCompostable(Items::JUNGLE_LEAVES, 0.3f);
    registerCompostable(Items::ACACIA_LEAVES, 0.3f);
    registerCompostable(Items::DARK_OAK_LEAVES, 0.3f);

    // 树苗 (所有6种)
    registerCompostable(Items::OAK_SAPLING, 0.3f);
    registerCompostable(Items::SPRUCE_SAPLING, 0.3f);
    registerCompostable(Items::BIRCH_SAPLING, 0.3f);
    registerCompostable(Items::JUNGLE_SAPLING, 0.3f);
    registerCompostable(Items::ACACIA_SAPLING, 0.3f);
    registerCompostable(Items::DARK_OAK_SAPLING, 0.3f);

    // 种子
    registerCompostable(Items::WHEAT_SEEDS, 0.3f);
    registerCompostable(Items::PUMPKIN_SEEDS, 0.3f);
    registerCompostable(Items::MELON_SEEDS, 0.3f);
    registerCompostable(Items::BEETROOT_SEEDS, 0.3f);

    // 海洋植物
    registerCompostable(Items::DRIED_KELP, 0.3f);
    registerCompostable(Items::KELP, 0.3f);
    registerCompostable(Items::SEAGRASS, 0.3f);

    // 草 (MC 1.16.5 中 Items.GRASS 对应 SHORT_GRASS)
    registerCompostable(Items::SHORT_GRASS, 0.3f);

    // 甜浆果
    registerCompostable(Items::SWEET_BERRIES, 0.3f);

    // TODO: 下界苗 NETHER_SPROUTS (物品暂未注册)
}

// ============================================================================
// 50% 概率物品
// 参考 MC 1.16.5 ComposterBlock.registerCompostable(0.5F, ...)
// ============================================================================
void CompostableItems::registerChance50()
{
    // 干海带块
    registerCompostable(Items::DRIED_KELP_BLOCK, 0.5f);

    // 高草
    registerCompostable(Items::TALL_GRASS, 0.5f);

    // 仙人掌
    registerCompostable(Items::CACTUS, 0.5f);

    // 甘蔗
    registerCompostable(Items::SUGAR_CANE, 0.5f);

    // 藤蔓
    registerCompostable(Items::VINE, 0.5f);

    // 西瓜片
    registerCompostable(Items::MELON_SLICE, 0.5f);

    // 下界植物
    registerCompostable(Items::WEEPING_VINES, 0.5f);  // 垂泪藤
    registerCompostable(Items::TWISTING_VINES, 0.5f); // 缠怨藤

    // TODO: NETHER_SPROUTS 下界苗 (物品暂未注册)
}

// ============================================================================
// 65% 概率物品
// 参考 MC 1.16.5 ComposterBlock.registerCompostable(0.65F, ...)
// ============================================================================
void CompostableItems::registerChance65()
{
    // 海泡菜
    registerCompostable(Items::SEA_PICKLE, 0.65f);

    // 睡莲
    registerCompostable(Items::LILY_PAD, 0.65f);

    // 南瓜和雕刻南瓜
    registerCompostable(Items::PUMPKIN, 0.65f);
    registerCompostable(Items::CARVED_PUMPKIN, 0.65f);

    // 西瓜块
    registerCompostable(Items::MELON, 0.65f);

    // 食物类
    registerCompostable(Items::APPLE, 0.65f);
    registerCompostable(Items::BEETROOT, 0.65f);
    registerCompostable(Items::CARROT, 0.65f);
    registerCompostable(Items::COCOA_BEANS, 0.65f);
    registerCompostable(Items::POTATO, 0.65f);
    registerCompostable(Items::WHEAT, 0.65f);

    // 蘑菇
    registerCompostable(Items::BROWN_MUSHROOM, 0.65f);
    registerCompostable(Items::RED_MUSHROOM, 0.65f);
    registerCompostable(Items::MUSHROOM_STEM, 0.65f);

    // 下界菌类
    registerCompostable(Items::CRIMSON_FUNGUS, 0.65f); // 绯红菌
    registerCompostable(Items::WARPED_FUNGUS, 0.65f);  // 诡异菌

    // 下界疣
    registerCompostable(Items::NETHER_WART, 0.65f);

    // 发酵蜘蛛眼
    registerCompostable(Items::FERMENTED_SPIDER_EYE, 0.65f);

    // 荧光菇
    registerCompostable(Items::SHROOMLIGHT, 0.65f);

    // 小型花朵
    registerCompostable(Items::DANDELION, 0.65f);          // 蒲公英
    registerCompostable(Items::POPPY, 0.65f);              // 虞美人
    registerCompostable(Items::BLUE_ORCHID, 0.65f);        // 蓝花美耳草
    registerCompostable(Items::ALLIUM, 0.65f);             // 绒球葱
    registerCompostable(Items::AZURE_BLUET, 0.65f);        // 蓝花美耳草
    registerCompostable(Items::RED_TULIP, 0.65f);          // 红色郁金香
    registerCompostable(Items::ORANGE_TULIP, 0.65f);       // 橙色郁金香
    registerCompostable(Items::WHITE_TULIP, 0.65f);        // 白色郁金香
    registerCompostable(Items::PINK_TULIP, 0.65f);         // 粉色郁金香
    registerCompostable(Items::OXEYE_DAISY, 0.65f);        // 滨菊
    registerCompostable(Items::CORNFLOWER, 0.65f);         // 矢车菊
    registerCompostable(Items::LILY_OF_THE_VALLEY, 0.65f); // 铃兰
    registerCompostable(Items::WITHER_ROSE, 0.65f);        // 凋零玫瑰

    // 蕨
    registerCompostable(Items::FERN, 0.65f);

    // 大型花朵
    registerCompostable(Items::SUNFLOWER, 0.65f); // 向日葵
    registerCompostable(Items::LILAC, 0.65f);     // 紫丁香
    registerCompostable(Items::ROSE_BUSH, 0.65f); // 玫瑰丛
    registerCompostable(Items::PEONY, 0.65f);     // 牡丹

    // TODO: LARGE_FERN 大型蕨 (物品暂未注册)
    // TODO: CRIMSON_ROOTS 绯红菌索 (物品暂未注册)
    // TODO: WARPED_ROOTS 诡异菌索 (物品暂未注册)
}

// ============================================================================
// 85% 概率物品
// 参考 MC 1.16.5 ComposterBlock.registerCompostable(0.85F, ...)
// ============================================================================
void CompostableItems::registerChance85()
{
    // 干草块
    registerCompostable(Items::HAY_BLOCK, 0.85f);

    // 蘑菇方块
    registerCompostable(Items::BROWN_MUSHROOM_BLOCK, 0.85f);
    registerCompostable(Items::RED_MUSHROOM_BLOCK, 0.85f);

    // 下界疣块
    registerCompostable(Items::NETHER_WART_BLOCK, 0.85f);
    registerCompostable(Items::WARPED_WART_BLOCK, 0.85f);

    // 食物类
    registerCompostable(Items::BREAD, 0.85f);
    registerCompostable(Items::BAKED_POTATO, 0.85f);
    registerCompostable(Items::COOKIE, 0.85f);
}

// ============================================================================
// 100% 概率物品
// 参考 MC 1.16.5 ComposterBlock.registerCompostable(1.0F, ...)
// ============================================================================
void CompostableItems::registerChance100()
{
    // 南瓜派
    registerCompostable(Items::PUMPKIN_PIE, 1.0f);

    // TODO: CAKE 蛋糕 (物品暂未注册)
}

} // namespace blocks
} // namespace mc
