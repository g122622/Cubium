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

#include "LootCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/InventorySlotMapping.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/PacketBuilders.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

namespace {

/**
 * @brief 生成随机种子
 */
u64 generateSeed()
{
    return static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count());
}

/**
 * @brief 同步玩家背包到客户端
 */
void syncInventoryToClient(ServerCommandSource& source, PlayerId playerId, const PlayerInventory& inventory)
{
    auto* server = source.server();
    if (server == nullptr) {
        return;
    }

    // 用 ContainerSetContent(containerId=0) 同步完整玩家物品栏。
    // stateId 取自玩家数据（containerId=0 在服务端无独立 AbstractContainerMenu 实例）。
    mc::network::ir::play::ContainerSetContent pkt;
    pkt.containerId = 0; // 玩家物品栏
    const auto* playerData = server->playerManager().getPlayer(playerId);
    pkt.stateId = (playerData != nullptr) ? playerData->incrementPlayerInventoryStateId() : 0;
    // items 按 InventoryMenu 46 槽布局构造，对齐 Java 客户端 containerId=0 期望。
    pkt.items = mc::buildMenuContent(inventory);
    pkt.carriedItem = mc::network::ir::play::ItemStackView{0, 0, {}}; // 空 carried

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
    (void)server->connectionManager().sendToPlayer(playerId, packet);
}

/**
 * @brief 获取命令源实体手持物品
 *
 * 使用 source.getEntity() 获取实体，
 * 如果实体是 LivingEntity 则从装备槽获取手持物品，非 LivingEntity 实体报错。
 * 回退到 source.getPlayer() 支持仅通过 PlayerId 标识的玩家。
 */
ItemStack getHeldItem(ServerCommandSource& source, const std::string& hand)
{
    // 优先从 entity 获取（支持非玩家实体）
    Entity* entity = source.entity();
    if (entity != nullptr) {
        auto* livingEntity = dynamic_cast<LivingEntity*>(entity);
        if (livingEntity != nullptr) {
            if (hand == "mainhand" || hand == "weapon") {
                return livingEntity->getMainHandItem();
            } else if (hand == "offhand") {
                return livingEntity->getOffHandItem();
            }
            return ItemStack::EMPTY;
        }
        // 非 LivingEntity 实体没有手持物品，抛出 ERROR_NO_HELD_ITEMS
        source.sendError("commands.loot.noHeldItem");
        return ItemStack::EMPTY;
    }

    // 回退到玩家物品栏（当 entity 为空但 player 非空时，如早期命令源构造场景）
    auto* player = source.player();
    if (player == nullptr) {
        return ItemStack::EMPTY;
    }

    PlayerInventory& inventory = player->inventory();
    if (hand == "mainhand" || hand == "weapon") {
        return inventory.getSelectedStack();
    } else if (hand == "offhand") {
        return inventory.getOffhandItem();
    }
    return ItemStack::EMPTY;
}

/**
 * @brief 解析战利品表
 */
const loot::LootTable* resolveLootTable(ServerCommandSource& source, const std::string& lootTableId)
{
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("No server available");
        return nullptr;
    }

    const loot::LootTable* table = server->lootTableManager().getTable(lootTableId);
    if (table == nullptr) {
        source.sendError("Unknown loot table: " + lootTableId);
        return nullptr;
    }

    return table;
}

/**
 * @brief 创建战利品上下文构建器的通用部分
 */
loot::LootContextBuilder createBaseContextBuilder(ServerCommandSource& source)
{
    auto* world = source.world();
    MC_ASSERT_RELEASE(world != nullptr);

    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    auto builder = loot::LootContextBuilder(*world)
                       .withSeed(generateSeed())
                       .withLootTableResolver(
                           [&manager = server->lootTableManager()](
                               const std::string& id) -> const loot::LootTable* { return manager.getTable(id); })
                       .withPredicateResolver([&manager = server->lootTableManager()](
                                                  const std::string& id) -> const loot::LootCondition* {
                           return manager.getPredicate(id);
                       });

    // 设置命令源实体（THIS_ENTITY 使用 source.getEntity()）
    if (source.entity() != nullptr) {
        builder.withNullableParameter(loot::LootParams::THIS_ENTITY, source.entity());
    }

    return builder;
}

/**
 * @brief 从 loot <loot_table> 源生成战利品
 */
std::vector<ItemStack> generateFromLootTable(ServerCommandSource& source, const std::string& lootTableId)
{
    auto* table = resolveLootTable(source, lootTableId);
    if (table == nullptr) {
        return {};
    }

    auto builder = createBaseContextBuilder(source);
    auto context = builder.build(loot::LootParameterSets::chest());
    return table->generate(*context);
}

/**
 * @brief 从 fish <loot_table> <pos> [tool] 源生成战利品
 */
std::vector<ItemStack> generateFromFish(
    ServerCommandSource& source, const std::string& lootTableId, const Vector3i& fishPos, const ItemStack& tool)
{
    auto* table = resolveLootTable(source, lootTableId);
    if (table == nullptr) {
        return {};
    }

    auto builder = createBaseContextBuilder(source);
    builder.withOwnedValue(loot::LootParams::TOOL, tool);
    auto context = builder.build(loot::LootParameterSets::fishing());
    return table->generate(*context);
}

