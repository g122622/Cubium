#pragma once

#include "Food.hpp"

namespace mc {
namespace item::food {

/**
 * @brief 原版食物定义
 *
 * 包含所有 Minecraft 1.16.5 原版食物的属性定义。
 * 参考: net.minecraft.item.Foods
 */
namespace Foods {

// ========== 基础食物 ==========

/// 苹果 (4饥饿, 0.3饱和度)
extern const Food APPLE;

/// 烤马铃薯 (5饥饿, 0.6饱和度)
extern const Food BAKED_POTATO;

/// 甜菜根 (1饥饿, 0.1饱和度)
extern const Food BEETROOT;

/// 面包 (5饥饿, 0.6饱和度)
extern const Food BREAD;

/// 胡萝卜 (3饥饿, 0.2饱和度)
extern const Food CARROT;

/// 紫颂果 (4饥饿, 0.2饱和度) - 可传送
extern const Food CHORUS_FRUIT;

/// 熟鸡肉 (6饥饿, 0.6饱和度) - 肉类
extern const Food COOKED_CHICKEN;

/// 熟鳕鱼 (5饥饿, 0.6饱和度) - 肉类
extern const Food COOKED_COD;

/// 熟羊肉 (6饥饿, 0.8饱和度) - 肉类
extern const Food COOKED_MUTTON;

/// 熟猪排 (8饥饿, 0.8饱和度) - 肉类
extern const Food COOKED_PORKCHOP;

/// 熟兔肉 (5饥饿, 0.6饱和度) - 肉类
extern const Food COOKED_RABBIT;

/// 熟鲑鱼 (6饥饿, 0.8饱和度) - 肉类
extern const Food COOKED_SALMON;

/// 曲奇 (2饥饿, 0.1饱和度)
extern const Food COOKIE;

/// 干海带 (1饥饿, 0.0饱和度)
extern const Food DRIED_KELP;

/// 蜂蜜瓶 (6饥饿, 0.0饱和度) - 可治愈中毒
extern const Food HONEY_BOTTLE;

/// 西瓜片 (2饥饿, 0.1饱和度)
extern const Food MELON_SLICE;

/// 蘑菇汤 (6饥饿, 0.6饱和度) - 返回碗
extern const Food MUSHROOM_STEW;

/// 毒马铃薯 (1饥饿, 0.0饱和度) - 60%概率中毒
extern const Food POISONOUS_POTATO;

/// 生牛肉 (3饥饿, 0.2饱和度)
extern const Food BEEF;

/// 生鸡肉 (2饥饿, 0.1饱和度) - 30%概率饥饿
extern const Food CHICKEN;

/// 生鳕鱼 (2饥饿, 0.1饱和度)
extern const Food COD;

/// 生羊肉 (2饥饿, 0.1饱和度)
extern const Food MUTTON;

/// 生猪排 (3饥饿, 0.1饱和度)
extern const Food PORKCHOP;

/// 生兔肉 (3饥饿, 0.1饱和度)
extern const Food RABBIT;

/// 生鲑鱼 (2饥饿, 0.1饱和度)
extern const Food SALMON;

/// 腐肉 (4饥饿, 0.1饱和度) - 80%概率饥饿
extern const Food ROTTEN_FLESH;

/// 蜘蛛眼 (2饥饿, 0.1饱和度) - 中毒
extern const Food SPIDER_EYE;

/// 甜浆果 (2饥饿, 0.1饱和度)
extern const Food SWEET_BERRIES;

// ========== 金苹果 ==========

/// 金苹果 (4饥饿, 1.2饱和度) - 给予生命恢复II、吸收效果
extern const Food GOLDEN_APPLE;

/// 附魔金苹果 (4饥饿, 1.2饱和度) - 给予生命恢复II、吸收、防火、抗性提升
extern const Food ENCHANTED_GOLDEN_APPLE;

// ========== 汤类 ==========

/// 甜菜汤 (6饥饿, 0.6饱和度) - 返回碗
extern const Food BEETROOT_SOUP;

/// 兔肉汤 (12饥饿, 0.6饱和度) - 返回碗
extern const Food RABBIT_STEW;

/// 迷之炖菜 (6饥饿, 0.6饱和度) - 随机效果，返回碗
extern const Food SUSPIC_STEW;

// ========== 特殊鱼类 ==========

/// 河豚 (1饥饿, 0.1饱和度) - 中毒IV、饥饿III、反胃
extern const Food PUFFERFISH;

/// 热带鱼 (1饥饿, 0.1饱和度)
extern const Food TROPICAL_FISH;

// ========== 初始化所有食物 ==========
void initialize();

} // namespace Foods
} // namespace item::food
} // namespace mc
