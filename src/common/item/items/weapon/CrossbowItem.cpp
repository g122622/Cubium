#include "CrossbowItem.hpp"
#include "ArrowItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../Items.hpp"
#include "../../enchantment/EnchantmentHelper.hpp"
#include "../../enchantment/enchantments/AllEnchantments.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../entity/entities/projectile/OtherProjectiles.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../sound/SoundEvents.hpp"
#include <cmath>

namespace mc {
namespace item {

// ========== 常量 ==========
namespace {
    constexpr i32 BASE_CHARGE_TIME = 25;  // 基础装填时间 (tick)
    constexpr i32 CHARGE_TIME_REDUCTION_PER_LEVEL = 5;  // 快速装填每级减少时间
    constexpr f32 ARROW_VELOCITY = 3.15f;  // 箭矢速度
    constexpr f32 FIREWORK_VELOCITY = 1.6f;  // 烟花速度
}

// ========== 构造函数 ==========

CrossbowItem::CrossbowItem(const ItemProperties& properties)
    : Item(properties)
{
}

// ========== Item 接口重写 ==========

i32 CrossbowItem::getUseDuration(const ItemStack& stack) const {
    return getChargeTime(stack) + 3;
}

UseAction CrossbowItem::getUseAction(const ItemStack& /*stack*/) const {
    return UseAction::Crossbow;
}

ItemActionResult CrossbowItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {

    ItemStack crossbowStack = player.getHeldItem(hand);

    // 如果已装填，则发射
    if (isCharged(crossbowStack)) {
        // 检测烟花火箭
        f32 velocity = ARROW_VELOCITY;
        if (hasChargedProjectile(crossbowStack, Items::FIREWORK_ROCKET)) {
            velocity = FIREWORK_VELOCITY;
        }
        fireProjectiles(world, player, crossbowStack, velocity, 1.0f);
        setCharged(crossbowStack, false);
        return ItemActionResult::success(crossbowStack);
    }

    // 检查是否有弹药
    ItemStack ammo = findAmmo(player);
    if (ammo.isEmpty() && !player.isCreative()) {
        return ItemActionResult::fail(crossbowStack);
    }

    // 开始装填
    player.setActiveHand(hand);

    return ItemActionResult::success(crossbowStack);
}

void CrossbowItem::onPlayerStoppedUsing(
    ItemStack& stack,
    IWorld& world,
    LivingEntity& entity,
    i32 timeLeft)
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
        if (loadProjectiles(*player, stack)) {
            setCharged(stack, true);
            // 播放装填完成音效
            player->playSound(SoundEvents::ITEM_CROSSBOW_LOADING_END, 1.0f, 1.0f);
        }
    }
}

// ========== 弩特有方法 ==========

