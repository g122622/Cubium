#include "TippedArrowItem.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../potion/PotionRegistry.hpp"
#include "../../potion/PotionUtils.hpp"

namespace mc {
namespace item {

// NBT 键
static constexpr const char* NBT_POTION = "Potion";
static constexpr const char* NBT_CUSTOM_POTION_EFFECTS = "CustomPotionEffects";

TippedArrowItem::TippedArrowItem(const ItemProperties& properties)
    : ArrowItem(properties)
{}

entity::AbstractArrowEntity* TippedArrowItem::createArrow(
    IWorld& world, const ItemStack& stack, LivingEntity& shooter) const
{
    // 创建箭矢实体
    auto arrow = entity::ArrowEntity::createFromShooter(shooter, &world);
    if (!arrow) {
        return nullptr;
    }

    // 获取药水效果
    auto effects = getEffects(stack);
    if (!effects.empty()) {
        // 设置药水效果到箭矢
        arrow->setEffects(effects);

        // 设置箭矢颜色
        u32 color = potion::PotionUtils::getColor(effects);
        arrow->setColor(color);
    }

    return arrow.release();
}

bool TippedArrowItem::isInfinite(const ItemStack& /*arrowStack*/, const ItemStack& /*bowStack*/, Player& player) const
{
    // MC 1.16.5: 药水箭不受益于无限附魔
    // 只有创造模式下才能无限使用
    return player.isCreative();
}

const potion::Potion* TippedArrowItem::getPotion(const ItemStack& stack)
{
    // 使用 PotionUtils 获取药水
    return potion::PotionUtils::getPotion(stack);
}

std::vector<entity::effect::EffectInstance> TippedArrowItem::getEffects(const ItemStack& stack)
{
    // 获取药水的基础效果
    std::vector<entity::effect::EffectInstance> effects;

    const potion::Potion* potion = getPotion(stack);
    if (potion != nullptr) {
        effects = potion::PotionUtils::getEffects(potion);
    }

    // 注意：自定义效果（NBT CustomPotionEffects）需要在 NBT 系统完善后合并
    // 当前仅返回药水的基础效果

    return effects;
}

void TippedArrowItem::setPotion(ItemStack& stack, const potion::Potion* potion)
{
    potion::PotionUtils::setPotion(stack, potion);
}

} // namespace item
} // namespace mc
