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
 */

#pragma once

#include "world/block/Block.hpp"

namespace mc {
namespace block_registry {

/**
 * @brief 铜方块（1.17 洞穴与山崖 + 1.21 扩展）的静态引用
 *
 * 包含铜块及其氧化变种、切制铜系列、门、活板门、格栅、灯、凿制铜等。
 */
struct CopperBlocks {
    // ========== 铜块系列（4个氧化等级 + 4个涂蜡变种 = 8个）==========
    // 未氧化铜块
    static Block* COPPER_BLOCK;
    static Block* EXPOSED_COPPER;
    static Block* WEATHERED_COPPER;
    static Block* OXIDIZED_COPPER;
    // 涂蜡铜块
    static Block* WAXED_COPPER_BLOCK;
    static Block* WAXED_EXPOSED_COPPER;
    static Block* WAXED_WEATHERED_COPPER;
    static Block* WAXED_OXIDIZED_COPPER;

    // ========== 切制铜系列（8个）==========
    static Block* CUT_COPPER;
    static Block* EXPOSED_CUT_COPPER;
    static Block* WEATHERED_CUT_COPPER;
    static Block* OXIDIZED_CUT_COPPER;
    static Block* WAXED_CUT_COPPER;
    static Block* WAXED_EXPOSED_CUT_COPPER;
    static Block* WAXED_WEATHERED_CUT_COPPER;
    static Block* WAXED_OXIDIZED_CUT_COPPER;

    // ========== 切制铜楼梯（8个）==========
    static Block* CUT_COPPER_STAIRS;
    static Block* EXPOSED_CUT_COPPER_STAIRS;
    static Block* WEATHERED_CUT_COPPER_STAIRS;
    static Block* OXIDIZED_CUT_COPPER_STAIRS;
    static Block* WAXED_CUT_COPPER_STAIRS;
    static Block* WAXED_EXPOSED_CUT_COPPER_STAIRS;
    static Block* WAXED_WEATHERED_CUT_COPPER_STAIRS;
    static Block* WAXED_OXIDIZED_CUT_COPPER_STAIRS;

    // ========== 切制铜台阶（8个）==========
    static Block* CUT_COPPER_SLAB;
    static Block* EXPOSED_CUT_COPPER_SLAB;
    static Block* WEATHERED_CUT_COPPER_SLAB;
    static Block* OXIDIZED_CUT_COPPER_SLAB;
    static Block* WAXED_CUT_COPPER_SLAB;
    static Block* WAXED_EXPOSED_CUT_COPPER_SLAB;
    static Block* WAXED_WEATHERED_CUT_COPPER_SLAB;
    static Block* WAXED_OXIDIZED_CUT_COPPER_SLAB;

    // ========== 1.21 铜扩展：铜门（8个）==========
    static Block* COPPER_DOOR;
    static Block* EXPOSED_COPPER_DOOR;
    static Block* WEATHERED_COPPER_DOOR;
    static Block* OXIDIZED_COPPER_DOOR;
    static Block* WAXED_COPPER_DOOR;
    static Block* WAXED_EXPOSED_COPPER_DOOR;
    static Block* WAXED_WEATHERED_COPPER_DOOR;
    static Block* WAXED_OXIDIZED_COPPER_DOOR;

    // ========== 1.21 铜扩展：铜活板门（8个）==========
    static Block* COPPER_TRAPDOOR;
    static Block* EXPOSED_COPPER_TRAPDOOR;
    static Block* WEATHERED_COPPER_TRAPDOOR;
    static Block* OXIDIZED_COPPER_TRAPDOOR;
    static Block* WAXED_COPPER_TRAPDOOR;
    static Block* WAXED_EXPOSED_COPPER_TRAPDOOR;
    static Block* WAXED_WEATHERED_COPPER_TRAPDOOR;
    static Block* WAXED_OXIDIZED_COPPER_TRAPDOOR;

