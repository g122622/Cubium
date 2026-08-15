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

#include "WitherRoseBlock.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

// ========== WitherRoseBlock ==========

void WitherRoseBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(pos);

    // 1. 客户端世界不施加效果
    if (world.isClientSide()) {
        return;
    }

    // 2. 和平难度不施加凋零效果
    if (world.difficulty() == Difficulty::Peaceful) {
        return;
    }

    // 3. 仅对活体生物（LivingEntity）生效
    auto* living = dynamic_cast<LivingEntity*>(&entity);
    if (living == nullptr) {
        return;
    }

    // 4. 亡灵族（含凋灵骷髅、凋灵 boss）免疫：UNDEAD 标签已含 wither_skeleton 与 wither，
    //    单次 contains 判定即可覆盖（亡灵 boss 亦走 WitherBoss 路径，统一由 UNDEAD 标签兜底）。
    if (EntityTypeTags::UNDEAD().contains(entity.getTypeId())) {
        return;
    }

    // 5. 已有凋零效果则跳过，避免重复施加刷新剩余时间
    if (living->hasEffect(entity::effect::EffectType::Wither)) {
        return;
    }

    // 6. 施加凋零 I（amplifier=0）/ 40 tick（2 秒）。对齐 wiki：碰撞施加凋零 I、0:02。
    living->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Wither, 40, 0));
}

} // namespace blocks
} // namespace mc
