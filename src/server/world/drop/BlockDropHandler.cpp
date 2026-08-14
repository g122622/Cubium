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

#include "BlockDropHandler.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// BlockDropHandler
// ============================================================================

std::vector<ItemStack> BlockDropHandler::generateDrops(IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    const Player* player,
    const ItemStack* tool,
    const loot::LootTableManager& lootTableManager)
{
    std::vector<ItemStack> drops;

    // 检查方块是否有掉落表
    const Block& block = state.owner();
    const loot::LootTable* lootTable = block.getLootTable(lootTableManager);
    spdlog::info("BlockDropHandler::generateDrops block='{}' lootTableId='{}' lootTableFound={} pos={} tool={} x{}",
        block.blockLocation().toString(),
        block.getLootTableId(),
        lootTable != nullptr,
        pos.toString(),
        (tool && tool->getItem()) ? tool->getItem()->itemLocation().toString() : "null",
        tool ? tool->getCount() : 0);

    if (lootTable) {
        // 使用掉落表生成掉落
        math::Random random(static_cast<u64>(world.seed() ^ static_cast<u64>(pos.x ^ pos.z)));

        auto context = buildLootContext(world, pos, state, player, tool, random);
        if (context) {
            spdlog::info(
                "BlockDropHandler::generateDrops built loot context for block='{}': hasBlockState={} hasBlockPos={} "
                "hasTool={} hasThisEntity={} hasExplosionRadius={} luck={} lootingModifier={}",
                block.blockLocation().toString(),
                context->has(loot::LootParams::BLOCK_STATE),
                context->has(loot::LootParams::BLOCK_POS),
                context->has(loot::LootParams::TOOL),
                context->has(loot::LootParams::THIS_ENTITY),
                context->has(loot::LootParams::EXPLOSION_RADIUS),
                context->getLuck(),
                context->getLootingModifier());
            // 设置掉落表解析器
            context->setLootTableResolver([&lootTableManager](const std::string& id) -> const loot::LootTable* {
                return lootTableManager.getTable(id);
            });
            context->setPredicateResolver([&lootTableManager](const std::string& id) -> const loot::LootCondition* {
                return lootTableManager.getPredicate(id);
            });

            drops = lootTable->generate(*context);
            spdlog::info("BlockDropHandler::generateDrops loot table '{}' returned {} drops for block '{}'",
                block.getLootTableId(),
                drops.size(),
                block.blockLocation().toString());
        }
    } else {
        // 使用默认掉落逻辑
        spdlog::warn(
            "Block {} at {} has no loot table, using default drops", block.blockLocation().toString(), pos.toString());
        drops = getDefaultDrops(state);
    }

    return drops;
}

std::vector<EntityInstanceId> BlockDropHandler::spawnDrops(server::ServerWorld& world,
    const BlockPos& pos,
    const std::vector<ItemStack>& drops,
    const std::string& throwerUuid)
{
    std::vector<EntityInstanceId> spawnedEntities;

    if (drops.empty()) {
        return spawnedEntities;
    }

    // 使用固定种子生成随机速度
    math::Random random(static_cast<u64>(pos.x ^ pos.z));

    // 使用 ItemDropHelper 统一生成物品实体
    return ItemDropHelper::spawnItemEntities(&world, pos, drops, random, throwerUuid);
}

std::vector<EntityInstanceId> BlockDropHandler::spawnDrops(EntityManager& entityManager,
    PhysicsEngine* physicsEngine,
    const BlockPos& pos,
    const std::vector<ItemStack>& drops,
    const std::string& throwerUuid)
{
    std::vector<EntityInstanceId> spawnedEntities;

    if (drops.empty()) {
        return spawnedEntities;
    }

    // 在方块中心位置生成物品实体
    f32 centerX = static_cast<f32>(pos.x) + 0.5f;
    f32 centerY = static_cast<f32>(pos.y) + 0.5f;
    f32 centerZ = static_cast<f32>(pos.z) + 0.5f;

    // ECS 迁移：实体构造需要 registry 句柄，此重载无 IWorld，经 EntityManager.registry() 取
    // EntityManager 持有 ServerWorld 的 m_entityRegistry 引用（任务#24）
    auto& registry = entityManager.registry();

    // 使用固定种子生成随机速度
    math::Random random(static_cast<u64>(pos.x ^ pos.z));

    for (const auto& stack : drops) {
        if (stack.isEmpty()) {
            continue;
        }

        auto itemEntity = std::make_unique<ItemEntity>(0, stack, centerX, centerY, centerZ, registry);

        // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
        itemEntity->setTypeId(entity::EntityTypeKeys::ITEM);

        // 使用 ItemDropHelper 统一随机速度计算
        Vector3 velocity = ItemDropHelper::getBlockDropVelocity(random);
        itemEntity->setVelocity(velocity.x, velocity.y, velocity.z);

        if (!throwerUuid.empty()) {
            itemEntity->setOwner(throwerUuid, throwerUuid);
        }

        itemEntity->setPickupDelay(ItemDropHelper::DEFAULT_PICKUP_DELAY);

        if (physicsEngine) {
            itemEntity->setPhysicsEngine(physicsEngine);
        }

        EntityInstanceId entityId = entityManager.addEntity(std::move(itemEntity));
        if (entityId != 0) {
            spawnedEntities.push_back(entityId);
        }
    }

    return spawnedEntities;
}

