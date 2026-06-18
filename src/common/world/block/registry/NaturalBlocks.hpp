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
 * @brief 自然方块（冰、粘液、珊瑚、海洋、仙人掌等）的静态引用
 */
struct NaturalBlocks {
    // 自然方块扩展
    static Block* CLAY;
    static Block* MYCELIUM;
    static Block* GRASS_PATH;
    static Block* PACKED_ICE;
    static Block* BLUE_ICE;
    static Block* FROSTED_ICE;
    static Block* SLIME_BLOCK;
    static Block* HONEY_BLOCK;
    static Block* CACTUS;
    static Block* DEAD_BUSH;
    static Block* LILY_PAD;
    static Block* VINE;
    static Block* COBWEB;
    static Block* SUGAR_CANE;
    static Block* FARMLAND;
    static Block* RED_SAND;
    static Block* DRIED_KELP_BLOCK;
    static Block* SEA_PICKLE;
    static Block* KELP;
    static Block* KELP_PLANT;
    static Block* SEAGRASS;
    static Block* TALL_SEAGRASS;
    static Block* BUBBLE_COLUMN;
    static Block* TURTLE_EGG;

    // 珊瑚方块
    static Block* DEAD_TUBE_CORAL_BLOCK;
    static Block* DEAD_BRAIN_CORAL_BLOCK;
    static Block* DEAD_BUBBLE_CORAL_BLOCK;
    static Block* DEAD_FIRE_CORAL_BLOCK;
    static Block* DEAD_HORN_CORAL_BLOCK;

    static Block* DEAD_TUBE_CORAL_FAN;
    static Block* DEAD_BRAIN_CORAL_FAN;
    static Block* DEAD_BUBBLE_CORAL_FAN;
    static Block* DEAD_FIRE_CORAL_FAN;
    static Block* DEAD_HORN_CORAL_FAN;

    static Block* DEAD_TUBE_CORAL_WALL_FAN;
    static Block* DEAD_BRAIN_CORAL_WALL_FAN;
    static Block* DEAD_BUBBLE_CORAL_WALL_FAN;
    static Block* DEAD_FIRE_CORAL_WALL_FAN;
    static Block* DEAD_HORN_CORAL_WALL_FAN;

    static Block* TUBE_CORAL_BLOCK;
    static Block* BRAIN_CORAL_BLOCK;
    static Block* BUBBLE_CORAL_BLOCK;
    static Block* FIRE_CORAL_BLOCK;
    static Block* HORN_CORAL_BLOCK;

    static Block* TUBE_CORAL_FAN;
    static Block* BRAIN_CORAL_FAN;
    static Block* BUBBLE_CORAL_FAN;
    static Block* FIRE_CORAL_FAN;
    static Block* HORN_CORAL_FAN;

    static Block* TUBE_CORAL_WALL_FAN;
    static Block* BRAIN_CORAL_WALL_FAN;
    static Block* BUBBLE_CORAL_WALL_FAN;
    static Block* FIRE_CORAL_WALL_FAN;
    static Block* HORN_CORAL_WALL_FAN;

    // 火把
    static Block* TORCH;
    static Block* WALL_TORCH;

    static Block* CONDUIT;

    // 蜂巢/蜂箱
    static Block* BEE_NEST;
    static Block* BEEHIVE;
};

void registerNaturalBlocks();

} // namespace block_registry
} // namespace mc
