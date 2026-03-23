#include "BlockDropHandler.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/entity/loot/LootTable.hpp"
#include "common/entity/loot/LootConditions.hpp"
#include "common/entity/ItemEntity.hpp"
#include "common/entity/Player.hpp"
#include "common/item/ItemStack.hpp"
#include "common/item/Item.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// BlockDropHandler
// ============================================================================

std::vector<ItemStack> BlockDropHandler::generateDrops(
    IWorld& world,
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

    if (lootTable) {
        // 使用掉落表生成掉落
        math::Random random(static_cast<u64>(world.seed() ^ static_cast<u64>(pos.x ^ pos.z)));

        auto context = buildLootContext(world, pos, state, player, tool, random);
        if (context) {
            // 设置掉落表解析器
            context->setLootTableResolver([&lootTableManager](const String& id) -> const loot::LootTable* {
                return lootTableManager.getTable(id);
            });

            drops = lootTable->generate(*context);
        }
    } else {
        // 使用默认掉落逻辑
        drops = getDefaultDrops(state);
    }

    return drops;
}

std::vector<EntityId> BlockDropHandler::spawnDrops(
    server::ServerWorld& world,
    const BlockPos& pos,
    const std::vector<ItemStack>& drops,
    const String& throwerUuid)
{
    std::vector<EntityId> spawnedEntities;

    if (drops.empty()) {
        return spawnedEntities;
    }

    // 在方块中心位置生成物品实体
    f32 centerX = static_cast<f32>(pos.x) + 0.5f;
    f32 centerY = static_cast<f32>(pos.y) + 0.5f;
    f32 centerZ = static_cast<f32>(pos.z) + 0.5f;

    // 使用固定种子生成随机速度
    math::Random random(static_cast<u64>(pos.x ^ pos.z));

    for (const auto& stack : drops) {
        if (stack.isEmpty()) {
            continue;
        }

        // 创建物品实体
        auto itemEntity = std::make_unique<ItemEntity>(
            0,  // ID将由世界分配
            stack,
            centerX,
            centerY,
            centerZ
        );

        // 设置随机散射速度（参考 MC 1.16.5 ItemEntity）
        // 速度范围：[-0.05, 0.05] + [0.0, 0.2]
        f32 vx = (random.nextFloat() - 0.5f) * 0.1f + random.nextFloat() * 0.2f;
        f32 vy = random.nextFloat() * 0.2f;
        f32 vz = (random.nextFloat() - 0.5f) * 0.1f + random.nextFloat() * 0.2f;
        itemEntity->setVelocity(vx, vy, vz);

        // 设置投掷者UUID（防止立即拾取）
        if (!throwerUuid.empty()) {
            itemEntity->setOwner(throwerUuid, throwerUuid);
        }

        // 设置拾取延迟
        itemEntity->setPickupDelay(10);  // 10 ticks = 0.5秒

        // 生成到世界
        EntityId entityId = world.spawnEntity(std::move(itemEntity));
        spawnedEntities.push_back(entityId);
    }

    return spawnedEntities;
}

std::vector<EntityId> BlockDropHandler::spawnDrops(
    EntityManager& entityManager,
    PhysicsEngine* physicsEngine,
    const BlockPos& pos,
    const std::vector<ItemStack>& drops,
    const String& throwerUuid)
{
    std::vector<EntityId> spawnedEntities;

    if (drops.empty()) {
        return spawnedEntities;
    }

    // 在方块中心位置生成物品实体
    f32 centerX = static_cast<f32>(pos.x) + 0.5f;
    f32 centerY = static_cast<f32>(pos.y) + 0.5f;
    f32 centerZ = static_cast<f32>(pos.z) + 0.5f;

    // 使用固定种子生成随机速度
    math::Random random(static_cast<u64>(pos.x ^ pos.z));

    for (const auto& stack : drops) {
        if (stack.isEmpty()) {
            continue;
        }

        auto itemEntity = std::make_unique<ItemEntity>(
            0,
            stack,
            centerX,
            centerY,
            centerZ
        );

        f32 vx = (random.nextFloat() - 0.5f) * 0.1f + random.nextFloat() * 0.2f;
        f32 vy = random.nextFloat() * 0.2f;
        f32 vz = (random.nextFloat() - 0.5f) * 0.1f + random.nextFloat() * 0.2f;
        itemEntity->setVelocity(vx, vy, vz);

        if (!throwerUuid.empty()) {
            itemEntity->setOwner(throwerUuid, throwerUuid);
        }

        itemEntity->setPickupDelay(10);

        if (physicsEngine) {
            itemEntity->setPhysicsEngine(physicsEngine);
        }

        EntityId entityId = entityManager.addEntity(std::move(itemEntity));
        if (entityId != 0) {
            spawnedEntities.push_back(entityId);
        }
    }

    return spawnedEntities;
}