/**
 * @brief 从 kill <target> 源生成战利品
 *
 * 使用目标实体的战利品表和 ENTITY 参数集，
 * 用魔法伤害作为伤害源，命令执行者作为击杀者。
 */
std::vector<ItemStack> generateFromKill(ServerCommandSource& source, Entity* target)
{
    if (target == nullptr) {
        return {};
    }

    auto* server = source.server();
    if (server == nullptr) {
        return {};
    }

    // 获取目标实体的战利品表ID
    std::string lootTableId = target->getLootTableId();
    if (lootTableId.empty()) {
        source.sendError("Entity " + target->getTypeId() + " has no loot table");
        return {};
    }

    // 查找战利品表
    auto* table = resolveLootTable(source, lootTableId);
    if (table == nullptr) {
        return {};
    }

    // 构建 ENTITY 参数集的 LootContext
    // 使用魔法伤害源，命令执行者作为攻击者
    auto builder = createBaseContextBuilder(source);

    // 设置 THIS_ENTITY（被杀实体）为必需参数
    builder.withParameter(loot::LootParams::THIS_ENTITY, target);

    // 设置 DAMAGE_SOURCE（魔法伤害，无实体来源）
    // DamageSource 是抽象类，不能使用 withOwnedValue，
    // 使用 withParameter 传入指针，确保 magicSource 生命周期覆盖 generate 调用
    auto magicSource = DamageSources::magic();
    builder.withParameter(loot::LootParams::DAMAGE_SOURCE, static_cast<DamageSource*>(&magicSource));

    // 设置攻击者（命令执行者）
    // 使用 source.entity() 支持非玩家命令执行者（如通过 /execute as @e 指定的实体）
    Entity* sourceEntity = source.entity();
    if (sourceEntity != nullptr) {
        builder.withNullableParameter(loot::LootParams::DIRECT_KILLER, sourceEntity);
        builder.withNullableParameter(loot::LootParams::KILLER_ENTITY, sourceEntity);
    }

    // 设置 LAST_DAMAGE_PLAYER（如果命令执行者是玩家）
    // lootparams$builder.withParameter(LootContextParams.LAST_DAMAGE_PLAYER, player) }
    Player* sourcePlayer = source.player();
    if (sourcePlayer != nullptr) {
        builder.withParameter(loot::LootParams::KILLER_PLAYER, sourcePlayer);
    }

    auto context = builder.build(loot::LootParameterSets::entity());
    return table->generate(*context);
}

/**
 * @brief 从 mine <pos> [tool] 源生成战利品
 */
std::vector<ItemStack> generateFromMine(ServerCommandSource& source, const Vector3i& minePos, const ItemStack& tool)
{
    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("No world available");
        return {};
    }

    BlockPos pos(minePos.x, minePos.y, minePos.z);
    const BlockState* blockState = world->getBlockState(pos);
    if (blockState == nullptr) {
        source.sendError("No block at that position");
        return {};
    }

    // 获取方块的战利品表ID
    const Block& block = blockState->getBlock();
    const std::string lootTableId = block.getLootTableId();
    if (lootTableId.empty()) {
        source.sendMessage("Block has no loot table");
        return {};
    }

    auto* table = resolveLootTable(source, lootTableId);
    if (table == nullptr) {
        return {};
    }

    auto builder = createBaseContextBuilder(source);
    builder.withParameter(loot::LootParams::BLOCK_STATE, const_cast<BlockState*>(blockState));
    builder.withParameter(loot::LootParams::BLOCK_POS, &pos);
    builder.withOwnedValue(loot::LootParams::TOOL, tool);
    BlockEntity* blockEntity = world->getBlockEntity(pos);
    if (blockEntity != nullptr) {
        builder.withNullableParameter(loot::LootParams::BLOCK_ENTITY, blockEntity);
    }

    auto context = builder.build(loot::LootParameterSets::block());
    return table->generate(*context);
}

/**
 * @brief 给予玩家物品列表
 *
 * @return 成功给予物品的玩家数量
 */
i32 giveItemsToPlayers(
    ServerCommandSource& source, const std::vector<PlayerId>& playerIds, const std::vector<ItemStack>& items)
{
    auto* server = source.server();
    if (server == nullptr) {
        return 0;
    }

    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        if (playerId == 0) {
            continue;
        }

        PlayerInventory* inventory = server->playerInventory(playerId);
        if (inventory == nullptr) {
            continue;
        }

        server::ServerWorld* playerWorld = server->getPlayerWorld(playerId);
        Player* player = playerWorld ? server->playerEntityManager().getPlayerEntity(playerId, *playerWorld) : nullptr;
        if (player == nullptr) {
            continue;
        }

        i32 totalAdded = 0;

        for (const auto& item : items) {
            if (item.isEmpty()) {
                continue;
            }

            ItemStack toGive = item.copy();
            i32 notAdded = inventory->add(toGive);

            // 背包满了，掉落剩余物品
            if (notAdded > 0) {
                ItemStack dropStack(item.getItem(), notAdded);
                math::Random rng(generateSeed());
                ItemDropHelper::spawnItemEntity(playerWorld,
                    dropStack,
                    player->x(),
                    player->y() + 0.5,
                    player->z(),
                    rng,
                    0,             // 立即可拾取
                    player->uuid() // owner
                );
            }

            totalAdded++;
        }

        if (totalAdded > 0) {
            syncInventoryToClient(source, playerId, *inventory);

            // 播放拾取音效（批5b：经 buildPlaySoundIr + connectionManager 投递，
            // 原 IServer::sendSoundToPlayer 纯虚已删）
            math::Random rng(static_cast<u64>(playerId) ^ generateSeed());
            const f32 pitch = (rng.nextFloat() - rng.nextFloat()) * 0.7f + 1.0f;
            server->connectionManager().sendToPlayer(playerId,
                mc::server::net::buildPlaySoundIr(SoundEvents::ENTITY_ITEM_PICKUP,
                    sound::SoundCategory::Players,
                    Vector3(player->x(), player->y(), player->z()),
                    0.2f,
                    pitch * 2.0f));
        }

        successCount++;
    }

    return successCount;
}

