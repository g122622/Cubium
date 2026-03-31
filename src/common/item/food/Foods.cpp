#include "Foods.hpp"

namespace mc {
namespace item::food {

// ========== 基础食物 ==========

const Food APPLE(4, 0.3f);

const Food BAKED_POTATO(5, 0.6f);

const Food BEETROOT(1, 0.1f);

const Food BREAD(5, 0.6f);

const Food CARROT(3, 0.2f);

const Food CHORUS_FRUIT(4, 0.2f);  // 可传送

const Food COOKED_CHICKEN = Food(6, 0.6f).setMeat();

const Food COOKED_COD = Food(5, 0.6f).setMeat();

const Food COOKED_MUTTON = Food(6, 0.8f).setMeat();

const Food COOKED_PORKCHOP = Food(8, 0.8f).setMeat();

const Food COOKED_RABBIT = Food(5, 0.6f).setMeat();

const Food COOKED_SALMON = Food(6, 0.8f).setMeat();

const Food COOKIE(2, 0.1f);

const Food DRIED_KELP(1, 0.0f);

const Food HONEY_BOTTLE(6, 0.0f);  // 治愈中毒，返回玻璃瓶

const Food MELON_SLICE(2, 0.1f);

const Food MUSHROOM_STEW(6, 0.6f);  // 返回碗

const Food POISONOUS_POTATO(1, 0.0f);  // 60%概率中毒

const Food BEEF(3, 0.2f);

const Food CHICKEN(2, 0.1f);  // 30%概率饥饿

const Food COD(2, 0.1f);

const Food MUTTON(2, 0.1f);

const Food PORKCHOP(3, 0.1f);

const Food RABBIT(3, 0.1f);

const Food SALMON(2, 0.1f);

const Food ROTTEN_FLESH(4, 0.1f);  // 80%概率饥饿

const Food SPIDER_EYE(2, 0.1f);  // 中毒

const Food SWEET_BERRIES(2, 0.1f);

// ========== 金苹果 ==========

const Food GOLDEN_APPLE = Food(4, 1.2f).setAlwaysEdible();

const Food ENCHANTED_GOLDEN_APPLE = Food(4, 1.2f).setAlwaysEdible();

// ========== 汤类 ==========

const Food BEETROOT_SOUP(6, 0.6f);  // 返回碗

const Food RABBIT_STEW(12, 0.6f);  // 返回碗

const Food SUSPIC_STEW(6, 0.6f);  // 随机效果，返回碗

// ========== 特殊鱼类 ==========

const Food PUFFERFISH(1, 0.1f);  // 中毒IV、饥饿III、反胃

const Food TROPICAL_FISH(1, 0.1f);

void Foods::initialize() {
    // 食物已通过静态初始化创建
    // 此函数可用于验证所有食物已正确初始化
}

} // namespace item::food
} // namespace mc
