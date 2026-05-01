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
#include "../../../world/IWorld.hpp"
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

UseAction CrossbowItem::getUseAction(const ItemStack& stack) const {
    (void)stack;
    return UseAction::Crossbow;
}

ItemActionResult CrossbowItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    (void)world;

    ItemStack crossbowStack = player.getHeldItem(hand);

    // 如果已装填，则发射
    if (isCharged(crossbowStack)) {
        // TODO: 实现烟花火箭检测
        f32 velocity = ARROW_VELOCITY;
        fireProjectiles(world, player, hand, crossbowStack, velocity, 1.0f);
        setCharged(crossbowStack, false);
        return ItemActionResult::success(crossbowStack);
    }

    // 检查是否有弹药
    // TODO: 需要实现 Player::findAmmo 或使用 BowItem 的方法
    // 暂时检查副手和主手
    ItemStack offhand = player.getHeldItem(Hand::OffHand);
    ItemStack mainhand = player.getHeldItem(Hand::MainHand);

    auto ammoPredicate = getAmmoPredicate();
    bool hasAmmo = ammoPredicate(offhand) || ammoPredicate(mainhand);

    if (!hasAmmo && !player.isCreative()) {
        return ItemActionResult::fail(crossbowStack);
    }

    // 开始装填
    if (!isCharged(crossbowStack)) {
        m_loadingStart = false;
        m_loadingMiddle = false;
        player.setActiveHand(hand);
    }

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
        // TODO: 检查弹药
        // hasAmmo(entity, stack)
        setCharged(stack, true);
        // TODO: 播放装填完成音效
    }

    (void)world;
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
        // TODO: 检查是否为烟花火箭
        // return item == Items::FIREWORK_ROCKET;
        return false;
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

bool CrossbowItem::hasAmmo(LivingEntity& /*entity*/, ItemStack& /*crossbow*/) {
    // TODO: 实现 findAmmo 和弹药检查
    // 暂时返回 true（假设有弹药）
    return true;
}

void CrossbowItem::fireProjectiles(
    IWorld& world,
    LivingEntity& shooter,
    Hand hand,
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

    for (size_t i = 0; i < projectiles.size() && i < projectileAngles.size(); ++i) {
        const ItemStack& projectile = projectiles[i];
        if (projectile.isEmpty()) {
            continue;
        }

        // 创建箭矢实体
        const ArrowItem* arrowItem = dynamic_cast<const ArrowItem*>(projectile.getItem());
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

                // 生成实体
                world.spawnEntity(std::unique_ptr<Entity>(arrow));
            }
        }

        // 消耗耐久度
        crossbow.attemptDamageItem(1);

        // TODO: 播放射击音效
    }

    // 清除弹丸
    clearProjectiles(crossbow);

    (void)hand;
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

    // TODO: 从 NBT 反序列化 ItemStack

    return projectiles;
}

void CrossbowItem::addChargedProjectile(ItemStack& crossbow, const ItemStack& projectile) {
    nlohmann::json& tag = crossbow.getOrCreateTag();

    if (!tag.contains("ChargedProjectiles")) {
        tag["ChargedProjectiles"] = nlohmann::json::array();
    }

    // TODO: 序列化 projectile 到 NBT
    (void)projectile;
}

void CrossbowItem::clearProjectiles(ItemStack& stack) {
    nlohmann::json* tag = stack.getTag();
    if (tag != nullptr && tag->contains("ChargedProjectiles")) {
        (*tag)["ChargedProjectiles"] = nlohmann::json::array();
    }
}

i32 CrossbowItem::getMultishotLevel(const ItemStack& stack) {
    return enchant::EnchantmentHelper::getEnchantmentLevel(stack, &enchant::AllEnchantments::MULTISHOT);
}

i32 CrossbowItem::getPiercingLevel(const ItemStack& stack) {
    return enchant::EnchantmentHelper::getEnchantmentLevel(stack, &enchant::AllEnchantments::PIERCING);
}

} // namespace item
} // namespace mc