/**
 * @brief 在指定位置生成物品实体
 *
 * @return 生成的物品数量
 */
i32 spawnItemsAtPosition(ServerCommandSource& source, const Vector3d& pos, const std::vector<ItemStack>& items)
{
    auto* world = source.world();
    if (world == nullptr) {
        return 0;
    }

    for (const auto& item : items) {
        if (item.isEmpty()) {
            continue;
        }

        math::Random rng(generateSeed());
        ItemDropHelper::spawnItemEntity(
            world, item.copy(), pos.x, pos.y, pos.z, rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
    }

    return static_cast<i32>(items.size());
}

/**
 * @brief 将物品插入容器
 *
 * 自动合并堆叠，然后放入空槽位。
 * @return 成功插入的物品数量
 */
i32 insertItemsIntoContainer(IInventory& inventory, ItemStack stack)
{
    if (stack.isEmpty()) {
        return 0;
    }

    const i32 containerSize = inventory.getContainerSize();

    // 先尝试与已有物品堆叠
    for (i32 i = 0; i < containerSize && !stack.isEmpty(); ++i) {
        ItemStack existing = inventory.getItem(i);
        if (!existing.isEmpty() && existing.canMergeWith(stack)) {
            const i32 maxStack = existing.getMaxStackSize();
            const i32 space = maxStack - existing.getCount();
            if (space > 0) {
                const i32 toAdd = std::min(stack.getCount(), space);
                existing.grow(toAdd);
                stack.shrink(toAdd);
                inventory.setItem(i, existing);
            }
        }
    }

    // 再尝试放入空槽位
    for (i32 i = 0; i < containerSize && !stack.isEmpty(); ++i) {
        ItemStack existing = inventory.getItem(i);
        if (existing.isEmpty() && inventory.canPlaceItem(i, stack)) {
            inventory.setItem(i, stack.copy());
            stack.setCount(0);
            break;
        }
    }

    return stack.isEmpty() ? 1 : 0;
}

/**
 * @brief 发送成功消息
 */
void sendSuccessMessage(ServerCommandSource& source, const std::vector<ItemStack>& items, const std::string& sourceDesc)
{
    if (items.size() == 1) {
        const auto& item = items[0];
        std::ostringstream ss;
        ss << "Dropped " << item.getCount() << " [" << (item.getItem() ? item.getItem()->getName() : "unknown") << "]"
           << " from " << sourceDesc;
        source.sendMessage(ss.str());
    } else if (items.size() > 1) {
        std::ostringstream ss;
        ss << "Dropped " << items.size() << " items from " << sourceDesc;
        source.sendMessage(ss.str());
    } else {
        source.sendMessage("No items generated");
    }
}

/**
 * @brief 从上下文中解析工具物品
 */
ItemStack resolveTool(CommandContext<ServerCommandSource>& context)
{
    // 优先使用显式指定的工具物品
    if (context.hasArgument("tool")) {
        auto itemInput = context.getArgument<ItemInput>("tool");
        if (itemInput.isValid()) {
            const Item* item = itemInput.getItem();
            if (item != nullptr) {
                return ItemStack(item, 1);
            }
        }
    }

    if (context.hasArgument("mine_tool")) {
        auto itemInput = context.getArgument<ItemInput>("mine_tool");
        if (itemInput.isValid()) {
            const Item* item = itemInput.getItem();
            if (item != nullptr) {
                return ItemStack(item, 1);
            }
        }
    }

    // mainhand/offhand 或未指定工具时，使用命令源的主手物品
    // 无工具参数时默认使用 mainhand
    auto& source = context.getSource();
    return getHeldItem(source, "mainhand");
}

} // namespace

// ============================================================================
// 命令注册
// ============================================================================

void LootCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto lootNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("loot");
    lootNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(lootNode,
        support::makeMetadata("Drops items from a loot table.",
            "/loot <fish|loot|kill|mine> ... <give|spawn|insert|replace> ...",
            2,
            {},
            true));

    // ============================================================================
    // /loot loot <loot_table> <give|spawn|insert|replace> ...
    // ============================================================================
    {
        auto sourceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("loot");
        auto tableArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
            "loot_table", StringArgumentType::string());

        // give
        auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
        auto playersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "players", EntityArgumentType::players());
        playersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return lootGive(ctx); });
        giveNode->addChild(playersArg);

        // spawn
        auto spawnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spawn");
        auto spawnPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "target_pos", Vec3ArgumentType::vec3());
        spawnPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return lootSpawn(ctx); });
        spawnNode->addChild(spawnPosArg);

        // insert
        auto insertNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("insert");
        auto insertPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "insert_pos", BlockPosArgumentType::blockPos());
        insertPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return lootInsert(ctx); });
        insertNode->addChild(insertPosArg);

        // replace entity
        auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
        auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
        auto entityTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "entities", EntityArgumentType::entities());
        auto entitySlotArg =
            std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("slot", IntegerArgumentType::integer(0));
        auto entityCountArg =
            std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(1));
        entityCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return lootReplaceEntity(ctx); });
        entitySlotArg->addChild(entityCountArg);
        entitySlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return lootReplaceEntity(ctx); });
        entityTargetsArg->addChild(entitySlotArg);
        entityNode->addChild(entityTargetsArg);

        // replace block
        auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
        auto blockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "target_pos", BlockPosArgumentType::blockPos());
        auto blockSlotArg =
            std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("slot", IntegerArgumentType::integer(0));
        auto blockCountArg =
            std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(1));
        blockCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return lootReplaceBlock(ctx); });
        blockSlotArg->addChild(blockCountArg);
        blockSlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return lootReplaceBlock(ctx); });
        blockPosArg->addChild(blockSlotArg);
        blockNode->addChild(blockPosArg);

        replaceNode->addChild(entityNode);
        replaceNode->addChild(blockNode);

        tableArg->addChild(giveNode);
        tableArg->addChild(spawnNode);
        tableArg->addChild(insertNode);
        tableArg->addChild(replaceNode);
        sourceNode->addChild(tableArg);
        lootNode->addChild(sourceNode);
    }

    // ============================================================================
    // /loot fish <loot_table> <pos> [tool|mainhand|offhand] <give|spawn|insert|replace> ...
    // ============================================================================
    {
        auto sourceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("fish");
        auto tableArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
            "fish_loot_table", StringArgumentType::string());
        auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "fish_pos", BlockPosArgumentType::blockPos());

        // 工具选项：tool参数、mainhand、offhand
        auto toolArg =
            std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>("tool", ItemArgumentType::item());
        auto mainhandNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("mainhand");
        auto offhandNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("offhand");

        // 目标节点构建器（为每个变体创建）
        auto addTargets = [](CommandNode<ServerCommandSource>& parentNode) {
            // give
            auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
            auto playersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
                "players", EntityArgumentType::players());
            playersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return fishGive(ctx); });
            giveNode->addChild(playersArg);

            // spawn
            auto spawnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spawn");
            auto spawnPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
                "target_pos", Vec3ArgumentType::vec3());
            spawnPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return fishSpawn(ctx); });
            spawnNode->addChild(spawnPosArg);

            // insert
            auto insertNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("insert");
            auto insertPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
                "insert_pos", BlockPosArgumentType::blockPos());
            insertPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return fishInsert(ctx); });
            insertNode->addChild(insertPosArg);

            // replace entity
            auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
            auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
            auto entityTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
                "entities", EntityArgumentType::entities());
            auto entitySlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
                "slot", IntegerArgumentType::integer(0));
            auto entityCountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
                "count", IntegerArgumentType::integer(1));
            entityCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return fishReplaceEntity(ctx); });
            entitySlotArg->addChild(entityCountArg);
            entitySlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return fishReplaceEntity(ctx); });
            entityTargetsArg->addChild(entitySlotArg);
            entityNode->addChild(entityTargetsArg);

            // replace block
            auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
            auto blockPosArg2 = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
                "target_pos", BlockPosArgumentType::blockPos());
            auto blockSlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
                "slot2", IntegerArgumentType::integer(0));
            auto blockCountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
                "count2", IntegerArgumentType::integer(1));
            blockCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return fishReplaceBlock(ctx); });
            blockSlotArg->addChild(blockCountArg);
            blockSlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return fishReplaceBlock(ctx); });
            blockPosArg2->addChild(blockSlotArg);
            blockNode->addChild(blockPosArg2);

            replaceNode->addChild(entityNode);
            replaceNode->addChild(blockNode);

            parentNode.addChild(giveNode);
            parentNode.addChild(spawnNode);
            parentNode.addChild(insertNode);
            parentNode.addChild(replaceNode);
        };

        // 位置后无工具 -> 目标
        addTargets(*posArg);

        // 工具参数后 -> 目标
        addTargets(*toolArg);
        addTargets(*mainhandNode);
        addTargets(*offhandNode);

        posArg->addChild(toolArg);
        posArg->addChild(mainhandNode);
        posArg->addChild(offhandNode);
        tableArg->addChild(posArg);
        sourceNode->addChild(tableArg);
        lootNode->addChild(sourceNode);
    }

    // ============================================================================
    // /loot kill <target> <give|spawn|insert|replace> ...
    // ============================================================================
    {
        auto sourceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("kill");
        auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "kill_target", EntityArgumentType::entity());

        // give
        auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
        auto playersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "players", EntityArgumentType::players());
        playersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return killGive(ctx); });
        giveNode->addChild(playersArg);

        // spawn
        auto spawnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spawn");
        auto spawnPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "target_pos", Vec3ArgumentType::vec3());
        spawnPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return killSpawn(ctx); });
        spawnNode->addChild(spawnPosArg);

        // insert
        auto insertNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("insert");
        auto insertPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "insert_pos", BlockPosArgumentType::blockPos());
        insertPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return killInsert(ctx); });
        insertNode->addChild(insertPosArg);

        // replace (simplified)
        auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
        auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
        auto entityTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "entities", EntityArgumentType::entities());
        auto entitySlotArg =
            std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("slot", IntegerArgumentType::integer(0));
        entitySlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return killReplaceEntity(ctx); });
        entityTargetsArg->addChild(entitySlotArg);
        entityNode->addChild(entityTargetsArg);
        replaceNode->addChild(entityNode);

        targetArg->addChild(giveNode);
        targetArg->addChild(spawnNode);
        targetArg->addChild(insertNode);
        targetArg->addChild(replaceNode);
        sourceNode->addChild(targetArg);
        lootNode->addChild(sourceNode);
    }

    // ============================================================================
    // /loot mine <pos> [tool|mainhand|offhand] <give|spawn|insert|replace> ...
    // ============================================================================
    {
        auto sourceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("mine");
        auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "mine_pos", BlockPosArgumentType::blockPos());

        auto toolArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>(
            "mine_tool", ItemArgumentType::item());
        auto mainhandNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("mainhand");
        auto offhandNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("offhand");

        auto addTargets = [](CommandNode<ServerCommandSource>& parentNode) {
            auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
            auto playersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
                "players", EntityArgumentType::players());
            playersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return mineGive(ctx); });
            giveNode->addChild(playersArg);

            auto spawnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spawn");
            auto spawnPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
                "target_pos", Vec3ArgumentType::vec3());
            spawnPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return mineSpawn(ctx); });
            spawnNode->addChild(spawnPosArg);

            auto insertNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("insert");
            auto insertPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
                "insert_pos", BlockPosArgumentType::blockPos());
            insertPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return mineInsert(ctx); });
            insertNode->addChild(insertPosArg);

            auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
            auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
            auto blockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
                "target_pos", BlockPosArgumentType::blockPos());
            auto blockSlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
                "slot", IntegerArgumentType::integer(0));
            blockSlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return mineReplaceBlock(ctx); });
            blockPosArg->addChild(blockSlotArg);
            blockNode->addChild(blockPosArg);
            replaceNode->addChild(blockNode);

            parentNode.addChild(giveNode);
            parentNode.addChild(spawnNode);
            parentNode.addChild(insertNode);
            parentNode.addChild(replaceNode);
        };

        addTargets(*posArg);
        addTargets(*toolArg);
        addTargets(*mainhandNode);
        addTargets(*offhandNode);

        posArg->addChild(toolArg);
        posArg->addChild(mainhandNode);
        posArg->addChild(offhandNode);
        sourceNode->addChild(posArg);
        lootNode->addChild(sourceNode);
    }

    dispatcher.registerCommand(lootNode);
}

