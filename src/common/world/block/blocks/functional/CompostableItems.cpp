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
    // 树叶 (所有种类)
    // 注: 树叶物品需要从方块获取，暂时跳过，等待树叶方块物品注册

    // 树苗 (所有种类) - 从 Items 获取
    registerCompostable(Items::WHEAT_SEEDS, 0.3f);    // 小麦种子
    registerCompostable(Items::PUMPKIN_SEEDS, 0.3f);  // 南瓜种子
    registerCompostable(Items::MELON_SEEDS, 0.3f);    // 西瓜种子
    registerCompostable(Items::BEETROOT_SEEDS, 0.3f); // 甜菜种子

    // 干海带 - 食物
    registerCompostable(Items::DRIED_KELP, 0.3f);

    // 甜浆果
    registerCompostable(Items::SWEET_BERRIES, 0.3f);

    // 注: 海草、海带、草、高草等是方块物品，需要从 VanillaBlocks 获取
    // 这里暂时只注册已存在的物品
}

// ============================================================================
// 50% 概率物品
// 参考 MC 1.16.5 ComposterBlock.registerCompostable(0.5F, ...)
// ============================================================================
void CompostableItems::registerChance50()
{
    // 干海带块 - 需要从方块获取
    // registerCompostable(Items::DRIED_KELP_BLOCK, 0.5f);

    // 仙人掌 - 方块物品
    // 甘蔗 - 方块物品
    // 藤蔓 - 方块物品

    // 西瓜片
    registerCompostable(Items::MELON_SLICE, 0.5f);

    // 注: 下界苗、垂泪藤、缠绕藤是下界方块物品，暂未注册
}

// ============================================================================
// 65% 概率物品
// 参考 MC 1.16.5 ComposterBlock.registerCompostable(0.65F, ...)
// ============================================================================
void CompostableItems::registerChance65()
{
    // 苹果
    registerCompostable(Items::APPLE, 0.65f);

    // 甜菜根
    registerCompostable(Items::BEETROOT, 0.65f);

    // 胡萝卜
    registerCompostable(Items::CARROT, 0.65f);

    // 可可豆 (染料)
    registerCompostable(Items::COCOA_BEANS, 0.65f);

    // 马铃薯
    registerCompostable(Items::POTATO, 0.65f);

    // 小麦
    registerCompostable(Items::WHEAT, 0.65f);

    // 南瓜 - 方块物品
    registerCompostable(Items::PUMPKIN, 0.65f);

    // 西瓜 - 方块物品
    registerCompostable(Items::MELON, 0.65f);

    // 地狱疣
    registerCompostable(Items::NETHER_WART, 0.65f);

    // 发酵蜘蛛眼
    registerCompostable(Items::FERMENTED_SPIDER_EYE, 0.65f);

    // 蘑菇相关 (红蘑菇、棕蘑菇是方块物品)
    // 花朵 (花朵是方块物品)
    // 睡莲 (方块物品)
    // 海泡菜 (方块物品)
    // 蕨、向日葵、丁香、玫瑰丛、牡丹、大型蕨 (方块物品)
}

// ============================================================================
// 85% 概率物品
// 参考 MC 1.16.5 ComposterBlock.registerCompostable(0.85F, ...)
// ============================================================================
void CompostableItems::registerChance85()
{
    // 面包
    registerCompostable(Items::BREAD, 0.85f);

    // 曲奇
    registerCompostable(Items::COOKIE, 0.85f);

    // 烤马铃薯
    registerCompostable(Items::BAKED_POTATO, 0.85f);

    // 干草块 - 方块物品
    // registerCompostable(Items::HAY_BLOCK, 0.85f);

    // 蘑菇块 (方块物品)
    // 下界疣块、诡异疣块 (方块物品)
}

// ============================================================================
// 100% 概率物品
// 参考 MC 1.16.5 ComposterBlock.registerCompostable(1.0F, ...)
// ============================================================================
void CompostableItems::registerChance100()
{
    // 蛋糕 - 暂未注册
    // registerCompostable(Items::CAKE, 1.0f);

    // 南瓜派
    registerCompostable(Items::PUMPKIN_PIE, 1.0f);
}

} // namespace blocks
} // namespace mc
