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

#include "../Pools.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

/**
 * @brief 堡垒遗迹模板池统一注册入口
 *
 * 堡垒遗迹有 4 种类型：
 * - 单元型 (Units) - 起始池: bastion/units/start
 * - 猪灵兽栏 (Stables) - 起始池: bastion/stables/start
 * - 宝藏型 (Treasure) - 起始池: bastion/treasure/start
 * - 桥梁型 (Bridge) - 起始池: bastion/bridge/start
 */
namespace BastionPools {

/**
 * @brief 注册所有堡垒遗迹模板池
 *
 * 包括：
 * 1. 起始池（4 种类型的起始结构）
 * 2. 各类型的扩展池
 * 3. 连接池（bridge, legs, walls 等）
 */
void registerAll(JigsawPatternRegistry& registry);

/**
 * @brief 检查是否已注册
 */
bool isRegistered();

} // namespace BastionPools

// ============================================================================
// 堡垒遗迹子类型模板池命名空间
// ============================================================================

/**
 * @brief 单元型堡垒遗迹模板池
 *
 * 模板池:
 * - bastion/units/start (起始池)
 * - bastion/units/center_pieces
 * - bastion/units/center_bridges
 * - bastion/units/stages
 * - bastion/units/walls
 * - bastion/units/walls/edges
 * - bastion/units/walls/walls
 * - bastion/units/roofs
 * - bastion/units/ramparts
 * - bastion/units/rampart_plates
 */
namespace BastionUnitsPools {
void registerAll(JigsawPatternRegistry& registry);
}

/**
 * @brief 猪灵兽栏堡垒遗迹模板池
 *
 * 模板池:
 * - bastion/stables/start (起始池)
 * - bastion/stables/legs
 * - bastion/stables/walls
 * - bastion/stables/ramparts
 * - bastion/stables/air_base
 * - bastion/stables/bridges
 * - bastion/stables/roofs
 * - bastion/stables/connectors
 */
namespace BastionStablesPools {
void registerAll(JigsawPatternRegistry& registry);
}

/**
 * @brief 宝藏型堡垒遗迹模板池
 *
 * 模板池:
 * - bastion/treasure/start (起始池)
 * - bastion/treasure/legs
 * - bastion/treasure/walls
 * - bastion/treasure/ramparts
 * - bastion/treasure/air_base
 * - bastion/treasure/bridges
 * - bastion/treasure/roofs
 * - bastion/treasure/extensions
 * - bastion/treasure/centers
 */
namespace BastionTreasurePools {
void registerAll(JigsawPatternRegistry& registry);
}

/**
 * @brief 桥梁型堡垒遗迹模板池
 *
 * 模板池:
 * - bastion/bridge/start (起始池)
 * - bastion/bridge/legs
 * - bastion/bridge/walls
 * - bastion/bridge/ramparts
 * - bastion/bridge/air_base
 * - bastion/bridge/bridges
 * - bastion/bridge/roofs
 * - bastion/bridge/connectors
 */
namespace BastionBridgePools {
void registerAll(JigsawPatternRegistry& registry);
}

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
