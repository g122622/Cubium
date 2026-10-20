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

#include "common/core/Types.hpp"
#include "common/entity/effect/EffectType.hpp"

namespace mc {

class IWorld;
class LivingEntity;
enum class RemovalReason : u8;

namespace entity {
namespace effect {

/**
 * @brief 效果在附体实体被移除时的回调（对齐 vanilla MobEffect.onMobRemoved）
 *
 * 供 LivingEntity::triggerOnDeathMobEffects 在实体因 KILLED/DISCARDED 被移除时对每个
 * 活跃效果调用。对应原版各效果类的 onMobRemoved 覆写：
 *   - WEAVING  → 掉落后在实体周围随机生成蜘蛛网（WeavingMobEffect）
 *   - OOZING   → 掉落时生成小史莱姆（OozingMobEffect）
 *   - WIND_CHARGED → 掉落时触发风弹式爆炸（WindChargedMobEffect）
 *   - INFESTED → 无 onMobRemoved 行为（其效果在 onMobHurt 时触发，见 TODO）
 *   - 其余效果 → 空实现
 *
 * 与 EffectManager::tick / addEffect 的风格一致：世界上下文由调用方经参数传入，
 * 本模块对具体实体与世界实现零依赖（可选定义定义在本模块的 onRemoval 实现文件中）。
 */
namespace onremoval {

/**
 * @brief 对单个效果执行"附体实体被移除"回调
 * @param world 实体所在世界（服务端）
 * @param entity 被移除的实体
 * @param type 效果类型
 * @param amplifier 效果等级（0 为 I 级）
 * @param reason 移除原因（KILLED / DISCARDED / 其他）
 */
void applyOnMobRemoved(IWorld& world, LivingEntity& entity, EffectType type, i32 amplifier, RemovalReason reason);

/**
 * @brief INFESTED 效果的受击回调：在实体位置生成 1~2 只蠹虫
 *
 * 对齐 vanilla InfestedMobEffect.onMobHurt + spawnSilverfish：蠹虫生成于实体碰撞箱中部，
 * 沿实体视线方向随机偏转让位，带 0.3 的水平初速度并被立即加入世界。
 * 概率门控（10%）由调用方 LivingEntity::actuallyHurt 完成。
 */
void spawnInfestedSilverfish(IWorld& world, LivingEntity& entity);

} // namespace onremoval

} // namespace effect
} // namespace entity
} // namespace mc