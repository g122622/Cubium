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

#include "world/block/Block.hpp"

namespace mc {
namespace block_registry {

/**
 * @brief 农作物方块的静态引用
 *
 * 包含种植在耕地上或依附于特定方块的农业相关方块。
 * 农作物方块（小麦、胡萝卜、马铃薯、甜菜根）由 FarmerWorkGoal 在种植/收获逻辑中使用，
 * 并通过 BlockItemRegistry 与对应的种子物品建立映射关系。
 * 可可豆附着在丛林原木侧面生长，甜浆果丛种植在草/泥土上。
 *
 * 注意：甘蔗（SugarCaneBlock）注册在 NaturalBlocks 中（PlantType::Beach，非耕地作物），
 * 南瓜/西瓜茎（StemBlock）注册在 VegetationBlocks 中（茎类作物，非 CropBlock 子类），
 * 下界疣（NetherWartBlock）注册在 NetherBlocks 中（下界植物）。
 * 这些方块不在 AgriculturalBlocks 中，因为它们不属于耕地农业系统。
 */
struct AgriculturalBlocks {
    static Block* WHEAT;     // minecraft:wheat - 小麦作物
    static Block* CARROTS;   // minecraft:carrots - 胡萝卜作物
    static Block* POTATOES;  // minecraft:potatoes - 马铃薯作物
    static Block* BEETROOTS; // minecraft:beetroots - 甜菜根作物
    static Block* COCOA;     // minecraft:cocoa - 可可豆（附着在丛林原木侧面）
};

void registerAgriculturalBlocks();

} // namespace block_registry
} // namespace mc
