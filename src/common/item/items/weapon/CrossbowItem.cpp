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

#include "CrossbowItem.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"
#include "common/item/items/weapon/ArrowItem.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>

namespace mc {
namespace item {

// ========== 常量 ==========
namespace {
constexpr i32 BASE_CHARGE_TIME = 25;               // 基础装填时间 (tick)
constexpr i32 CHARGE_TIME_REDUCTION_PER_LEVEL = 5; // 快速装填每级减少时间
constexpr f32 ARROW_VELOCITY = 3.15f;              // 箭矢速度
constexpr f32 FIREWORK_VELOCITY = 1.6f;            // 烟花速度
} // namespace

// ========== 构造函数 ==========

CrossbowItem::CrossbowItem(const ItemProperties& properties)
    : Item(properties)
{}

// ========== Item 接口重写 ==========

i32 CrossbowItem::getUseDuration(const ItemStack& stack) const
{
    return getChargeTime(stack) + 3;
}

UseAction CrossbowItem::getUseAction(const ItemStack& /*stack*/) const
{
    return UseAction::Crossbow;
}

ItemActionResult CrossbowItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{

    ItemStack crossbowStack = player.getHeldItem(hand);

    // 如果已装填，则发射
    if (isCharged(crossbowStack)) {
        // 检测烟花火箭
        f32 velocity = ARROW_VELOCITY;
        if (hasChargedProjectile(crossbowStack, Items::FIREWORK_ROCKET)) {
            velocity = FIREWORK_VELOCITY;
        }
        _fireProjectiles(world, player, crossbowStack, velocity, 1.0f, LivingEntity::handToEquipmentSlot(hand));
        setCharged(crossbowStack, false);
        return ItemActionResult::success(crossbowStack);
    }

    // 检查是否有弹药
    i32 ammoSlot = _findAmmoSlot(player);
    bool hasAmmo = ammoSlot >= 0;
    if (!hasAmmo && !player.isCreative()) {
        return ItemActionResult::fail(crossbowStack);
    }

    // 开始装填
    player.setActiveHand(hand);

    return ItemActionResult::success(crossbowStack);
}

void CrossbowItem::onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft)
{
    // 检查是否是玩家
    Player* player = dynamic_cast<Player*>(&entity);
    if (player == nullptr) {
        return;
    }

    // 计算装填时间
    i32 chargeTime = getUseDuration(stack) - timeLeft;
    f32 chargeProgress = static_cast<f32>(chargeTime) / static_cast<f32>(getChargeTime(stack));
    if (chargeProgress > 1.0f) {
        chargeProgress = 1.0f;
    }

    // 检查是否完全装填
    if (chargeProgress >= 1.0f && !isCharged(stack)) {
        // 装填弹丸
        if (_loadProjectiles(*player, stack)) {
            setCharged(stack, true);
            // 播放装填完成音效
            player->playSound(SoundEvents::ITEM_CROSSBOW_LOADING_END, 1.0f, 1.0f);
        }
    }
}

// ========== 弩特有方法 ==========

bool CrossbowItem::isCharged(const ItemStack& stack)
{
    const nlohmann::json* tag = stack.getTag();
    if (tag == nullptr) {
        return false;
    }
    auto it = tag->find("Charged");
    if (it == tag->end()) {
        return false;
    }
    return it->get<bool>();
}

void CrossbowItem::setCharged(ItemStack& stack, bool charged)
{
    stack.getOrCreateTag()["Charged"] = charged;
}

i32 CrossbowItem::getChargeTime(const ItemStack& stack)
{
    i32 quickChargeLevel =
        enchant::EnchantmentHelper::getEnchantmentLevel(stack, &enchant::AllEnchantments::QUICK_CHARGE);
    if (quickChargeLevel == 0) {
        return BASE_CHARGE_TIME;
    }
    return BASE_CHARGE_TIME - CHARGE_TIME_REDUCTION_PER_LEVEL * quickChargeLevel;
}

std::function<bool(const ItemStack&)> CrossbowItem::getAmmoPredicate() const
{
    return [](const ItemStack& stack) -> bool {
        if (stack.isEmpty()) {
            return false;
        }
        // 接受箭矢和烟花火箭
        const Item* item = stack.getItem();
        if (dynamic_cast<const ArrowItem*>(item) != nullptr) {
            return true;
        }
        // 检查烟花火箭
        return item == Items::FIREWORK_ROCKET;
    };
}

std::function<bool(const ItemStack&)> CrossbowItem::getInventoryAmmoPredicate() const
{
    return [](const ItemStack& stack) -> bool {
        if (stack.isEmpty()) {
            return false;
        }
        // 只接受箭矢
        return dynamic_cast<const ArrowItem*>(stack.getItem()) != nullptr;
    };
}

// ========== 私有方法 ==========

bool CrossbowItem::_isAmmo(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    // 接受箭矢和烟花火箭
    const Item* item = stack.getItem();
    if (dynamic_cast<const ArrowItem*>(item) != nullptr) {
        return true;
    }
    // 检查烟花火箭
    return item == Items::FIREWORK_ROCKET;
}

i32 CrossbowItem::_findAmmoSlot(Player& player)
{
    PlayerInventory& inventory = player.inventory();

    // 先检查副手（槽位 40）
    ItemStack offhand = player.getHeldItem(Hand::OffHand);
    if (_isAmmo(offhand)) {
        return InventorySlots::OFFHAND;
    }

    // 再检查主手（弩本身在主手时跳过，但如果是弩以外的东西则需要检查）
    ItemStack mainhand = player.getHeldItem(Hand::MainHand);
    if (_isAmmo(mainhand)) {
        return inventory.getSelectedSlot();
    }

    // 检查背包
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack slot = inventory.getItem(i);
        if (_isAmmo(slot)) {
            return i;
        }
    }

