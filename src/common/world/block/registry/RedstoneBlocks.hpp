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
 * @brief 红石方块、铁轨方块的静态引用
 */
struct RedstoneBlocks {
    // 红石方块
    static Block* REDSTONE_WIRE;
    static Block* REDSTONE_TORCH;
    static Block* REDSTONE_WALL_TORCH;
    static Block* REDSTONE_LAMP;
    static Block* REDSTONE_REPEATER;
    static Block* REDSTONE_COMPARATOR;
    static Block* OBSERVER;
    static Block* LEVER;
    static Block* STONE_BUTTON;
    static Block* OAK_BUTTON;
    static Block* SPRUCE_BUTTON;
    static Block* BIRCH_BUTTON;
    static Block* JUNGLE_BUTTON;
    static Block* ACACIA_BUTTON;
    static Block* DARK_OAK_BUTTON;
    static Block* CRIMSON_BUTTON;
    static Block* WARPED_BUTTON;
    static Block* STONE_PRESSURE_PLATE;
    static Block* OAK_PRESSURE_PLATE;
    static Block* LIGHT_WEIGHTED_PRESSURE_PLATE;
    static Block* HEAVY_WEIGHTED_PRESSURE_PLATE;
    static Block* DAYLIGHT_DETECTOR;
    static Block* PISTON;
    static Block* STICKY_PISTON;
    static Block* PISTON_HEAD;
    static Block* MOVING_PISTON;
    static Block* DISPENSER;
    static Block* DROPPER;
    static Block* NOTE_BLOCK;
    static Block* TRIPWIRE;
    static Block* TRIPWIRE_HOOK;
    static Block* TARGET;

    // 铁轨方块
    static Block* RAIL;
    static Block* POWERED_RAIL;
    static Block* DETECTOR_RAIL;
    static Block* ACTIVATOR_RAIL;
};

void registerRedstoneBlocks();

} // namespace block_registry
} // namespace mc