bool BlockDropHandler::canHarvestBlock(
    const BlockState& state,
    const Player* player,
    const ItemStack* tool)
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

std::vector<ItemStack> BlockDropHandler::getDefaultDrops(const BlockState& state) {
    // 默认掉落逻辑：无掉落
    // 子类或掉落表可以覆盖此行为
    (void)state;
    return {};
}

std::unique_ptr<loot::LootContext> BlockDropHandler::buildLootContext(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    const Player* player,
    const ItemStack* tool,
    math::Random& random)
{
    auto context = loot::LootContextBuilder(world)
        .withRandom(random)
        .withSeed(world.seed() ^ static_cast<u64>(pos.x ^ pos.z))
        .build();

    if (!context) {
        return nullptr;
    }

    // 设置方块状态和位置参数
    context->set(loot::LootParams::BLOCK_STATE, const_cast<BlockState*>(&state));
    context->set(loot::LootParams::BLOCK_POS, const_cast<BlockPos*>(&pos));

    // 设置工具参数
    if (tool && !tool->isEmpty()) {
        context->set(loot::LootParams::TOOL, const_cast<ItemStack*>(tool));

        // 设置时运等级
        i32 fortuneLevel = getFortuneLevel(tool);
        if (fortuneLevel > 0) {
            context->setLootingModifier(fortuneLevel);
            // 同时设置 FORTUNE_LEVEL 参数
            // 使用 LootContext 的 owned storage 来避免内存泄漏
            context->setOwnedValue(loot::LootParams::FORTUNE_LEVEL, fortuneLevel);
        }

        // 设置精准采集等级
        i32 silkTouchLevel = hasSilkTouch(tool) ? 1 : 0;
        if (silkTouchLevel > 0) {
            context->setOwnedValue(loot::LootParams::SILK_TOUCH_LEVEL, silkTouchLevel);
        }
    }

    // 设置玩家参数
    if (player) {
        Entity* playerEntity = const_cast<Player*>(player);
        context->set(loot::LootParams::THIS_ENTITY, playerEntity);
        context->set(loot::LootParams::KILLER_PLAYER, const_cast<Player*>(player));
    }

    return context;
}

bool BlockDropHandler::hasSilkTouch(const ItemStack* tool) {
    if (!tool || tool->isEmpty()) {
        return false;
    }

    return item::enchant::EnchantmentHelper::hasSilkTouch(*tool);
}

i32 BlockDropHandler::getFortuneLevel(const ItemStack* tool) {
    if (!tool || tool->isEmpty()) {
        return 0;
    }

    return item::enchant::EnchantmentHelper::getFortuneLevel(*tool);
}

i32 BlockDropHandler::applyFortuneBonus(i32 baseCount, i32 fortuneLevel, math::Random& random) {
    if (fortuneLevel <= 0) {
        return baseCount;
    }

    // MC 1.16.5 时运公式:
    // Fortune I: 33% 概率 +1
    // Fortune II: 25% 概率 +1, 25% 概率 +2 (累计 +0~2)
    // Fortune III: 20% 概率 +1, 20% 概率 +2, 20% 概率 +3 (累计 +0~3)
    i32 bonus = 0;
    for (i32 i = 0; i < fortuneLevel; ++i) {
        f32 chance = 1.0f / static_cast<f32>(fortuneLevel + 2);
        if (random.nextFloat() < chance) {
            ++bonus;
        }
    }

    return baseCount + bonus;
}

} // namespace mc
