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

// 便利头文件：包含所有战利品条件类
// 使用此头文件可一次性引入所有 loot condition 实现

#include "common/item/loot/conditions/AndCondition.hpp"
#include "common/item/loot/conditions/BlockStateCondition.hpp"
#include "common/item/loot/conditions/DamageSourcePropertiesCondition.hpp"
#include "common/item/loot/conditions/EntityPropertiesCondition.hpp"
#include "common/item/loot/conditions/EntityScoresCondition.hpp"
#include "common/item/loot/conditions/FishingOpenWaterCondition.hpp"
#include "common/item/loot/conditions/FortuneCondition.hpp"
#include "common/item/loot/conditions/KilledByPlayerCondition.hpp"
#include "common/item/loot/conditions/LocationCheckCondition.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/conditions/LootConditionBuilder.hpp"
#include "common/item/loot/conditions/NotCondition.hpp"
#include "common/item/loot/conditions/OrCondition.hpp"
#include "common/item/loot/conditions/RandomChanceCondition.hpp"
#include "common/item/loot/conditions/RandomChanceWithLuckCondition.hpp"
#include "common/item/loot/conditions/ReferenceCondition.hpp"
#include "common/item/loot/conditions/SilkTouchCondition.hpp"
#include "common/item/loot/conditions/SurvivesExplosionCondition.hpp"
#include "common/item/loot/conditions/TimeCheckCondition.hpp"
#include "common/item/loot/conditions/ToolTypeCondition.hpp"
#include "common/item/loot/conditions/WeatherCheckCondition.hpp"
