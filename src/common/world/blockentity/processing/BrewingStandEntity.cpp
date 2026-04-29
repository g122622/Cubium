#include "world/blockentity/processing/BrewingStandEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "item/core/ItemStack.hpp"
#include "item/Items.hpp"
#include "item/potion/PotionBrewing.hpp"
#include "item/potion/PotionUtils.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/property/Properties.hpp"
#include <algorithm>

namespace mc {
namespace blockentity {

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

    return potion::PotionBrewing::isPotionItem(stack);
}

void BrewingStandEntity::tick(IWorld& world) {
    bool brewing = isBrewing();
    if (brewing != m_lastBrewing) {
        updateBlockState(world);
        m_lastBrewing = brewing;
    }

    // 获取当前材料
    const ItemStack& ingredientStack = m_inventory.getItem(INGREDIENT_SLOT);

    // MC Java: 检测材料变化，重置酿造时间
    // 参考: BrewingStandTileEntity.java lines 113-116
    // 如果材料变化，重置酿造时间
    if (!ingredientStack.isSameItem(m_ingredientCache)) {
        m_ingredientCache = ingredientStack.copy();
        m_brewTime = 0;
        setChanged();
    }

    if (!canBrew()) {
        m_brewTime = 0;
        return;
    }

    if (m_fuel <= 0) {
        ItemStack fuelStack = m_inventory.getItem(FUEL_SLOT);
        if (!fuelStack.isEmpty() && fuelStack.getItem() == Items::BLAZE_POWDER) {
            fuelStack.shrink(1);
            m_fuel += FUEL_PER_BREW;
            m_inventory.setItem(FUEL_SLOT, fuelStack);
            setChanged();
        }
    }

    if (m_fuel <= 0) {
        m_brewTime = 0;
        return;
    }

    if (m_brewTime > 0) {
        --m_brewTime;
        if (m_brewTime == 0) {
            doBrew(world);
            consumeFuel();
            updateBlockState(world);
            setChanged();
        }
    } else {
        m_brewTime = 400;
        setChanged();
    }
}

bool BrewingStandEntity::canBrew() const {
    const ItemStack& ingredientStack = m_inventory.getItem(INGREDIENT_SLOT);
    if (ingredientStack.isEmpty()) {
        return false;
    }

    if (!potion::PotionBrewing::isReagent(ingredientStack)) {
        return false;
    }

    for (i32 i = 0; i < BOTTLE_SLOTS; ++i) {
        const ItemStack& bottleStack = m_inventory.getItem(i);
        if (bottleStack.isEmpty()) {
            continue;
        }

        if (!potion::PotionBrewing::isPotionItem(bottleStack)) {
            continue;
        }

        if (potion::PotionBrewing::canBrew(bottleStack, ingredientStack)) {
            return true;
        }
    }

    return false;
}

void BrewingStandEntity::doBrew(IWorld& world) {
    MC_UNUSED(world);

    ItemStack ingredientStack = m_inventory.getItem(INGREDIENT_SLOT);
    if (ingredientStack.isEmpty()) {
        return;
    }

    for (i32 i = 0; i < BOTTLE_SLOTS; ++i) {
        ItemStack bottleStack = m_inventory.getItem(i);
        if (bottleStack.isEmpty()) {
            continue;
        }

        if (!potion::PotionBrewing::isPotionItem(bottleStack)) {
            continue;
        }

        if (potion::PotionBrewing::canBrew(bottleStack, ingredientStack)) {
            ItemStack result = potion::PotionBrewing::brew(bottleStack, ingredientStack);
            m_inventory.setItem(i, result);
        }
    }

    ingredientStack.shrink(1);
    m_inventory.setItem(INGREDIENT_SLOT, ingredientStack);
}

void BrewingStandEntity::consumeFuel() {
    if (m_fuel > 0) {
        --m_fuel;
        setChanged();
    }
}

void BrewingStandEntity::updateBlockState(IWorld& world) {
    const BlockState* state = world.getBlockState(getPos());
    if (state == nullptr) {
        return;
    }

    if (!state->hasProperty(BlockStateProperties::HAS_BOTTLE_0()) ||
        !state->hasProperty(BlockStateProperties::HAS_BOTTLE_1()) ||
        !state->hasProperty(BlockStateProperties::HAS_BOTTLE_2())) {
        return;
    }

    const BlockState& updated = state
        ->with(BlockStateProperties::HAS_BOTTLE_0(), hasBottle(0))
        .with(BlockStateProperties::HAS_BOTTLE_1(), hasBottle(1))
        .with(BlockStateProperties::HAS_BOTTLE_2(), hasBottle(2));

    world.setBlockState(getPos(), &updated, 3);
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

    if (data.contains("items")) {
        m_inventory.load(data["items"]);
    }

    return true;
}

void BrewingStandEntity::save(nlohmann::json& data) const {
    ContainerBlockEntity::save(data);

    data["brew_time"] = m_brewTime;
    data["fuel"] = m_fuel;

    nlohmann::json itemsJson;
    m_inventory.save(itemsJson);
    data["items"] = itemsJson;
}

std::unique_ptr<BlockEntity> BrewingStandEntity::clone() const {
    auto clone = std::make_unique<BrewingStandEntity>(m_pos);
    clone->m_brewTime = m_brewTime;
    clone->m_fuel = m_fuel;
    clone->m_lastBrewing = m_lastBrewing;
    for (i32 slot = 0; slot < TOTAL_SLOTS; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            clone->m_inventory.setItem(slot, stack.copy());
        }
    }
    return clone;
}

} // namespace blockentity
} // namespace mc