    return -1;
}

bool CrossbowItem::_loadProjectiles(Player& player, ItemStack& crossbow)
{
    i32 multishotLevel = _getMultishotLevel(crossbow);
    i32 projectileCount = multishotLevel > 0 ? 3 : 1;
    bool isCreative = player.isCreative();

    i32 ammoSlot = _findAmmoSlot(player);
    PlayerInventory& inventory = player.inventory();

    for (i32 i = 0; i < projectileCount; ++i) {
        ItemStack projectileToLoad;

        if (ammoSlot >= 0) {
            ItemStack ammo = inventory.getItem(ammoSlot);
            if (isCreative) {
                // 创造模式：复制弹药
                projectileToLoad = ammo.copy();
            } else {
                // 生存模式：从背包槽位消耗弹药
                // 使用 removeItem 直接操作背包槽位，正确处理数量减少和空堆清理
                projectileToLoad = inventory.removeItem(ammoSlot, 1);

                // 多重射击：后续弹丸需要额外弹药
                if (i < projectileCount - 1) {
                    // 检查当前槽位是否还有弹药
                    ItemStack remaining = inventory.getItem(ammoSlot);
                    if (remaining.isEmpty()) {
                        // 当前槽位已耗尽，寻找下一个弹药槽位
                        ammoSlot = _findAmmoSlot(player);
                    }
                }
            }
        } else if (isCreative) {
            // 创造模式无弹药时使用默认箭矢
            projectileToLoad = ItemStack(Items::ARROW, 1);
        }

        if (!projectileToLoad.isEmpty()) {
            _addChargedProjectile(crossbow, projectileToLoad);
        } else {
            // 弹药不足
            return false;
        }
    }

    return true;
}