    // ========== 1.21 铜扩展：铜格栅（8个）==========
    static Block* COPPER_GRATE;
    static Block* EXPOSED_COPPER_GRATE;
    static Block* WEATHERED_COPPER_GRATE;
    static Block* OXIDIZED_COPPER_GRATE;
    static Block* WAXED_COPPER_GRATE;
    static Block* WAXED_EXPOSED_COPPER_GRATE;
    static Block* WAXED_WEATHERED_COPPER_GRATE;
    static Block* WAXED_OXIDIZED_COPPER_GRATE;

    // ========== 1.21 铜扩展：铜灯（8个）==========
    static Block* COPPER_BULB;
    static Block* EXPOSED_COPPER_BULB;
    static Block* WEATHERED_COPPER_BULB;
    static Block* OXIDIZED_COPPER_BULB;
    static Block* WAXED_COPPER_BULB;
    static Block* WAXED_EXPOSED_COPPER_BULB;
    static Block* WAXED_WEATHERED_COPPER_BULB;
    static Block* WAXED_OXIDIZED_COPPER_BULB;

    // ========== 1.21 铜扩展：凿制铜（8个）==========
    static Block* CHISELED_COPPER;
    static Block* EXPOSED_CHISELED_COPPER;
    static Block* WEATHERED_CHISELED_COPPER;
    static Block* OXIDIZED_CHISELED_COPPER;
    static Block* WAXED_CHISELED_COPPER;
    static Block* WAXED_EXPOSED_CHISELED_COPPER;
    static Block* WAXED_WEATHERED_CHISELED_COPPER;
    static Block* WAXED_OXIDIZED_CHISELED_COPPER;

    // ========== 1.21 铜扩展：铜栏杆（8个）==========
    static Block* COPPER_BARS;
    static Block* EXPOSED_COPPER_BARS;
    static Block* WEATHERED_COPPER_BARS;
    static Block* OXIDIZED_COPPER_BARS;
    static Block* WAXED_COPPER_BARS;
    static Block* WAXED_EXPOSED_COPPER_BARS;
    static Block* WAXED_WEATHERED_COPPER_BARS;
    static Block* WAXED_OXIDIZED_COPPER_BARS;

    // ========== 1.21 铜扩展：铜链（8个）==========
    static Block* COPPER_CHAIN;
    static Block* EXPOSED_COPPER_CHAIN;
    static Block* WEATHERED_COPPER_CHAIN;
    static Block* OXIDIZED_COPPER_CHAIN;
    static Block* WAXED_COPPER_CHAIN;
    static Block* WAXED_EXPOSED_COPPER_CHAIN;
    static Block* WAXED_WEATHERED_COPPER_CHAIN;
    static Block* WAXED_OXIDIZED_COPPER_CHAIN;

    // ========== 1.21 铜扩展：铜灯笼（8个）==========
    static Block* COPPER_LANTERN;
    static Block* EXPOSED_COPPER_LANTERN;
    static Block* WEATHERED_COPPER_LANTERN;
    static Block* OXIDIZED_COPPER_LANTERN;
    static Block* WAXED_COPPER_LANTERN;
    static Block* WAXED_EXPOSED_COPPER_LANTERN;
    static Block* WAXED_WEATHERED_COPPER_LANTERN;
    static Block* WAXED_OXIDIZED_COPPER_LANTERN;

    // ========== 避雷针（1.17 基础 + 1.21 铜扩展氧化变种）==========
    // 未氧化避雷针（基础版，不参与氧化）
    static Block* LIGHTNING_ROD;
    // 可氧化避雷针变种（1.21+）
    static Block* EXPOSED_LIGHTNING_ROD;
    static Block* WEATHERED_LIGHTNING_ROD;
    static Block* OXIDIZED_LIGHTNING_ROD;
    // 涂蜡避雷针变种（1.21+）
    static Block* WAXED_LIGHTNING_ROD;
    static Block* WAXED_EXPOSED_LIGHTNING_ROD;
    static Block* WAXED_WEATHERED_LIGHTNING_ROD;
    static Block* WAXED_OXIDIZED_LIGHTNING_ROD;

    // ========== 粗矿块（1.17）==========
    static Block* RAW_IRON_BLOCK;
    static Block* RAW_COPPER_BLOCK;
    static Block* RAW_GOLD_BLOCK;
};

void registerCopperBlocks();

} // namespace block_registry
} // namespace mc
