#include "world/blockentity/processing/BrewingStandEntity.hpp"
#include "world/IWorld.hpp"
#include "item/core/ItemStack.hpp"
#include "item/Items.hpp"
#include "item/potion/PotionBrewing.hpp"
#include "item/potion/PotionUtils.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== BrewingStandEntity 实现 ==========

BrewingStandEntity::BrewingStandEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::BrewingStand, pos)
    , m_inventory(TOTAL_SLOTS) {
}

BrewingStandEntity::~BrewingStandEntity() = default;

void BrewingStandEntity::setFuelLevel(i32 fuel) {
    m_fuel = std::max(0, std::min(fuel, FUEL_PER_BREW * 64));
    setChanged();
}

bool BrewingStandEntity::hasBottle(i32 slot) const {
    if (slot < 0 || slot >= BOTTLE_SLOTS) {
        return false;
    }
    const ItemStack& stack = m_inventory.getItem(slot);
    if (stack.isEmpty()) {
        return false;
    }
    // 检查是否为药水瓶（普通药水、喷溅药水、滞留药水）
    // TODO: 添加 Items::POTION, Items::SPLASH_POTION, Items::LINGERING_POTION 检查
    return true;
}

void BrewingStandEntity::tick(IWorld& world) {
    // 更新方块状态
    bool brewing = isBrewing();
    if (brewing != m_lastBrewing) {
        updateBlockState(world);
        m_lastBrewing = brewing;
    }

    // 检查是否可以酿造
    if (!canBrew()) {
        m_brewTime = 0;
        return;
    }

    // 检查燃料
    if (m_fuel <= 0) {
        // 尝试消耗烈焰粉
        ItemStack& fuelStack = m_inventory.getItem(FUEL_SLOT);
        if (!fuelStack.isEmpty() && fuelStack.getItem() != nullptr) {
            // TODO: 检查是否为烈焰粉 (Items::BLAZE_POWDER)
            // 暂时检查物品名称
            if (fuelStack.getItem()->getTranslationKey() == "item.minecraft.blaze_powder") {
                fuelStack.shrink(1);
                m_fuel += FUEL_PER_BREW;
                setChanged();
            }
        }
    }

    if (m_fuel <= 0) {
        m_brewTime = 0;
        return;
    }

    // 酿造进度
    if (m_brewTime > 0) {
        m_brewTime--;
        if (m_brewTime == 0) {
            // 酿造完成
            doBrew(world);
            consumeFuel();
            setChanged();
        }
    } else {
        // 开始酿造
        m_brewTime = 400; // 20秒
        setChanged();
    }
}

bool BrewingStandEntity::canBrew() const {
    // 检查材料槽位
    const ItemStack& ingredientStack = m_inventory.getItem(INGREDIENT_SLOT);
    if (ingredientStack.isEmpty()) {
        return false;
    }

    // 检查材料是否为有效的酿造材料
    if (!potion::PotionBrewing::isReagent(ingredientStack)) {
        return false;
    }

    // 检查至少有一个药水瓶可以被酿造
    for (i32 i = 0; i < BOTTLE_SLOTS; ++i) {
        const ItemStack& bottleStack = m_inventory.getItem(i);
        if (bottleStack.isEmpty()) {
            continue;
        }

        // 检查是否为药水容器
        if (!potion::PotionBrewing::isPotionItem(bottleStack)) {
            continue;
        }

        // 检查是否可以酿造
        if (potion::PotionBrewing::canBrew(bottleStack, ingredientStack)) {
            return true;
        }
    }

    return false;
}

void BrewingStandEntity::doBrew(IWorld& world) {
    MC_UNUSED(world);

    ItemStack& ingredientStack = m_inventory.getItem(INGREDIENT_SLOT);
    if (ingredientStack.isEmpty()) {
        return;
    }

    // 遍历所有药水瓶槽位
    for (i32 i = 0; i < BOTTLE_SLOTS; ++i) {
        ItemStack& bottleStack = m_inventory.getItem(i);
        if (bottleStack.isEmpty()) {
            continue;
        }

        // 检查是否为药水容器
        if (!potion::PotionBrewing::isPotionItem(bottleStack)) {
            continue;
        }

        // 检查是否可以酿造
        if (potion::PotionBrewing::canBrew(bottleStack, ingredientStack)) {
            // 执行酿造
            ItemStack result = potion::PotionBrewing::brew(bottleStack, ingredientStack);
            m_inventory.setItem(i, result);
        }
    }

    // 消耗材料
    ingredientStack.shrink(1);
}

void BrewingStandEntity::consumeFuel() {
    if (m_fuel > 0) {
        m_fuel--;
        setChanged();
    }
}

void BrewingStandEntity::updateBlockState(IWorld& world) {
    // TODO: 更新方块的 HAS_BOTTLE_0/1/2 属性
    MC_UNUSED(world);
}

bool BrewingStandEntity::load(const nlohmann::json& data) {
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    if (data.contains("brew_time")) {
        m_brewTime = data["brew_time"].get<i32>();
    }

    if (data.contains("fuel")) {
        m_fuel = data["fuel"].get<i32>();
    }

    // 加载物品
    if (data.contains("items")) {
        m_inventory.load(data["items"]);
    }

    return true;
}

void BrewingStandEntity::save(nlohmann::json& data) const {
    ContainerBlockEntity::save(data);

    data["brew_time"] = m_brewTime;
    data["fuel"] = m_fuel;

    // 保存物品
    nlohmann::json itemsJson;
    m_inventory.save(itemsJson);
    data["items"] = itemsJson;
}

std::unique_ptr<BlockEntity> BrewingStandEntity::clone() const {
    auto clone = std::make_unique<BrewingStandEntity>(m_pos);
    clone->m_brewTime = m_brewTime;
    clone->m_fuel = m_fuel;
    clone->m_lastBrewing = m_lastBrewing;
    // TODO: 复制物品
    return clone;
}

} // namespace blockentity
} // namespace mc
