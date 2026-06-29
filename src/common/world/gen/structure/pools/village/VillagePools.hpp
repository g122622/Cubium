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

#include "common/world/gen/structure/pools/Pools.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

/**
 * @brief 村庄模板池统一注册入口
 *
 * 管理 5 种村庄类型的模板池：
 * - 平原村庄 (PlainsVillagePools)
 * - 沙漠村庄 (DesertVillagePools)
 * - 热带草原村庄 (SavannaVillagePools)
 * - 雪地村庄 (SnowyVillagePools)
 * - 针叶林村庄 (TaigaVillagePools)
 */
namespace VillagePools {

/**
 * @brief 注册所有村庄模板池
 *
 * 包括：
 * 1. 空池（已由 Pools::registerEmptyPool 注册）
 * 2. 公共池（animals, cats, iron_golem, well_bottoms 等）
 * 3. 各生物群系特定池
 */
void registerAll(TemplatePoolRegistry& registry);

/**
 * @brief 注册公共模板池
 *
 * 注册公共池：
 * - village/common/animals
 * - village/common/cats
 * - village/common/iron_golem
 * - village/common/well_bottoms
 * - village/common/butcher_animals
 * - village/common/sheep
 */
void registerCommonPools(TemplatePoolRegistry& registry);

/**
 * @brief 检查是否已注册
 */
bool isRegistered();

} // namespace VillagePools

// ============================================================================
// 各生物群系村庄模板池命名空间
// ============================================================================

/**
 * @brief 平原村庄模板池
 *
 * 模板池:
 * - village/plains/town_centers (起始池)
 * - village/plains/streets
 * - village/plains/houses
 * - village/plains/terminators
 * - village/plains/trees
 * - village/plains/decor
 * - village/plains/villagers
 * - village/plains/zombie/streets
 * - village/plains/zombie/houses
 * - village/plains/zombie/decor
 * - village/plains/zombie/villagers
 */
namespace PlainsVillagePools {
void registerAll(TemplatePoolRegistry& registry);
}

/**
 * @brief 沙漠村庄模板池
 */
namespace DesertVillagePools {
void registerAll(TemplatePoolRegistry& registry);
}

/**
 * @brief 热带草原村庄模板池
 */
namespace SavannaVillagePools {
void registerAll(TemplatePoolRegistry& registry);
}

/**
 * @brief 雪地村庄模板池
 */
namespace SnowyVillagePools {
void registerAll(TemplatePoolRegistry& registry);
}

/**
 * @brief 针叶林村庄模板池
 */
namespace TaigaVillagePools {
void registerAll(TemplatePoolRegistry& registry);
}

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