bool CrossbowItem::isCharged(const ItemStack& stack) {
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

void CrossbowItem::setCharged(ItemStack& stack, bool charged) {
    stack.getOrCreateTag()["Charged"] = charged;
}

i32 CrossbowItem::getChargeTime(const ItemStack& stack) {
    i32 quickChargeLevel = enchant::EnchantmentHelper::getEnchantmentLevel(
        stack, &enchant::AllEnchantments::QUICK_CHARGE);
    if (quickChargeLevel == 0) {
        return BASE_CHARGE_TIME;
    }
    return BASE_CHARGE_TIME - CHARGE_TIME_REDUCTION_PER_LEVEL * quickChargeLevel;
}

std::function<bool(const ItemStack&)> CrossbowItem::getAmmoPredicate() const {
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

std::function<bool(const ItemStack&)> CrossbowItem::getInventoryAmmoPredicate() const {
    return [](const ItemStack& stack) -> bool {
        if (stack.isEmpty()) {
            return false;
        }
        // 只接受箭矢
        return dynamic_cast<const ArrowItem*>(stack.getItem()) != nullptr;
    };
}

// ========== 私有方法 ==========

bool CrossbowItem::isAmmo(const ItemStack& stack) {
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

ItemStack CrossbowItem::findAmmo(Player& player) {
    // 先检查副手
    ItemStack offhand = player.getHeldItem(Hand::OffHand);
    if (isAmmo(offhand)) {
        return offhand;
    }

    // 再检查主手（弩本身在主手时跳过）
    ItemStack mainhand = player.getHeldItem(Hand::MainHand);
    if (isAmmo(mainhand)) {
        return mainhand;
    }

    // 检查背包
    PlayerInventory& inventory = player.inventory();
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack slot = inventory.getItem(i);
        if (isAmmo(slot)) {
            return slot;
        }
    }

    return ItemStack::EMPTY;
}

bool CrossbowItem::loadProjectiles(Player& player, ItemStack& crossbow) {
    i32 multishotLevel = getMultishotLevel(crossbow);
    i32 projectileCount = multishotLevel > 0 ? 3 : 1;
    bool isCreative = player.isCreative();

    ItemStack ammo = findAmmo(player);

    for (i32 i = 0; i < projectileCount; ++i) {
        ItemStack projectileToLoad;

        if (!ammo.isEmpty()) {
            if (isCreative) {
                // 创造模式：复制弹药
                projectileToLoad = ammo.copy();
            } else {
                // 生存模式：消耗弹药
                if (i == 0) {
                    projectileToLoad = ammo.split(1);
                } else {
                    // 多重射击：后续弹丸需要额外弹药
                    if (ammo.getCount() > i) {
                        projectileToLoad = ammo.split(1);
                    } else {
                        // 弹药不足，尝试找更多
                        ItemStack moreAmmo = findAmmo(player);
                        if (!moreAmmo.isEmpty()) {
                            projectileToLoad = moreAmmo.split(1);
                            ammo = moreAmmo;
                        }
                    }
                }
            }
        } else if (isCreative) {
            // 创造模式无弹药时使用默认箭矢
            projectileToLoad = ItemStack(Items::ARROW, 1);
        }

        if (!projectileToLoad.isEmpty()) {
            addChargedProjectile(crossbow, projectileToLoad);
        } else {
            // 弹药不足
            return false;
        }
    }

    return true;
}

void CrossbowItem::fireProjectiles(
    IWorld& world,
    LivingEntity& shooter,
    ItemStack& crossbow,
    f32 velocity,
    f32 inaccuracy)
{
    std::vector<ItemStack> projectiles = getChargedProjectiles(crossbow);

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

    i32 piercingLevel = getPiercingLevel(crossbow);

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
            auto firework = std::make_unique<entity::FireworkRocketEntity>(
                LegacyEntityType::FireworkRocket, EntityId(0));
            firework->setWorld(&world);
            firework->setPosition(shooter.x(), shooter.y() + shooter.eyeHeight() - 0.15f, shooter.z());
            firework->setShooter(&shooter);
            firework->shootFrom(shooter, shooter.pitch(), shooter.yaw(), projectileAngles[i], velocity, inaccuracy);

            world.spawnEntity(std::move(firework));

            // 消耗耐久度（烟花消耗3点）
            crossbow.attemptDamageItem(3);
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
                    arrow->shootFrom(shooter, shooter.pitch(), shooter.yaw(), projectileAngles[i], velocity, inaccuracy);

                    // 生成实体（createArrow返回裸指针，需要包装为unique_ptr）
                    world.spawnEntity(std::unique_ptr<Entity>(arrow));
                }
            }

            // 消耗耐久度（箭矢消耗1点）
            crossbow.attemptDamageItem(1);
        }
    }

    // 清除弹丸
    clearProjectiles(crossbow);
}

std::vector<ItemStack> CrossbowItem::getChargedProjectiles(const ItemStack& stack) {
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

void CrossbowItem::addChargedProjectile(ItemStack& crossbow, const ItemStack& projectile) {
    nlohmann::json& tag = crossbow.getOrCreateTag();

    if (!tag.contains("ChargedProjectiles")) {
        tag["ChargedProjectiles"] = nlohmann::json::array();
    }

    // 序列化弹丸到NBT
    nlohmann::json projectileJson = projectile.toJson();
    tag["ChargedProjectiles"].push_back(projectileJson);
}

void CrossbowItem::clearProjectiles(ItemStack& stack) {
    nlohmann::json* tag = stack.getTag();
    if (tag != nullptr && tag->contains("ChargedProjectiles")) {
        (*tag)["ChargedProjectiles"] = nlohmann::json::array();
    }
}

bool CrossbowItem::hasChargedProjectile(const ItemStack& stack, const Item* item) {
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

i32 CrossbowItem::getMultishotLevel(const ItemStack& stack) {
    return enchant::EnchantmentHelper::getEnchantmentLevel(stack, &enchant::AllEnchantments::MULTISHOT);
}

i32 CrossbowItem::getPiercingLevel(const ItemStack& stack) {
    return enchant::EnchantmentHelper::getEnchantmentLevel(stack, &enchant::AllEnchantments::PIERCING);
}

} // namespace item
} // namespace mc