// ============================================================================
// /loot loot <loot_table> - 目标分发
// ============================================================================

i32 LootCommand::lootGive(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("loot_table");
    auto items = generateFromLootTable(source, lootTableId);
    if (items.empty()) {
        return 0;
    }

    auto selector = context.getArgument<EntitySelector>("players");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched");
        return 0;
    }

    i32 result = giveItemsToPlayers(source, playerIds, items);
    sendSuccessMessage(source, items, lootTableId);
    return result;
}

i32 LootCommand::lootSpawn(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("loot_table");
    auto items = generateFromLootTable(source, lootTableId);
    if (items.empty()) {
        return 0;
    }

    Vector3d pos = Vec3ArgumentType::getVec3(context, "target_pos", source);
    i32 result = spawnItemsAtPosition(source, pos, items);
    sendSuccessMessage(source, items, lootTableId);
    return result;
}

i32 LootCommand::lootInsert(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("loot_table");
    auto items = generateFromLootTable(source, lootTableId);
    if (items.empty()) {
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "insert_pos", source);
    BlockPos blockPos(pos.x, pos.y, pos.z);

    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("No world available");
        return 0;
    }

    BlockEntity* blockEntity = world->getBlockEntity(blockPos);
    if (blockEntity == nullptr) {
        source.sendError("No block entity at that position");
        return 0;
    }

    auto* container = dynamic_cast<ContainerBlockEntity*>(blockEntity);
    if (container == nullptr) {
        source.sendError("Block is not a container");
        return 0;
    }

    IInventory* inventory = container->getInventory();
    if (inventory == nullptr) {
        source.sendError("Container has no inventory");
        return 0;
    }

    i32 inserted = 0;
    for (auto& item : items) {
        if (item.isEmpty()) {
            continue;
        }
        if (insertItemsIntoContainer(*inventory, item.copy())) {
            inventory->setChanged();
            inserted++;
        }
    }

    sendSuccessMessage(source, items, lootTableId);
    return inserted;
}

i32 LootCommand::lootReplaceEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("loot_table");
    auto items = generateFromLootTable(source, lootTableId);

    auto selector = context.getArgument<EntitySelector>("entities");
    auto playerIds = support::resolvePlayerIds(source, selector);
    i32 slot = context.getArgument<i32>("slot");
    i32 count = 1;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }

    i32 totalReplaced = 0;
    for (PlayerId playerId : playerIds) {
        auto* playerWorld = source.server()->getPlayerWorld(playerId);
        auto* player =
            playerWorld ? source.server()->playerEntityManager().getPlayerEntity(playerId, *playerWorld) : nullptr;
        if (player == nullptr) {
            continue;
        }

        PlayerInventory& inventory = player->inventory();

        for (i32 i = 0; i < count; ++i) {
            i32 targetSlot = slot + i;
            if (targetSlot >= PlayerInventory::TOTAL_SIZE) {
                break;
            }

            ItemStack replacement = (i < static_cast<i32>(items.size())) ? items[i].copy() : ItemStack::EMPTY;
            inventory.setItem(targetSlot, replacement);
            totalReplaced++;
        }

        syncInventoryToClient(source, playerId, inventory);
    }

    sendSuccessMessage(source, items, lootTableId);
    return totalReplaced;
}