void CrossbowItem::_fireProjectiles(
    IWorld& world, LivingEntity& shooter, ItemStack& crossbow, f32 velocity, f32 inaccuracy, EquipmentSlot slot)
{
    std::vector<ItemStack> projectiles = _getChargedProjectiles(crossbow);

    if (projectiles.empty()) {
        // 如果没有存储弹丸，创建默认箭矢
        projectiles.push_back(ItemStack(Items::ARROW, 1));
    }

    // 获取多重射击的弹道偏移
    std::vector<f32> projectileAngles;
    if (projectiles.size() == 1) {
        projectileAngles.push_back(0.0f);
    } else if (projectiles.size() >= 3) {
        projectileAngles = {0.0f, -10.0f, 10.0f};
    }

    bool isCreative = false;
    Player* player = dynamic_cast<Player*>(&shooter);
    if (player != nullptr) {
        isCreative = player->isCreative();
    }

    i32 piercingLevel = _getPiercingLevel(crossbow);

    // 播放发射音效
    bool hasFirework = false;
    for (const auto& proj : projectiles) {
        if (proj.getItem() == Items::FIREWORK_ROCKET) {
            hasFirework = true;
            break;
        }
    }
    if (player != nullptr) {
        if (hasFirework) {
            player->playSound(SoundEvents::ITEM_CROSSBOW_ROCKET, 1.0f, 1.0f);
        } else {
            player->playSound(SoundEvents::ITEM_CROSSBOW_SHOOT, 1.0f, 1.0f);
        }
    }

    for (size_t i = 0; i < projectiles.size() && i < projectileAngles.size(); ++i) {
        const ItemStack& projectile = projectiles[i];
        if (projectile.isEmpty()) {
            continue;
        }

        const Item* item = projectile.getItem();

        // 检查是否是烟花火箭
        if (item == Items::FIREWORK_ROCKET) {
            // 创建烟花火箭实体
            auto firework = std::make_unique<entity::FireworkRocketEntity>(EntityInstanceId(0));
            firework->setTypeId(entity::EntityTypeKeys::FIREWORK_ROCKET);
            firework->setWorld(&world);
            firework->setPosition(shooter.x(), shooter.y() + shooter.eyeHeight() - 0.15f, shooter.z());
            firework->setShooter(&shooter);
            firework->setShotFromCrossbow(true);   // 标记为从弩射出
            firework->setFireworkItem(projectile); // 设置烟花物品（包含爆炸效果数据）
            firework->shootFrom(shooter, shooter.pitch(), shooter.yaw(), projectileAngles[i], velocity, inaccuracy);

            world.spawnEntity(std::move(firework));

            // 消耗耐久度（烟花消耗3点），若物品损坏则触发 onEquippedItemBroken 回调
            LivingEntity::hurtAndBreak(crossbow, 3, &shooter, slot);
        } else {
            // 创建箭矢实体
            const ArrowItem* arrowItem = dynamic_cast<const ArrowItem*>(item);
            if (arrowItem != nullptr) {
                entity::AbstractArrowEntity* arrow = arrowItem->createArrow(world, projectile, shooter);
                if (arrow != nullptr) {
                    // 设置从弩射出
                    arrow->setShotFromCrossbow(true);

                    // 设置穿透等级
                    if (piercingLevel > 0) {
                        arrow->setPierceLevel(static_cast<u8>(piercingLevel));
                    }

                    // 玩家射出的箭必定暴击
                    if (player != nullptr) {
                        arrow->setCritical(true);
                    }

                    // 设置拾取状态
                    if (isCreative || projectileAngles[i] != 0.0f) {
                        arrow->setPickupStatus(entity::PickupStatus::CreativeOnly);
                    }

                    // 发射
                    arrow->shootFrom(
                        shooter, shooter.pitch(), shooter.yaw(), projectileAngles[i], velocity, inaccuracy);

                    // 生成实体（createArrow返回裸指针，需要包装为unique_ptr）
                    world.spawnEntity(std::unique_ptr<Entity>(arrow));
                }
            }

            // 消耗耐久度（箭矢消耗1点），若物品损坏则触发 onEquippedItemBroken 回调
            LivingEntity::hurtAndBreak(crossbow, 1, &shooter, slot);
        }
    }

    // 清除弹丸
    clearProjectiles(crossbow);
}

std::vector<ItemStack> CrossbowItem::_getChargedProjectiles(const ItemStack& stack)
{
    std::vector<ItemStack> projectiles;
    const nlohmann::json* tag = stack.getTag();
    if (tag == nullptr) {
        return projectiles;
    }

    auto it = tag->find("ChargedProjectiles");
    if (it == tag->end() || !it->is_array()) {
        return projectiles;
    }

    for (const auto& itemJson : *it) {
        if (itemJson.is_object()) {
            auto result = ItemStack::fromJson(itemJson);
            if (result.success()) {
                projectiles.push_back(result.value());
            }
        }
    }

    return projectiles;
}

void CrossbowItem::_addChargedProjectile(ItemStack& crossbow, const ItemStack& projectile)
{
    nlohmann::json& tag = crossbow.getOrCreateTag();

    if (!tag.contains("ChargedProjectiles")) {
        tag["ChargedProjectiles"] = nlohmann::json::array();
    }

    // 序列化弹丸到NBT
    nlohmann::json projectileJson = projectile.toJson();
    tag["ChargedProjectiles"].push_back(projectileJson);
}

void CrossbowItem::clearProjectiles(ItemStack& stack)
{
    nlohmann::json* tag = stack.getTag();
    if (tag != nullptr && tag->contains("ChargedProjectiles")) {
        (*tag)["ChargedProjectiles"] = nlohmann::json::array();
    }
}

bool CrossbowItem::hasChargedProjectile(const ItemStack& stack, const Item* item)
{
    // 直接遍历NBT，避免构建完整的vector
    const nlohmann::json* tag = stack.getTag();
    if (tag == nullptr) {
        return false;
    }

    auto it = tag->find("ChargedProjectiles");
    if (it == tag->end() || !it->is_array()) {
        return false;
    }

    for (const auto& itemJson : *it) {
        if (itemJson.is_object()) {
            auto result = ItemStack::fromJson(itemJson);
            if (result.success() && result.value().getItem() == item) {
                return true;
            }
        }
    }
    return false;
}

i32 CrossbowItem::_getMultishotLevel(const ItemStack& stack)
{
    return enchant::EnchantmentHelper::getEnchantmentLevel(stack, &enchant::AllEnchantments::MULTISHOT);
}

i32 CrossbowItem::_getPiercingLevel(const ItemStack& stack)
{
    return enchant::EnchantmentHelper::getEnchantmentLevel(stack, &enchant::AllEnchantments::PIERCING);
}

} // namespace item
} // namespace mc
