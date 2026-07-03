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
 * @brief 花盆方块的静态引用
 *
 * 包含1个空花盆（minecraft:flower_pot）和37个盆栽方块（minecraft:potted_*）。
 *
 * 空花盆可以容纳一种植物内容物。玩家右键空花盆并手持可盆栽植物时，
 * 花盆会变为对应的 potted_* 方块；空手右键已盆栽的花盆可取出内容物。
 *
 * 注册顺序：必须在所有内容物方块（花卉、树苗、蘑菇、仙人掌、下界菌等）注册之后调用。
 */
struct FlowerPotBlocks {
    /// 空花盆（minecraft:flower_pot）
    static Block* FLOWER_POT;

    // ========== 树苗系列 ==========
    static Block* POTTED_OAK_SAPLING;
    static Block* POTTED_SPRUCE_SAPLING;
    static Block* POTTED_BIRCH_SAPLING;
    static Block* POTTED_JUNGLE_SAPLING;
    static Block* POTTED_ACACIA_SAPLING;
    static Block* POTTED_DARK_OAK_SAPLING;
    static Block* POTTED_CHERRY_SAPLING;
    static Block* POTTED_PALE_OAK_SAPLING;
    static Block* POTTED_MANGROVE_PROPAGULE;

    // ========== 花卉系列 ==========
    static Block* POTTED_DANDELION;
    static Block* POTTED_POPPY;
    static Block* POTTED_BLUE_ORCHID;
    static Block* POTTED_ALLIUM;
    static Block* POTTED_AZURE_BLUET;
    static Block* POTTED_RED_TULIP;
    static Block* POTTED_ORANGE_TULIP;
    static Block* POTTED_WHITE_TULIP;
    static Block* POTTED_PINK_TULIP;
    static Block* POTTED_OXEYE_DAISY;
    static Block* POTTED_CORNFLOWER;
    static Block* POTTED_LILY_OF_THE_VALLEY;
    static Block* POTTED_WITHER_ROSE;
    static Block* POTTED_TORCHFLOWER;
    static Block* POTTED_OPEN_EYEBLOSSOM;
    static Block* POTTED_CLOSED_EYEBLOSSOM;

    // ========== 蕨/枯草 ==========
    static Block* POTTED_FERN;
    static Block* POTTED_DEAD_BUSH;

    // ========== 蘑菇 ==========
    static Block* POTTED_RED_MUSHROOM;
    static Block* POTTED_BROWN_MUSHROOM;

    // ========== 仙人掌/竹子 ==========
    static Block* POTTED_CACTUS;
    static Block* POTTED_BAMBOO;

    // ========== 下界菌/菌索 ==========
    static Block* POTTED_CRIMSON_FUNGUS;
    static Block* POTTED_WARPED_FUNGUS;
    static Block* POTTED_CRIMSON_ROOTS;
    static Block* POTTED_WARPED_ROOTS;

    // ========== 杜鹃花 ==========
    static Block* POTTED_AZALEA_BUSH;
    static Block* POTTED_FLOWERING_AZALEA_BUSH;
};

void registerFlowerPotBlocks();

} // namespace block_registry
} // namespace mc