i32 LootCommand::lootReplaceBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("loot_table");
    auto items = generateFromLootTable(source, lootTableId);

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "target_pos", source);
    BlockPos blockPos(pos.x, pos.y, pos.z);
    i32 slot = context.getArgument<i32>("slot");
    i32 count = 1;
    if (context.hasArgument("count2")) {
        count = context.getArgument<i32>("count2");
    }

    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("No world available");
        return 0;
    }

    BlockEntity* blockEntity = world->getBlockEntity(blockPos);
    if (blockEntity == nullptr) {
        source.sendError("No block entity at that position");
        return 0;
    }

    auto* container = dynamic_cast<ContainerBlockEntity*>(blockEntity);
    if (container == nullptr) {
        source.sendError("Block is not a container");
        return 0;
    }

    IInventory* inventory = container->getInventory();
    if (inventory == nullptr) {
        source.sendError("Container has no inventory");
        return 0;
    }

    const i32 containerSize = inventory->getContainerSize();
    if (slot < 0 || slot >= containerSize) {
        source.sendError(
            "Slot " + std::to_string(slot) + " is out of range (0-" + std::to_string(containerSize - 1) + ")");
        return 0;
    }

    i32 replaced = 0;
    for (i32 i = 0; i < count; ++i) {
        i32 targetSlot = slot + i;
        if (targetSlot >= containerSize) {
            break;
        }

        ItemStack replacement = (i < static_cast<i32>(items.size())) ? items[i].copy() : ItemStack::EMPTY;

        if (inventory->canPlaceItem(targetSlot, replacement)) {
            inventory->setItem(targetSlot, replacement);
            replaced++;
        }
    }

    if (replaced > 0) {
        inventory->setChanged();
    }

    sendSuccessMessage(source, items, lootTableId);
    return replaced;
}

// ============================================================================
// /loot fish <loot_table> <pos> [tool] - 目标分发
// ============================================================================

i32 LootCommand::fishGive(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("fish_loot_table");
    auto fishPos = BlockPosArgumentType::getBlockPos(context, "fish_pos", source);
    ItemStack tool = resolveTool(context);

    auto items = generateFromFish(source, lootTableId, fishPos, tool);
    if (items.empty()) {
        return 0;
    }

    auto selector = context.getArgument<EntitySelector>("players");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched");
        return 0;
    }

    i32 result = giveItemsToPlayers(source, playerIds, items);
    sendSuccessMessage(source, items, lootTableId);
    return result;
}

i32 LootCommand::fishSpawn(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("fish_loot_table");
    auto fishPos = BlockPosArgumentType::getBlockPos(context, "fish_pos", source);
    ItemStack tool = resolveTool(context);

    auto items = generateFromFish(source, lootTableId, fishPos, tool);
    if (items.empty()) {
        return 0;
    }

    Vector3d pos = Vec3ArgumentType::getVec3(context, "target_pos", source);
    i32 result = spawnItemsAtPosition(source, pos, items);
    sendSuccessMessage(source, items, lootTableId);
    return result;
}

i32 LootCommand::fishInsert(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("fish_loot_table");
    auto fishPos = BlockPosArgumentType::getBlockPos(context, "fish_pos", source);
    ItemStack tool = resolveTool(context);

    auto items = generateFromFish(source, lootTableId, fishPos, tool);
    if (items.empty()) {
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "insert_pos", source);
    BlockPos blockPos(pos.x, pos.y, pos.z);

    auto* world = source.world();
    BlockEntity* blockEntity = world ? world->getBlockEntity(blockPos) : nullptr;
    auto* container = blockEntity ? dynamic_cast<ContainerBlockEntity*>(blockEntity) : nullptr;
    IInventory* inventory = container ? container->getInventory() : nullptr;
    if (inventory == nullptr) {
        source.sendError("No container at that position");
        return 0;
    }

    i32 inserted = 0;
    for (auto& item : items) {
        if (!item.isEmpty() && insertItemsIntoContainer(*inventory, item.copy())) {
            inventory->setChanged();
            inserted++;
        }
    }

    sendSuccessMessage(source, items, lootTableId);
    return inserted;
}

i32 LootCommand::fishReplaceEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("fish_loot_table");
    auto fishPos = BlockPosArgumentType::getBlockPos(context, "fish_pos", source);
    ItemStack tool = resolveTool(context);

    auto items = generateFromFish(source, lootTableId, fishPos, tool);

    auto selector = context.getArgument<EntitySelector>("entities");
    auto playerIds = support::resolvePlayerIds(source, selector);
    i32 slot = context.getArgument<i32>("slot");

    i32 totalReplaced = 0;
    for (PlayerId playerId : playerIds) {
        auto* playerWorld = source.server()->getPlayerWorld(playerId);
        auto* player =
            playerWorld ? source.server()->playerEntityManager().getPlayerEntity(playerId, *playerWorld) : nullptr;
        if (player == nullptr) {
            continue;
        }

        PlayerInventory& inventory = player->inventory();
        if (slot >= 0 && slot < PlayerInventory::TOTAL_SIZE) {
            ItemStack replacement = items.empty() ? ItemStack::EMPTY : items[0].copy();
            inventory.setItem(slot, replacement);
            totalReplaced++;
        }

        syncInventoryToClient(source, playerId, inventory);
    }

    sendSuccessMessage(source, items, lootTableId);
    return totalReplaced;
}