bool BlockDropHandler::canHarvestBlock(const BlockState& state, const Player* player, const ItemStack* tool)
{
    // 基岩等不可破坏方块
    if (state.hardness() < 0.0f) {
        return false;
    }

    // 创造模式总是可以采集
    if (player && player->gameMode() == GameMode::Creative) {
        return true;
    }

    // 检查方块是否需要特定工具
    if (!state.requiresTool()) {
        // 不需要工具，直接可以采集
        return true;
    }

    // 检查工具是否有效
    if (!tool || tool->isEmpty()) {
        // 需要工具但没有工具
        return false;
    }

    // 使用 ItemStack 的 canHarvestBlock 方法检查
    return tool->canHarvestBlock(state);
}

std::vector<ItemStack> BlockDropHandler::getDefaultDrops(const BlockState& state)
{
    // 默认掉落逻辑：无掉落
    // 子类或掉落表可以覆盖此行为
    (void)state;
    return {};
}

std::unique_ptr<loot::LootContext> BlockDropHandler::buildLootContext(IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    const Player* player,
    const ItemStack* tool,
    math::Random& random)
{
    auto contextBuilder =
        loot::LootContextBuilder(world).withRandom(random).withSeed(world.seed() ^ static_cast<u64>(pos.x ^ pos.z));

    // 设置方块状态和位置参数（方块掉落的必需参数）
    contextBuilder.withParameter(loot::LootParams::BLOCK_STATE, const_cast<BlockState*>(&state));
    contextBuilder.withParameter(loot::LootParams::BLOCK_POS, const_cast<BlockPos*>(&pos));

    // 设置工具参数
    if (tool && !tool->isEmpty()) {
        contextBuilder.withParameter(loot::LootParams::TOOL, const_cast<ItemStack*>(tool));

        // 设置时运等级
        i32 fortuneLevel = getFortuneLevel(tool);
        if (fortuneLevel > 0) {
            contextBuilder.withLootingModifier(fortuneLevel);
            contextBuilder.withOwnedValue(loot::LootParams::FORTUNE_LEVEL, fortuneLevel);
        }

        // 设置精准采集等级
        i32 silkTouchLevel = hasSilkTouch(tool) ? 1 : 0;
        if (silkTouchLevel > 0) {
            contextBuilder.withOwnedValue(loot::LootParams::SILK_TOUCH_LEVEL, silkTouchLevel);
        }
    }

    // 设置玩家参数
    if (player) {
        Entity* playerEntity = const_cast<Player*>(player);
        contextBuilder.withParameter(loot::LootParams::THIS_ENTITY, playerEntity);
    }

    // 使用方块掉落参数集构建上下文
    return contextBuilder.build(loot::LootParameterSets::block());
}

bool BlockDropHandler::hasSilkTouch(const ItemStack* tool)
{
    if (!tool || tool->isEmpty()) {
        return false;
    }

    return item::enchant::EnchantmentHelper::hasSilkTouch(*tool);
}

i32 BlockDropHandler::getFortuneLevel(const ItemStack* tool)
{
    if (!tool || tool->isEmpty()) {
        return 0;
    }

    return item::enchant::EnchantmentHelper::getFortuneLevel(*tool);
}

// ============================================================================
// 经验掉落
// ============================================================================

OreType BlockDropHandler::getOreType(const BlockState& state)
{
    const Block& block = state.owner();

    // 检查是否是各种矿石
    if (&block == VanillaBlocks::COAL_ORE) {
        return OreType::Coal;
    }
    if (&block == VanillaBlocks::DIAMOND_ORE) {
        return OreType::Diamond;
    }
    if (&block == VanillaBlocks::EMERALD_ORE) {
        return OreType::Emerald;
    }
    if (&block == VanillaBlocks::LAPIS_ORE) {
        return OreType::Lapis;
    }
    if (&block == VanillaBlocks::NETHER_QUARTZ_ORE) {
        return OreType::NetherQuartz;
    }
    if (&block == VanillaBlocks::NETHER_GOLD_ORE) {
        return OreType::NetherGold;
    }
    if (&block == VanillaBlocks::REDSTONE_ORE) {
        return OreType::Redstone;
    }

    return OreType::None;
}

