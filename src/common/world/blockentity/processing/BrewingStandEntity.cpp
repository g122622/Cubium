#include "world/blockentity/processing/BrewingStandEntity.hpp"
#include "world/IWorld.hpp"
#include "item/ItemStack.hpp"
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
    // TODO: 检查槽位是否有药水瓶物品
    return !m_inventory.getItem(slot).isEmpty();
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
        // TODO: 检查燃料槽位是否有烈焰粉
        // 如果有，消耗并设置燃料
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
    // TODO: 实现完整的酿造检查逻辑
    // 1. 检查材料槽位是否有有效的酿造材料
    // 2. 检查药水瓶槽位是否有药水瓶
    // 3. 检查材料是否能与药水瓶反应
    return false; // 暂时返回false
}

void BrewingStandEntity::doBrew(IWorld& world) {
    MC_UNUSED(world);

    // TODO: 实现酿造逻辑
    // 1. 遍历所有药水瓶槽位
    // 2. 对每个药水瓶应用酿造配方
    // 3. 消耗材料
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