i32 LootCommand::fishReplaceBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTableId = context.getArgument<std::string>("fish_loot_table");
    auto fishPos = BlockPosArgumentType::getBlockPos(context, "fish_pos", source);
    ItemStack tool = resolveTool(context);

    auto items = generateFromFish(source, lootTableId, fishPos, tool);

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "target_pos", source);
    BlockPos blockPos(pos.x, pos.y, pos.z);
    i32 slot = context.getArgument<i32>("slot2");

    auto* world = source.world();
    BlockEntity* blockEntity = world ? world->getBlockEntity(blockPos) : nullptr;
    auto* container = blockEntity ? dynamic_cast<ContainerBlockEntity*>(blockEntity) : nullptr;
    IInventory* inventory = container ? container->getInventory() : nullptr;
    if (inventory == nullptr) {
        source.sendError("No container at that position");
        return 0;
    }

    const i32 containerSize = inventory->getContainerSize();
    if (slot < 0 || slot >= containerSize) {
        source.sendError("Slot out of range");
        return 0;
    }

    ItemStack replacement = items.empty() ? ItemStack::EMPTY : items[0].copy();
    if (inventory->canPlaceItem(slot, replacement)) {
        inventory->setItem(slot, replacement);
        inventory->setChanged();
    }

    sendSuccessMessage(source, items, lootTableId);
    return 1;
}

// ============================================================================
// /loot kill <target> - 目标分发
// ============================================================================

namespace {

/**
 * @brief 从kill目标实体选择器生成战利品
 *
 * 使用 EntityResolver 解析任意实体（包括非玩家实体如僵尸、动物等）。
 *
 * @param source 命令源
 * @param selector 实体选择器
 * @return 生成的所有战利品物品列表
 */
std::vector<ItemStack> collectKillLoot(ServerCommandSource& source, const EntitySelector& selector)
{
    std::vector<ItemStack> allItems;
    auto entities = support::EntityResolver::resolve(source, selector);
    for (Entity* entity : entities) {
        auto items = generateFromKill(source, entity);
        for (auto& item : items) {
            allItems.push_back(std::move(item));
        }
    }
    return allItems;
}

} // anonymous namespace

i32 LootCommand::killGive(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto selector = context.getArgument<EntitySelector>("kill_target");

    auto allItems = collectKillLoot(source, selector);
    if (allItems.empty()) {
        source.sendMessage("No loot generated from kill target");
        return 0;
    }

    auto giveSelector = context.getArgument<EntitySelector>("players");
    auto givePlayerIds = support::resolvePlayerIds(source, giveSelector);
    if (givePlayerIds.empty()) {
        source.sendError("No players matched");
        return 0;
    }

    i32 result = giveItemsToPlayers(source, givePlayerIds, allItems);
    sendSuccessMessage(source, allItems, "kill");
    return result;
}

i32 LootCommand::killSpawn(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto selector = context.getArgument<EntitySelector>("kill_target");

    auto allItems = collectKillLoot(source, selector);
    if (allItems.empty()) {
        return 0;
    }

    Vector3d pos = Vec3ArgumentType::getVec3(context, "target_pos", source);
    i32 result = spawnItemsAtPosition(source, pos, allItems);

    sendSuccessMessage(source, allItems, "kill");
    return result;
}

i32 LootCommand::killInsert(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto selector = context.getArgument<EntitySelector>("kill_target");

    auto allItems = collectKillLoot(source, selector);
    if (allItems.empty()) {
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "insert_pos", source);
    BlockPos blockPos(pos.x, pos.y, pos.z);

    auto* world = source.world();
    if (world == nullptr) {
        return 0;
    }

    BlockEntity* blockEntity = world->getBlockEntity(blockPos);
    auto* container = blockEntity ? dynamic_cast<ContainerBlockEntity*>(blockEntity) : nullptr;
    IInventory* inventory = container ? container->getInventory() : nullptr;
    if (inventory == nullptr) {
        source.sendError("No container at that position");
        return 0;
    }

    i32 inserted = 0;
    for (auto& item : allItems) {
        if (!item.isEmpty() && insertItemsIntoContainer(*inventory, item.copy())) {
            inventory->setChanged();
            inserted++;
        }
    }

    sendSuccessMessage(source, allItems, "kill");
    return inserted;
}

