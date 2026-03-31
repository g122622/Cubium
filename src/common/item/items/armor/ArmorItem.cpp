#include "ArmorItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"

namespace mc {
namespace item::items {

ArmorItem::ArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot,
                     ItemProperties properties)
    : Item(std::move(properties))
    , m_material(material)
    , m_slot(slot) {
}

f32 ArmorItem::getDestroySpeed(const ItemStack& /*stack*/, const BlockState& /*state*/) const {
    // 盔甲不是工具，返回默认速度
    return 1.0f;
}

ItemActionResult ArmorItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    // TODO: 实现盔甲装备逻辑
    // 需要在 Player 类中添加 getArmorStack/setArmorStack 方法
    (void)world;
    (void)player;
    (void)hand;
    return ItemActionResult::pass(player.getHeldItem(hand));
}

i32 ArmorItem::getTotalArmorValue(const LivingEntity& entity) {
    i32 total = 0;

    // TODO: 实现获取盔甲槽位的物品
    // 遍历所有盔甲槽位
    // for (i32 i = 0; i < 4; ++i) {
    //     ItemStack stack = entity.getArmorStack(i);
    //     ...
    // }
    (void)entity;
    return total;
}

f32 ArmorItem::getTotalToughness(const LivingEntity& entity) {
    f32 total = 0.0f;

    // TODO: 实现获取盔甲槽位的物品
    // for (i32 i = 0; i < 4; ++i) {
    //     ItemStack stack = entity.getArmorStack(i);
    //     ...
    // }
    (void)entity;
    return total;
}

f32 ArmorItem::getTotalKnockbackResistance(const LivingEntity& entity) {
    f32 total = 0.0f;

    // TODO: 实现获取盔甲槽位的物品
    // for (i32 i = 0; i < 4; ++i) {
    //     ItemStack stack = entity.getArmorStack(i);
    //     ...
    // }
    (void)entity;
    return std::min(total, 1.0f);  // 上限为1.0
}

} // namespace item::items
} // namespace mc
