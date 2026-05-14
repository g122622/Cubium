#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/blockentity/CraftingTableEntity.hpp"
#include "world/blockentity/interactive/DispenserBlockEntity.hpp"
#include "world/blockentity/interactive/DropperBlockEntity.hpp"
#include "world/blockentity/interactive/EnchantingTableEntity.hpp"
#include "world/blockentity/interactive/PistonBlockEntity.hpp"
#include "world/blockentity/interactive/SignEntity.hpp"
#include "world/blockentity/processing/BeaconEntity.hpp"
#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "world/blockentity/processing/ConduitEntity.hpp"
#include "world/blockentity/processing/FurnaceEntity.hpp"
#include "world/blockentity/processing/SmokerEntity.hpp"
#include "world/blockentity/redstone/ComparatorEntity.hpp"
#include "world/blockentity/redstone/DaylightDetectorEntity.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/blockentity/storage/TrappedChestEntity.hpp"
#include "world/blockentity/transport/HopperEntity.hpp"

namespace mc {
namespace blockentity {

BlockEntityRegistry& BlockEntityRegistry::instance()
{
    static BlockEntityRegistry instance;
    return instance;
}

void BlockEntityRegistry::registerType(BlockEntityType type, Factory factory)
{
    MC_ASSERT(factory && "Factory cannot be null");
    m_factories[type] = std::move(factory);
}

void BlockEntityRegistry::registerBuiltinTypes()
{
    // 注册工作台方块实体
    registerType(
        BlockEntityType::CraftingTable, [](const BlockPos& pos) { return std::make_unique<CraftingTableEntity>(pos); });

    // 注册活塞方块实体
    registerType(BlockEntityType::Piston, [](const BlockPos& pos) { return std::make_unique<PistonBlockEntity>(pos); });

    // 注册箱子方块实体
    registerType(BlockEntityType::Chest, [](const BlockPos& pos) { return std::make_unique<ChestEntity>(pos); });

    // 注册陷阱箱方块实体
    registerType(
        BlockEntityType::TrappedChest, [](const BlockPos& pos) { return std::make_unique<TrappedChestEntity>(pos); });

    // 注册漏斗方块实体
    registerType(BlockEntityType::Hopper, [](const BlockPos& pos) { return std::make_unique<HopperEntity>(pos); });

    // 注册发射器方块实体
    registerType(BlockEntityType::Dispenser,
        [](const BlockPos& pos) { return std::make_unique<DispenserBlockEntity>(BlockEntityType::Dispenser, pos); });

    // 注册投掷器方块实体
    registerType(
        BlockEntityType::Dropper, [](const BlockPos& pos) { return std::make_unique<DropperBlockEntity>(pos); });

    // 注册熔炉方块实体
    registerType(BlockEntityType::Furnace, [](const BlockPos& pos) { return std::make_unique<FurnaceEntity>(pos); });

    // 注册高炉方块实体
    registerType(
        BlockEntityType::BlastFurnace, [](const BlockPos& pos) { return std::make_unique<BlastFurnaceEntity>(pos); });

    // 注册烟熏炉方块实体
    registerType(BlockEntityType::Smoker, [](const BlockPos& pos) { return std::make_unique<SmokerEntity>(pos); });

    // 注册附魔台方块实体
    registerType(BlockEntityType::EnchantingTable,
        [](const BlockPos& pos) { return std::make_unique<EnchantingTableEntity>(pos); });

    // 注册比较器方块实体
    registerType(
        BlockEntityType::Comparator, [](const BlockPos& pos) { return std::make_unique<ComparatorEntity>(pos); });

    // 注册日光探测器方块实体
    registerType(BlockEntityType::DaylightDetector,
        [](const BlockPos& pos) { return std::make_unique<DaylightDetectorEntity>(pos); });

    // 注册信标方块实体
    registerType(BlockEntityType::Beacon, [](const BlockPos& pos) { return std::make_unique<BeaconEntity>(pos); });

    // 注册潮涌核心方块实体
    registerType(BlockEntityType::Conduit, [](const BlockPos& pos) { return std::make_unique<ConduitEntity>(pos); });

    // 注册告示牌方块实体
    registerType(BlockEntityType::Sign, [](const BlockPos& pos) { return std::make_unique<SignEntity>(pos); });
}

std::unique_ptr<BlockEntity> BlockEntityRegistry::create(BlockEntityType type, const BlockPos& pos) const
{
    auto it = m_factories.find(type);
    if (it == m_factories.end()) {
        return nullptr;
    }
    return it->second(pos);
}

std::unique_ptr<BlockEntity> BlockEntityRegistry::createFromJson(const nlohmann::json& data) const
{
    // 解析方块实体类型ID
    if (!data.contains("id") || !data["id"].is_string()) {
        return nullptr;
    }

    ResourceLocation id(data["id"].get<std::string>());
    BlockEntityType type = blockEntityTypeFromId(id);

    if (type == BlockEntityType::Unknown) {
        return nullptr;
    }

    // 解析位置
    BlockPos pos(0, 0, 0);
    if (data.contains("x") && data["x"].is_number()) {
        pos.x = data["x"].get<i32>();
    }
    if (data.contains("y") && data["y"].is_number()) {
        pos.y = data["y"].get<i32>();
    }
    if (data.contains("z") && data["z"].is_number()) {
        pos.z = data["z"].get<i32>();
    }

    // 创建方块实体
    auto entity = create(type, pos);
    if (!entity) {
        return nullptr;
    }

    // 加载数据
    if (!entity->load(data)) {
        return nullptr;
    }

    return entity;
}

bool BlockEntityRegistry::hasType(BlockEntityType type) const
{
    return m_factories.find(type) != m_factories.end();
}

} // namespace blockentity
} // namespace mc