i32 LootCommand::killReplaceEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto selector = context.getArgument<EntitySelector>("kill_target");

    auto allItems = collectKillLoot(source, selector);
    if (allItems.empty()) {
        return 0;
    }

    // kill replace 使用实体选择器获取目标玩家并替换其物品栏槽位
    auto replaceSelector = context.getArgument<EntitySelector>("players");
    auto replacePlayerIds = support::resolvePlayerIds(source, replaceSelector);
    if (replacePlayerIds.empty()) {
        source.sendError("No players matched");
        return 0;
    }

    i32 slot = context.getArgument<i32>("slot");
    i32 count = 0;

    for (PlayerId replacePlayerId : replacePlayerIds) {
        auto* replacePlayerWorld = source.server()->getPlayerWorld(replacePlayerId);
        auto* replacePlayer = replacePlayerWorld
            ? source.server()->playerEntityManager().getPlayerEntity(replacePlayerId, *replacePlayerWorld)
            : nullptr;
        if (replacePlayer == nullptr) {
            continue;
        }

        PlayerInventory& inventory = replacePlayer->inventory();
        if (slot >= 0 && slot < inventory.getContainerSize()) {
            ItemStack replacement = allItems.empty() ? ItemStack::EMPTY : allItems[0].copy();
            inventory.setItem(slot, replacement);
            inventory.setChanged();
            count++;
        }
    }

    sendSuccessMessage(source, allItems, "kill");
    return count;
}

// ============================================================================
// /loot mine <pos> [tool] - 目标分发
// ============================================================================

i32 LootCommand::mineGive(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto minePos = BlockPosArgumentType::getBlockPos(context, "mine_pos", source);
    ItemStack tool = resolveTool(context);

    auto items = generateFromMine(source, minePos, tool);
    if (items.empty()) {
        return 0;
    }

    // 获取战利品表描述
    auto* world = source.world();
    BlockPos pos(minePos.x, minePos.y, minePos.z);
    const BlockState* blockState = world ? world->getBlockState(pos) : nullptr;
    std::string sourceDesc = blockState ? blockState->getBlock().getLootTableId() : "mine";

    auto selector = context.getArgument<EntitySelector>("players");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched");
        return 0;
    }

    i32 result = giveItemsToPlayers(source, playerIds, items);
    sendSuccessMessage(source, items, sourceDesc);
    return result;
}

i32 LootCommand::mineSpawn(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto minePos = BlockPosArgumentType::getBlockPos(context, "mine_pos", source);
    ItemStack tool = resolveTool(context);

    auto items = generateFromMine(source, minePos, tool);
    if (items.empty()) {
        return 0;
    }

    Vector3d pos = Vec3ArgumentType::getVec3(context, "target_pos", source);
    i32 result = spawnItemsAtPosition(source, pos, items);

    auto* world = source.world();
    BlockPos bp(minePos.x, minePos.y, minePos.z);
    const BlockState* blockState = world ? world->getBlockState(bp) : nullptr;
    std::string sourceDesc = blockState ? blockState->getBlock().getLootTableId() : "mine";
    sendSuccessMessage(source, items, sourceDesc);
    return result;
}

i32 LootCommand::mineInsert(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto minePos = BlockPosArgumentType::getBlockPos(context, "mine_pos", source);
    ItemStack tool = resolveTool(context);

    auto items = generateFromMine(source, minePos, tool);
    if (items.empty()) {
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "insert_pos", source);
    BlockPos blockPos(pos.x, pos.y, pos.z);

    auto* world = source.world();
    BlockEntity* blockEntity = world ? world->getBlockEntity(blockPos) : nullptr;
    auto* container = blockEntity ? dynamic_cast<ContainerBlockEntity*>(blockEntity) : nullptr;
    IInventory* inventory = container ? container->getInventory() : nullptr;
    if (inventory == nullptr) {
        source.sendError("No container at that position");
        return 0;
    }

    i32 inserted = 0;
    for (auto& item : items) {
        if (!item.isEmpty() && insertItemsIntoContainer(*inventory, item.copy())) {
            inventory->setChanged();
            inserted++;
        }
    }

    BlockPos bp(minePos.x, minePos.y, minePos.z);
    const BlockState* blockState = world ? world->getBlockState(bp) : nullptr;
    std::string sourceDesc = blockState ? blockState->getBlock().getLootTableId() : "mine";
    sendSuccessMessage(source, items, sourceDesc);
    return inserted;
}

i32 LootCommand::mineReplaceBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto minePos = BlockPosArgumentType::getBlockPos(context, "mine_pos", source);
    ItemStack tool = resolveTool(context);

    auto items = generateFromMine(source, minePos, tool);

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "target_pos", source);
    BlockPos blockPos(pos.x, pos.y, pos.z);
    i32 slot = context.getArgument<i32>("slot");

    auto* world = source.world();
    BlockEntity* blockEntity = world ? world->getBlockEntity(blockPos) : nullptr;
    auto* container = blockEntity ? dynamic_cast<ContainerBlockEntity*>(blockEntity) : nullptr;
    IInventory* inventory = container ? container->getInventory() : nullptr;
    if (inventory == nullptr) {
        source.sendError("No container at that position");
        return 0;
    }

    const i32 containerSize = inventory->getContainerSize();
    if (slot < 0 || slot >= containerSize) {
        source.sendError("Slot out of range");
        return 0;
    }

    ItemStack replacement = items.empty() ? ItemStack::EMPTY : items[0].copy();
    if (inventory->canPlaceItem(slot, replacement)) {
        inventory->setItem(slot, replacement);
        inventory->setChanged();
    }

    BlockPos bp(minePos.x, minePos.y, minePos.z);
    const BlockState* blockState = world ? world->getBlockState(bp) : nullptr;
    std::string sourceDesc = blockState ? blockState->getBlock().getLootTableId() : "mine";
    sendSuccessMessage(source, items, sourceDesc);
    return 1;
}

} // namespace command
} // namespace mc
