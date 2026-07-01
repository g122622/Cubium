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
 * @brief 下界方块（灵魂沙、玄武岩、黑石、下界扩展等）的静态引用
 */
struct NetherBlocks {
    // 下界方块
    static Block* SOUL_SAND;
    static Block* SOUL_SOIL;
    static Block* BASALT;
    static Block* POLISHED_BASALT;
    static Block* BLACKSTONE;
    static Block* POLISHED_BLACKSTONE;
    static Block* CRYING_OBSIDIAN;
    static Block* RESPAWN_ANCHOR;
    static Block* MAGMA;
    static Block* NETHER_WART_BLOCK;
    static Block* WARPED_WART_BLOCK;
    static Block* FIRE;
    static Block* SOUL_FIRE;
    static Block* NETHER_WART;

    // 下界扩展植物方块
    static Block* CRIMSON_STEM;
    static Block* WARPED_STEM;
    static Block* STRIPPED_CRIMSON_STEM;
    static Block* STRIPPED_WARPED_STEM;
    static Block* CRIMSON_HYPHAE;
    static Block* WARPED_HYPHAE;
    static Block* STRIPPED_CRIMSON_HYPHAE;
    static Block* STRIPPED_WARPED_HYPHAE;
    static Block* CRIMSON_NYLIUM;
    static Block* WARPED_NYLIUM;
    static Block* SHROOMLIGHT;
    static Block* CRIMSON_FUNGUS;
    static Block* WARPED_FUNGUS;
    static Block* WEEPING_VINES;
    static Block* TWISTING_VINES;
    static Block* WEEPING_VINES_PLANT;
    static Block* TWISTING_VINES_PLANT;
    static Block* CRIMSON_ROOTS;
    static Block* WARPED_ROOTS;
    static Block* NETHER_SPROUTS;

    // 灵魂火把
    static Block* SOUL_TORCH;
    static Block* SOUL_WALL_TORCH;

    // 黑石建筑方块
    static Block* BLACKSTONE_STAIRS;
    static Block* BLACKSTONE_SLAB;
    static Block* BLACKSTONE_WALL;
    static Block* POLISHED_BLACKSTONE_BRICKS;
    static Block* CRACKED_POLISHED_BLACKSTONE_BRICKS;
    static Block* CHISELED_POLISHED_BLACKSTONE;
    static Block* POLISHED_BLACKSTONE_BRICK_STAIRS;
    static Block* POLISHED_BLACKSTONE_BRICK_SLAB;
    static Block* POLISHED_BLACKSTONE_BRICK_WALL;
    static Block* POLISHED_BLACKSTONE_STAIRS;
    static Block* POLISHED_BLACKSTONE_SLAB;
    static Block* POLISHED_BLACKSTONE_WALL;
    static Block* GILDED_BLACKSTONE;

    // 下界砖扩展
    static Block* CHISELED_NETHER_BRICKS;
    static Block* CRACKED_NETHER_BRICKS;
    static Block* NETHER_BRICK_FENCE;

    // 磁石
    static Block* LODESTONE;

    // 漏斗和钟
    static Block* HOPPER;
    static Block* BELL;

    // 末地方块
    static Block* END_STONE_BRICKS;
    static Block* END_ROD;
    static Block* CHORUS_PLANT;
    static Block* CHORUS_FLOWER;
    static Block* DRAGON_EGG;

    // 末地传送门系列
    static Block* NETHER_PORTAL;
    static Block* END_PORTAL;
    static Block* END_PORTAL_FRAME;
    static Block* END_GATEWAY;
    static Block* BEACON;
    static Block* BREWING_STAND;
    static Block* ENDER_CHEST;
    static Block* LANTERN;
    static Block* SOUL_LANTERN;
    static Block* CAMPFIRE;
    static Block* SOUL_CAMPFIRE;
    static Block* JACK_O_LANTERN;
};

void registerNetherBlocks();

} // namespace block_registry
} // namespace mc
