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

#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/blockentity/CraftingTableEntity.hpp"
#include "world/blockentity/interactive/BannerEntity.hpp"
#include "world/blockentity/interactive/BeehiveBlockEntity.hpp"
#include "world/blockentity/interactive/BellBlockEntity.hpp"
#include "world/blockentity/interactive/BrushableBlockEntity.hpp"
#include "world/blockentity/interactive/CopperGolemStatueBlockEntity.hpp"
#include "world/blockentity/interactive/DecoratedPotBlockEntity.hpp"
#include "world/blockentity/interactive/DispenserBlockEntity.hpp"
#include "world/blockentity/interactive/DropperBlockEntity.hpp"
#include "world/blockentity/interactive/EnchantingTableEntity.hpp"
#include "world/blockentity/interactive/EndGatewayEntity.hpp"
#include "world/blockentity/interactive/JukeboxEntity.hpp"
#include "world/blockentity/interactive/LecternEntity.hpp"
#include "world/blockentity/interactive/PistonBlockEntity.hpp"
#include "world/blockentity/interactive/ShelfBlockEntity.hpp"
#include "world/blockentity/interactive/SignEntity.hpp"
#include "world/blockentity/processing/BeaconEntity.hpp"
#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "world/blockentity/processing/BrewingStandEntity.hpp"
#include "world/blockentity/processing/CampfireBlockEntity.hpp"
#include "world/blockentity/processing/ConduitEntity.hpp"
#include "world/blockentity/processing/FurnaceEntity.hpp"
#include "world/blockentity/processing/SmokerEntity.hpp"
#include "world/blockentity/redstone/CommandBlockEntity.hpp"
#include "world/blockentity/redstone/ComparatorEntity.hpp"
#include "world/blockentity/redstone/DaylightDetectorEntity.hpp"
#include "world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include "world/blockentity/storage/BarrelEntity.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/blockentity/storage/EnderChestEntity.hpp"
#include "world/blockentity/storage/ShulkerBoxEntity.hpp"
#include "world/blockentity/storage/TrappedChestEntity.hpp"
#include "world/blockentity/transport/HopperEntity.hpp"
#include "world/blockentity/trial/CrafterBlockEntity.hpp"
#include "world/blockentity/trial/TrialSpawnerBlockEntity.hpp"
#include "world/blockentity/trial/VaultBlockEntity.hpp"
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

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

    // 注册营火方块实体
    registerType(BlockEntityType::Campfire,
        [](const BlockPos& pos) { return std::make_unique<blockentity::CampfireBlockEntity>(pos); });

    // 注册末地折跃门方块实体
    registerType(BlockEntityType::EndGateway,
        [](const BlockPos& pos) { return std::make_unique<blockentity::EndGatewayEntity>(pos); });

    // 注册命令方块实体
    registerType(BlockEntityType::CommandBlock,
        [](const BlockPos& pos) { return std::make_unique<blockentity::CommandBlockEntity>(pos); });

    // 注册酿造台方块实体
    registerType(BlockEntityType::BrewingStand,
        [](const BlockPos& pos) { return std::make_unique<blockentity::BrewingStandEntity>(pos); });

    // 注册木桶方块实体
    registerType(BlockEntityType::Barrel, [](const BlockPos& pos) { return std::make_unique<BarrelEntity>(pos); });

    // 注册末影箱方块实体
    registerType(
        BlockEntityType::EnderChest, [](const BlockPos& pos) { return std::make_unique<EnderChestEntity>(pos); });

    // 注册潜影盒方块实体
    registerType(
        BlockEntityType::ShulkerBox, [](const BlockPos& pos) { return std::make_unique<ShulkerBoxEntity>(pos); });

    // 注册讲台方块实体
    registerType(BlockEntityType::Lectern, [](const BlockPos& pos) { return std::make_unique<LecternEntity>(pos); });

    // 注册唱片机方块实体
    registerType(BlockEntityType::Jukebox, [](const BlockPos& pos) { return std::make_unique<JukeboxEntity>(pos); });

    // 注册钟方块实体
    registerType(BlockEntityType::Bell, [](const BlockPos& pos) { return std::make_unique<BellBlockEntity>(pos); });

    // 注册书架方块实体
    registerType(BlockEntityType::Shelf, [](const BlockPos& pos) { return std::make_unique<ShelfBlockEntity>(pos); });

    // 注册旗帜方块实体
    registerType(BlockEntityType::Banner, [](const BlockPos& pos) { return std::make_unique<BannerEntity>(pos); });

    // 注册蜂巢方块实体
    registerType(BlockEntityType::Beehive,
        [](const BlockPos& pos) { return std::make_unique<blockentity::BeehiveBlockEntity>(pos); });

    // 注册试炼密室方块实体
    registerType(BlockEntityType::TrialSpawner,
        [](const BlockPos& pos) { return std::make_unique<TrialSpawnerBlockEntity>(pos); });
    registerType(BlockEntityType::Vault, [](const BlockPos& pos) { return std::make_unique<VaultBlockEntity>(pos); });
    registerType(
        BlockEntityType::Crafter, [](const BlockPos& pos) { return std::make_unique<CrafterBlockEntity>(pos); });

    // 注册刷怪笼方块实体
    registerType(BlockEntityType::MobSpawner,
        [](const BlockPos& pos) { return std::make_unique<blockentity::MobSpawnerBlockEntity>(pos); });

    // 注册幽匿感测体方块实体
    registerType(BlockEntityType::SculkSensor,
        [](const BlockPos& pos) { return std::make_unique<blockentity::SculkSensorBlockEntity>(pos); });

    // 注册幽匿尖啸体方块实体
    registerType(BlockEntityType::SculkShrieker,
        [](const BlockPos& pos) { return std::make_unique<blockentity::SculkShriekerBlockEntity>(pos); });

    // 注册饰纹陶罐方块实体
    registerType(BlockEntityType::DecoratedPot,
        [](const BlockPos& pos) { return std::make_unique<blockentity::DecoratedPotBlockEntity>(pos); });

    // 注册铜傀儡雕像方块实体
    registerType(BlockEntityType::CopperGolemStatue,
        [](const BlockPos& pos) { return std::make_unique<blockentity::CopperGolemStatueBlockEntity>(pos); });

    // 注册可刷方块实体（可疑沙/可疑沙砾）
    registerType(BlockEntityType::BrushableBlock,
        [](const BlockPos& pos) { return std::make_unique<BrushableBlockEntity>(pos); });
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
