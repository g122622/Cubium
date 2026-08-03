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
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

// 静态成员初始化
std::unordered_map<const Item*, f32> CompostableItems::s_chances;
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
    _registerChance30();
    _registerChance50();
    _registerChance65();
    _registerChance85();
    _registerChance100();

    s_initialized = true;
}

f32 CompostableItems::getCompostChance(const Item* item)
{
    // 与 isCompostable 一致：空指针视为不可堆肥，返回 0.0f。
    // 原 MC_ASSERT_RELEASE(item != nullptr) 会让 NullItemHandling 测试触发
    // 断言并中止整个 mc_tests 套件。MC 1.21.11 中 COMPOSTABLES.getFloat(item)
    // 对未注册/空物品返回 -1.0F（默认值），本项目的约定是返回 0.0f 表示
    // 不可堆肥（见下方 s_chances.find 未命中分支），此处保持一致。
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

void CompostableItems::_registerCompostable(const Item* item, f32 chance)
{
    if (item != nullptr) {
        s_chances[item] = chance;
    }
}

// ============================================================================
// 30% 概率物品
// ============================================================================
void CompostableItems::_registerChance30()
{
    // 树叶 (所有6种)
    _registerCompostable(Items::OAK_LEAVES, 0.3f);
    _registerCompostable(Items::SPRUCE_LEAVES, 0.3f);
    _registerCompostable(Items::BIRCH_LEAVES, 0.3f);
    _registerCompostable(Items::JUNGLE_LEAVES, 0.3f);
    _registerCompostable(Items::ACACIA_LEAVES, 0.3f);
    _registerCompostable(Items::DARK_OAK_LEAVES, 0.3f);

    // 树苗 (所有6种)
    _registerCompostable(Items::OAK_SAPLING, 0.3f);
    _registerCompostable(Items::SPRUCE_SAPLING, 0.3f);
    _registerCompostable(Items::BIRCH_SAPLING, 0.3f);
    _registerCompostable(Items::JUNGLE_SAPLING, 0.3f);
    _registerCompostable(Items::ACACIA_SAPLING, 0.3f);
    _registerCompostable(Items::DARK_OAK_SAPLING, 0.3f);

    // 种子
    _registerCompostable(Items::WHEAT_SEEDS, 0.3f);
    _registerCompostable(Items::PUMPKIN_SEEDS, 0.3f);
    _registerCompostable(Items::MELON_SEEDS, 0.3f);
    _registerCompostable(Items::BEETROOT_SEEDS, 0.3f);
    _registerCompostable(Items::TORCHFLOWER_SEEDS, 0.3f);
    _registerCompostable(Items::PITCHER_POD, 0.3f);

    // 海洋植物
    _registerCompostable(Items::DRIED_KELP, 0.3f);
    _registerCompostable(Items::KELP, 0.3f);
    _registerCompostable(Items::SEAGRASS, 0.3f);

    // 草
    _registerCompostable(Items::SHORT_GRASS, 0.3f);

    // 甜浆果
    _registerCompostable(Items::SWEET_BERRIES, 0.3f);
}

// ============================================================================
// 50% 概率物品
// ============================================================================
void CompostableItems::_registerChance50()
{
    // 干海带块
    _registerCompostable(Items::DRIED_KELP_BLOCK, 0.5f);

    // 高草
    _registerCompostable(Items::TALL_GRASS, 0.5f);

    // 仙人掌
    _registerCompostable(Items::CACTUS, 0.5f);

    // 甘蔗
    _registerCompostable(Items::SUGAR_CANE, 0.5f);

    // 藤蔓
    _registerCompostable(Items::VINE, 0.5f);

    // 西瓜片
    _registerCompostable(Items::MELON_SLICE, 0.5f);

    // 下界植物
    _registerCompostable(Items::WEEPING_VINES, 0.5f);
    _registerCompostable(Items::TWISTING_VINES, 0.5f);

    // 下界苗
    _registerCompostable(Items::NETHER_SPROUTS, 0.5f);
}

// ============================================================================
// 65% 概率物品
// ============================================================================
void CompostableItems::_registerChance65()
{
    // 海泡菜
    _registerCompostable(Items::SEA_PICKLE, 0.65f);

    // 睡莲
    _registerCompostable(Items::LILY_PAD, 0.65f);

    // 南瓜和雕刻南瓜
    _registerCompostable(Items::PUMPKIN, 0.65f);
    _registerCompostable(Items::CARVED_PUMPKIN, 0.65f);

    // 西瓜块
    _registerCompostable(Items::MELON, 0.65f);

    // 食物类
    _registerCompostable(Items::APPLE, 0.65f);
    _registerCompostable(Items::BEETROOT, 0.65f);
    _registerCompostable(Items::CARROT, 0.65f);
    _registerCompostable(Items::COCOA_BEANS, 0.65f);
    _registerCompostable(Items::POTATO, 0.65f);
    _registerCompostable(Items::WHEAT, 0.65f);

    // 蘑菇
    _registerCompostable(Items::BROWN_MUSHROOM, 0.65f);
    _registerCompostable(Items::RED_MUSHROOM, 0.65f);
    _registerCompostable(Items::MUSHROOM_STEM, 0.65f);

    // 下界菌类
    _registerCompostable(Items::CRIMSON_FUNGUS, 0.65f);
    _registerCompostable(Items::WARPED_FUNGUS, 0.65f);

    // 下界疣
    _registerCompostable(Items::NETHER_WART, 0.65f);

    // 发酵蜘蛛眼
    _registerCompostable(Items::FERMENTED_SPIDER_EYE, 0.65f);

    // 荧光菇
    _registerCompostable(Items::SHROOMLIGHT, 0.65f);

    // 小型花朵
    _registerCompostable(Items::DANDELION, 0.65f);
    _registerCompostable(Items::POPPY, 0.65f);
    _registerCompostable(Items::BLUE_ORCHID, 0.65f);
    _registerCompostable(Items::ALLIUM, 0.65f);
    _registerCompostable(Items::AZURE_BLUET, 0.65f);
    _registerCompostable(Items::RED_TULIP, 0.65f);
    _registerCompostable(Items::ORANGE_TULIP, 0.65f);
    _registerCompostable(Items::WHITE_TULIP, 0.65f);
    _registerCompostable(Items::PINK_TULIP, 0.65f);
    _registerCompostable(Items::OXEYE_DAISY, 0.65f);
    _registerCompostable(Items::CORNFLOWER, 0.65f);
    _registerCompostable(Items::LILY_OF_THE_VALLEY, 0.65f);
    _registerCompostable(Items::WITHER_ROSE, 0.65f);
    _registerCompostable(Items::TORCHFLOWER, 0.65f);

    // 大型花朵
    _registerCompostable(Items::SUNFLOWER, 0.65f);
    _registerCompostable(Items::LILAC, 0.65f);
    _registerCompostable(Items::ROSE_BUSH, 0.65f);
    _registerCompostable(Items::PEONY, 0.65f);
    _registerCompostable(Items::PITCHER_PLANT, 0.65f);

    // 蕨
    _registerCompostable(Items::FERN, 0.65f);

    // 大型蕨
    _registerCompostable(Items::LARGE_FERN, 0.65f);

    // 下界菌索
    _registerCompostable(Items::CRIMSON_ROOTS, 0.65f);
    _registerCompostable(Items::WARPED_ROOTS, 0.65f);
}

// ============================================================================
// 85% 概率物品
// ============================================================================
void CompostableItems::_registerChance85()
{
    // 干草块
    _registerCompostable(Items::HAY_BLOCK, 0.85f);

    // 蘑菇方块
    _registerCompostable(Items::BROWN_MUSHROOM_BLOCK, 0.85f);
    _registerCompostable(Items::RED_MUSHROOM_BLOCK, 0.85f);

    // 下界疣块
    _registerCompostable(Items::NETHER_WART_BLOCK, 0.85f);
    _registerCompostable(Items::WARPED_WART_BLOCK, 0.85f);

    // 食物类
    _registerCompostable(Items::BREAD, 0.85f);
    _registerCompostable(Items::BAKED_POTATO, 0.85f);
    _registerCompostable(Items::COOKIE, 0.85f);
}

// ============================================================================
// 100% 概率物品
// ============================================================================
void CompostableItems::_registerChance100()
{
    // 南瓜派
    _registerCompostable(Items::PUMPKIN_PIE, 1.0f);

    // 蛋糕
    _registerCompostable(Items::CAKE, 1.0f);
}

} // namespace blocks
} // namespace mc
