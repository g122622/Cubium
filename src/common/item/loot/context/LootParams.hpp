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

#include "LootParameter.hpp"
#include "common/core/Types.hpp"

namespace mc {

// Forward declarations
class Entity;
class Player;
class DamageSource;
class BlockState;
class BlockPos;
class BlockEntity;
class ItemStack;

namespace loot {

// 预定义掉落参数
// 注意：模板参数是值类型，使用时传入指针
namespace LootParams {
extern const LootParameter<Entity> THIS_ENTITY;         // 当前实体
extern const LootParameter<Player> KILLER_PLAYER;       // 击杀玩家
extern const LootParameter<Entity> KILLER_ENTITY;       // 击杀实体
extern const LootParameter<Entity> DIRECT_KILLER;       // 直接击杀者
extern const LootParameter<DamageSource> DAMAGE_SOURCE; // 伤害来源
extern const LootParameter<f32> LUCK;                   // 幸运值

// 方块相关参数
extern const LootParameter<BlockState> BLOCK_STATE;   // 被破坏的方块状态
extern const LootParameter<BlockPos> BLOCK_POS;       // 方块位置
extern const LootParameter<ItemStack> TOOL;           // 使用的工具
extern const LootParameter<BlockEntity> BLOCK_ENTITY; // 方块实体

// 附魔等级参数
extern const LootParameter<i32> FORTUNE_LEVEL;    // 时运附魔等级
extern const LootParameter<i32> SILK_TOUCH_LEVEL; // 精准采集附魔等级
extern const LootParameter<i32> LOOTING_MODIFIER; // 掠夺附魔等级修正

// 爆炸相关参数
extern const LootParameter<f32> EXPLOSION_RADIUS; // 爆炸半径

// 钓鱼相关参数
extern const LootParameter<bool> IS_IN_OPEN_WATER; // 是否在开放水域
} // namespace LootParams

} // namespace loot
} // namespace mc
