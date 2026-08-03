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

#include "Foods.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/item/food/Food.hpp"

namespace mc {
namespace item::food {

using EffectType = entity::effect::EffectType;

// ========== 基础食物 ==========

const Food Foods::APPLE(4, 0.3f);

const Food Foods::BAKED_POTATO(5, 0.6f);

const Food Foods::BEEF = Food(3, 0.3f).setMeat();

const Food Foods::BEETROOT(1, 0.6f);

const Food Foods::BEETROOT_SOUP(6, 0.6f); // 返回碗

const Food Foods::BREAD(5, 0.6f);

const Food Foods::CARROT(3, 0.6f);

const Food Foods::CHICKEN = Food(2, 0.3f).setMeat().addEffect(EffectType::Hunger, 600, 0, 0.3f); // 30%概率饥饿 30秒

const Food Foods::CHORUS_FRUIT = Food(4, 0.3f).setAlwaysEdible(); // 传送效果

const Food Foods::COD(2, 0.1f);

const Food Foods::COOKED_BEEF = Food(8, 0.8f).setMeat();

const Food Foods::COOKED_CHICKEN = Food(6, 0.6f).setMeat();

const Food Foods::COOKED_COD(5, 0.6f); // 注意：鳕鱼不是肉类

const Food Foods::COOKED_MUTTON = Food(6, 0.8f).setMeat();

const Food Foods::COOKED_PORKCHOP = Food(8, 0.8f).setMeat();

const Food Foods::COOKED_RABBIT = Food(5, 0.6f).setMeat();

const Food Foods::COOKED_SALMON(6, 0.8f); // 注意：鲑鱼不是肉类

const Food Foods::COOKIE(2, 0.1f);

const Food Foods::DRIED_KELP = Food(1, 0.3f).setFastEat();

const Food Foods::HONEY(6, 0.1f); // 注意：这只是蜂蜜食物属性，蜂蜜瓶还有额外效果

const Food Foods::MELON_SLICE(2, 0.3f);

const Food Foods::MUSHROOM_STEW(6, 0.6f); // 返回碗

const Food Foods::MUTTON = Food(2, 0.3f).setMeat();

const Food Foods::POISONOUS_POTATO = Food(2, 0.3f).addEffect(EffectType::Poison, 100, 0, 0.6f); // 60%概率中毒 5秒

const Food Foods::PORKCHOP = Food(3, 0.3f).setMeat();

const Food Foods::POTATO(1, 0.3f);

const Food Foods::PUFFERFISH = Food(1, 0.1f)
                                   .addEffect(EffectType::Poison, 1200, 3, 1.0f) // 中毒IV 60秒
                                   .addEffect(EffectType::Hunger, 300, 2, 1.0f)  // 饥饿III 15秒
                                   .addEffect(EffectType::Nausea, 300, 0, 1.0f); // 反胃 15秒

const Food Foods::PUMPKIN_PIE(8, 0.3f);

const Food Foods::RABBIT = Food(3, 0.3f).setMeat();

const Food Foods::RABBIT_STEW(10, 0.6f); // 返回碗

const Food Foods::ROTTEN_FLESH =
    Food(4, 0.1f).setMeat().addEffect(EffectType::Hunger, 600, 0, 0.8f); // 80%概率饥饿 30秒

const Food Foods::SALMON(2, 0.1f);

const Food Foods::SPIDER_EYE = Food(2, 0.8f).addEffect(EffectType::Poison, 100, 0, 1.0f); // 100%概率中毒 5秒

const Food Foods::SUSPICIOUS_STEW = Food(6, 0.6f).setAlwaysEdible(); // 随机效果，返回碗，始终可食用

const Food Foods::SWEET_BERRIES(2, 0.1f);

const Food Foods::GLOW_BERRIES(2, 0.1f);

const Food Foods::TROPICAL_FISH(1, 0.1f);

// ========== 金苹果 ==========

const Food Foods::GOLDEN_APPLE = Food(4, 1.2f)
                                     .setAlwaysEdible()
                                     .addEffect(EffectType::Regeneration, 100, 1, 1.0f) // 生命恢复II 5秒
                                     .addEffect(EffectType::Absorption, 2400, 0, 1.0f); // 吸收 2分钟

const Food Foods::ENCHANTED_GOLDEN_APPLE = Food(4, 1.2f)
                                               .setAlwaysEdible()
                                               .addEffect(EffectType::Regeneration, 400, 1, 1.0f)    // 生命恢复II 20秒
                                               .addEffect(EffectType::Resistance, 6000, 0, 1.0f)     // 抗性提升 5分钟
                                               .addEffect(EffectType::FireResistance, 6000, 0, 1.0f) // 防火 5分钟
                                               .addEffect(EffectType::Absorption, 2400, 3, 1.0f);    // 吸收IV 2分钟

const Food Foods::GOLDEN_CARROT(6, 1.2f);

const Food Foods::HONEY_BOTTLE = Food(6, 0.1f); // 治愈中毒，返回玻璃瓶

void Foods::initialize()
{
    // 食物已通过静态初始化创建
    // 此函数可用于验证所有食物已正确初始化
}

} // namespace item::food
} // namespace mc
