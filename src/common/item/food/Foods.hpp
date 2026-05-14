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

#include "Food.hpp"

namespace mc {
namespace item::food {

/**
 * @brief 原版食物定义
 *
 * 包含所有 Minecraft 1.16.5 原版食物的属性定义。
 * 参考: net.minecraft.item.Foods
 *
 * 饱和度计算公式：saturation = food * saturationModifier * 2.0
 * 例如苹果 (4, 0.3F)：saturation = 4 * 0.3 * 2.0 = 2.4
 */
namespace Foods {

// ========== 基础食物 ==========

/// 苹果 (4饥饿, 0.3饱和度修正)
extern const Food APPLE;

/// 烤马铃薯 (5饥饿, 0.6饱和度修正)
extern const Food BAKED_POTATO;

/// 生牛肉 (3饥饿, 0.3饱和度修正, 肉类)
extern const Food BEEF;

/// 甜菜根 (1饥饿, 0.6饱和度修正)
extern const Food BEETROOT;

/// 甜菜汤 (6饥饿, 0.6饱和度修正, 返回碗)
extern const Food BEETROOT_SOUP;

/// 面包 (5饥饿, 0.6饱和度修正)
extern const Food BREAD;

/// 胡萝卜 (3饥饿, 0.6饱和度修正)
extern const Food CARROT;

/// 生鸡肉 (2饥饿, 0.3饱和度修正, 肉类, 30%饥饿效果)
extern const Food CHICKEN;

/// 紫颂果 (4饥饿, 0.3饱和度修正, 始终可食用)
extern const Food CHORUS_FRUIT;

/// 生鳕鱼 (2饥饿, 0.1饱和度修正)
extern const Food COD;

/// 熟牛肉 (8饥饿, 0.8饱和度修正, 肉类)
extern const Food COOKED_BEEF;

/// 熟鸡肉 (6饥饿, 0.6饱和度修正, 肉类)
extern const Food COOKED_CHICKEN;

/// 熟鳕鱼 (5饥饿, 0.6饱和度修正)
extern const Food COOKED_COD;

/// 熟羊肉 (6饥饿, 0.8饱和度修正, 肉类)
extern const Food COOKED_MUTTON;

/// 熟猪排 (8饥饿, 0.8饱和度修正, 肉类)
extern const Food COOKED_PORKCHOP;

/// 熟兔肉 (5饥饿, 0.6饱和度修正, 肉类)
extern const Food COOKED_RABBIT;

/// 熟鲑鱼 (6饥饿, 0.8饱和度修正)
extern const Food COOKED_SALMON;

/// 曲奇 (2饥饿, 0.1饱和度修正)
extern const Food COOKIE;

/// 干海带 (1饥饿, 0.3饱和度修正, 快速食用)
extern const Food DRIED_KELP;

/// 蜂蜜 (6饥饿, 0.1饱和度修正) - 注意：这不是蜂蜜瓶，蜂蜜瓶返回玻璃瓶
extern const Food HONEY;

/// 西瓜片 (2饥饿, 0.3饱和度修正)
extern const Food MELON_SLICE;

/// 蘑菇汤 (6饥饿, 0.6饱和度修正, 返回碗)
extern const Food MUSHROOM_STEW;

/// 生羊肉 (2饥饿, 0.3饱和度修正, 肉类)
extern const Food MUTTON;

/// 毒马铃薯 (2饥饿, 0.3饱和度修正, 60%概率中毒)
extern const Food POISONOUS_POTATO;

/// 生猪排 (3饥饿, 0.3饱和度修正, 肉类)
extern const Food PORKCHOP;

/// 马铃薯 (1饥饿, 0.3饱和度修正)
extern const Food POTATO;

/// 河豚 (1饥饿, 0.1饱和度修正, 中毒IV/饥饿III/反胃)
extern const Food PUFFERFISH;

/// 南瓜派 (8饥饿, 0.3饱和度修正)
extern const Food PUMPKIN_PIE;

/// 生兔肉 (3饥饿, 0.3饱和度修正, 肉类)
extern const Food RABBIT;

/// 兔肉汤 (10饥饿, 0.6饱和度修正, 返回碗)
extern const Food RABBIT_STEW;

/// 腐肉 (4饥饿, 0.1饱和度修正, 肉类, 80%概率饥饿)
extern const Food ROTTEN_FLESH;

/// 生鲑鱼 (2饥饿, 0.1饱和度修正)
extern const Food SALMON;

/// 蜘蛛眼 (2饥饿, 0.8饱和度修正, 100%概率中毒)
extern const Food SPIDER_EYE;

/// 迷之炖菜 (6饥饿, 0.6饱和度修正, 随机效果, 返回碗)
extern const Food SUSPICIOUS_STEW;

/// 甜浆果 (2饥饿, 0.1饱和度修正)
extern const Food SWEET_BERRIES;

/// 热带鱼 (1饥饿, 0.1饱和度修正)
extern const Food TROPICAL_FISH;

// ========== 金苹果 ==========

/// 金苹果 (4饥饿, 1.2饱和度修正, 始终可食用, 生命恢复II 5秒, 吸收 2分钟)
extern const Food GOLDEN_APPLE;

/// 附魔金苹果 (4饥饿, 1.2饱和度修正, 始终可食用, 生命恢复II 20秒, 抗性提升 5分钟, 防火 5分钟, 吸收IV 2分钟)
extern const Food ENCHANTED_GOLDEN_APPLE;

/// 金胡萝卜 (6饥饿, 1.2饱和度修正)
extern const Food GOLDEN_CARROT;

/// 蜂蜜瓶 (6饥饿, 0.1饱和度修正, 治愈中毒, 返回玻璃瓶)
extern const Food HONEY_BOTTLE;

// ========== 初始化所有食物 ==========
void initialize();

} // namespace Foods
} // namespace item::food
} // namespace mc