i32 BlockDropHandler::spawnOreExperience(
    server::ServerWorld& world, const BlockPos& pos, OreType oreType, math::Random& rng)
{
    if (oreType == OreType::None) {
        return 0;
    }

    i32 xp = entity::experience::utils::randomOreExperience(rng, static_cast<i32>(oreType));

    if (xp <= 0) {
        return 0;
    }

    const f32 centerX = static_cast<f32>(pos.x) + 0.5f;
    const f32 centerY = static_cast<f32>(pos.y) + 0.5f;
    const f32 centerZ = static_cast<f32>(pos.z) + 0.5f;

    std::vector<i32> xpValues;
    entity::experience::utils::splitExperience(xp, xpValues);

    i32 spawnedCount = 0;
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        return 0;
    }
    for (i32 xpValue : xpValues) {
        auto orb = std::make_unique<ExperienceOrbEntity>(&world, centerX, centerY, centerZ, xpValue, *registry);

        // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
        orb->setTypeId(entity::EntityTypeKeys::EXPERIENCE_ORB);

        orb->setPickupDelay(10);

        const EntityInstanceId entityId = world.spawnEntity(std::move(orb));
        if (entityId != 0) {
            ++spawnedCount;
        }
    }

    return spawnedCount;
}

i32 BlockDropHandler::spawnOreExperience(
    EntityManager& entityManager, PhysicsEngine* physicsEngine, const BlockPos& pos, OreType oreType, math::Random& rng)
{
    if (oreType == OreType::None) {
        return 0;
    }

    // 使用 ExperienceUtils 计算随机经验值
    i32 xp = entity::experience::utils::randomOreExperience(rng, static_cast<i32>(oreType));

    if (xp <= 0) {
        return 0;
    }

    // 在方块中心位置生成经验球
    f32 centerX = static_cast<f32>(pos.x) + 0.5f;
    f32 centerY = static_cast<f32>(pos.y) + 0.5f;
    f32 centerZ = static_cast<f32>(pos.z) + 0.5f;

    // 分割经验值为多个经验球
    std::vector<i32> xpValues;
    entity::experience::utils::splitExperience(xp, xpValues);

    i32 spawnedCount = 0;
    // ECS 迁移：实体构造需要 registry 句柄，此重载无 IWorld，经 EntityManager.registry() 取
    // EntityManager 持有 ServerWorld 的 m_entityRegistry 引用（任务#24）
    auto& registry = entityManager.registry();
    for (i32 xpValue : xpValues) {
        // 创建经验球实体（不传 world，后续添加到 entityManager 时设置）
        auto orb = std::make_unique<ExperienceOrbEntity>(xpValue, registry);

        // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
        orb->setTypeId(entity::EntityTypeKeys::EXPERIENCE_ORB);

        // 设置位置
        orb->setPosition(centerX, centerY, centerZ);

        // 设置随机散射速度
        f32 vx = (rng.nextFloat() - 0.5f) * 0.2f;
        f32 vy = rng.nextFloat() * 0.2f + 0.1f;
        f32 vz = (rng.nextFloat() - 0.5f) * 0.2f;
        orb->setVelocity(vx, vy, vz);

        // 设置拾取延迟
        orb->setPickupDelay(10);

        if (physicsEngine) {
            orb->setPhysicsEngine(physicsEngine);
        }

        EntityInstanceId entityId = entityManager.addEntity(std::move(orb));
        if (entityId != 0) {
            spawnedCount++;
        }
    }

    return spawnedCount;
}

i32 BlockDropHandler::handleBlockBreakExperience(
    server::ServerWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack* tool, math::Random& rng)
{
    OreType oreType = getOreType(state);
    if (oreType == OreType::None) {
        return 0;
    }

    if (hasSilkTouch(tool)) {
        return 0;
    }

    if (state.requiresTool()) {
        if (!tool || tool->isEmpty()) {
            return 0;
        }

        if (!tool->canHarvestBlock(state)) {
            return 0;
        }
    }

    return spawnOreExperience(world, pos, oreType, rng);
}

i32 BlockDropHandler::handleBlockBreakExperience(EntityManager& entityManager,
    PhysicsEngine* physicsEngine,
    const BlockPos& pos,
    const BlockState& state,
    const ItemStack* tool,
    math::Random& rng)
{
    // 检查是否是矿石
    OreType oreType = getOreType(state);
    if (oreType == OreType::None) {
        return 0;
    }

    // 精准采集不掉落经验
    if (hasSilkTouch(tool)) {
        return 0;
    }

    // 检查是否使用正确工具
    // 如果方块需要工具才能采集，检查工具是否有效
    if (state.requiresTool()) {
        if (!tool || tool->isEmpty()) {
            return 0; // 需要工具但没有工具
        }

        // 使用 ItemStack 的 canHarvestBlock 方法检查
        if (!tool->canHarvestBlock(state)) {
            return 0; // 工具不能采集此方块
        }
    }

    // 生成经验球
    return spawnOreExperience(entityManager, physicsEngine, pos, oreType, rng);
}

} // namespace mc
