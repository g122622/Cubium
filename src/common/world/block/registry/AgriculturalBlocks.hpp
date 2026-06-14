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
 * @brief 农作物方块（小麦、胡萝卜、马铃薯、甜菜根）的静态引用
 * TODO 看看有没有其他农作物方块需要注册（如：甘蔗、南瓜茎、藤蔓等），如果有则添加到这里
 *
 * 这些作物方块由 FarmerWorkGoal 在种植/收获逻辑中使用，
 * 并通过 BlockItemRegistry 与对应的种子物品建立映射关系。
 */
struct AgriculturalBlocks {
    static Block* WHEAT;     // minecraft:wheat - 小麦作物
    static Block* CARROTS;   // minecraft:carrots - 胡萝卜作物
    static Block* POTATOES;  // minecraft:potatoes - 马铃薯作物
    static Block* BEETROOTS; // minecraft:beetroots - 甜菜根作物
};

void registerAgriculturalBlocks();

} // namespace block_registry
} // namespace mc
