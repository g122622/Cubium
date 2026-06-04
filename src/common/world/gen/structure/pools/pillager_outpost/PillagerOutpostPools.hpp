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
 * @brief 掠夺者前哨站模板池
 *
 * 掠夺者前哨站是一个相对简单的结构，主要包括：
 * - 主塔 (base_plate)
 * - 塔楼各层
 * - 观察塔 (watchtower)
 * - 外围特征 (cage, tent, logs, targets 等)
 */
namespace PillagerOutpostPools {

/**
 * @brief 注册所有掠夺者前哨站模板池
 *
 * 包括：
 * 1. 起始池 (feature_cage_with_allays, base_plate, watchtower)
 * 2. 塔楼扩展池
 * 3. 外围特征池
 */
void registerAll(JigsawPatternRegistry& registry);

/**
 * @brief 检查是否已注册
 */
bool isRegistered();

} // namespace PillagerOutpostPools

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
