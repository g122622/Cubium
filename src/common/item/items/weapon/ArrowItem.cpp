#include "ArrowItem.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../enchantment/EnchantmentHelper.hpp"
#include "../../enchantment/enchantments/AllEnchantments.hpp"

namespace mc {
namespace item {

ArrowItem::ArrowItem(const ItemProperties& properties)
    : Item(properties)
{
}

entity::AbstractArrowEntity* ArrowItem::createArrow(
    IWorld& world,
    const ItemStack& stack,
    LivingEntity& shooter) const
{
    (void)stack;  // 普通箭不使用物品堆信息
    (void)world;
    (void)shooter;

    // TODO: 需要实现实体工厂方法
    // 目前返回 nullptr，等待实体系统完善后实现
    // auto arrow = ArrowEntity::createFromShooter(shooter, &world);
    // return arrow.release();
    return nullptr;
}

bool ArrowItem::isInfinite(
    const ItemStack& arrowStack,
    const ItemStack& bowStack,
    Player& player) const
{
    // 创造模式总是无限
    if (player.isCreative()) {
        return true;
    }

    // 检查弓是否有无限附魔
    i32 infinityLevel = enchant::EnchantmentHelper::getEnchantmentLevel(
        bowStack, &enchant::AllEnchantments::INFINITY_ARROW);

    if (infinityLevel <= 0) {
        return false;
    }

    // MC 1.16.5: 只有普通箭（ArrowItem类）受益于无限附魔
    // 光灵箭和药水箭需要子类重写此方法返回false
    // 注意：光灵箭物品应该是 SpectralArrowItem，药水箭应该是 TippedArrowItem
    // 它们会重写此方法返回 false
    (void)arrowStack;  // 普通箭不检查物品堆类型
    return true;
}

} // namespace item
} // namespace mc
