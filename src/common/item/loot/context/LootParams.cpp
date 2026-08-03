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

#include "LootParams.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootParameter.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"

namespace mc {
namespace loot {

// 预定义掉落参数
namespace LootParams {
const LootParameter<Entity> THIS_ENTITY("this_entity");
const LootParameter<Player> KILLER_PLAYER("killer_player");
const LootParameter<Entity> KILLER_ENTITY("killer_entity");
const LootParameter<Entity> DIRECT_KILLER("direct_killer");
const LootParameter<DamageSource> DAMAGE_SOURCE("damage_source");
const LootParameter<f32> LUCK("luck");

// 方块相关参数
const LootParameter<BlockState> BLOCK_STATE("block_state");
const LootParameter<BlockPos> BLOCK_POS("block_pos");
const LootParameter<ItemStack> TOOL("tool");
const LootParameter<BlockEntity> BLOCK_ENTITY("block_entity");

// 附魔等级参数
const LootParameter<i32> FORTUNE_LEVEL("fortune_level");
const LootParameter<i32> SILK_TOUCH_LEVEL("silk_touch_level");
const LootParameter<i32> LOOTING_MODIFIER("looting_modifier");

// 爆炸相关参数
const LootParameter<f32> EXPLOSION_RADIUS("explosion_radius");

// 钓鱼相关参数
const LootParameter<bool> IS_IN_OPEN_WATER("is_in_open_water");
} // namespace LootParams

} // namespace loot
} // namespace mc
