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

#include "BlockInteractionManager.hpp"
#include "InventoryManager.hpp"
#include "SignCommandHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/drop/BlockDropHandler.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::server::interaction {

// ============================================================================
// 辅助方法实现
// ============================================================================

ServerPlayerData* BlockInteractionManager::_validatePlayer(PlayerId playerId) const noexcept
{
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData || !playerData->loggedIn) {
        return nullptr;
    }
    return playerData;
}

std::optional<Error> BlockInteractionManager::_validateInteractionPreconditions(
    PlayerId playerId, const BlockPos& pos, bool checkYRange) const
{
    // 验证玩家
    auto* playerData = _validatePlayer(playerId);
    if (!playerData) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 验证距离
    if (!_canInteract(playerId, pos)) {
        return Error(ErrorCode::InvalidArgument, "Block too far away");
    }

    // 验证 Y 范围（可选）
    if (checkYRange) {
        if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
            return Error(ErrorCode::InvalidArgument, "Block Y out of range");
        }
    }

    return std::nullopt;
}

const BlockState* BlockInteractionManager::_getNonAirBlockState(ServerWorld& world, const BlockPos& pos) const noexcept
{
    const BlockState* state = world.getBlockState(pos);
    if (!state || state->isAir()) {
        return nullptr;
    }
    return state;
}

std::optional<Error> BlockInteractionManager::_checkWorldModificationAllowed(ServerWorld& world) const noexcept
{
    if (world.isDebugWorld()) {
        return Error(ErrorCode::PermissionDenied, "Cannot modify blocks in debug world");
    }
    return std::nullopt;
}

ItemStack BlockInteractionManager::_getHeldTool(PlayerId playerId) const noexcept
{
    if (!m_inventoryManager) {
        return ItemStack();
    }
    return m_inventoryManager->getHeldItem(playerId);
}

Player* BlockInteractionManager::_getPlayerEntity(PlayerId playerId, ServerWorld& world) const noexcept
{
    if (m_server == nullptr) {
        return nullptr;
    }

    return m_server->playerEntityManager().getPlayerEntity(playerId, world);
}

u32 BlockInteractionManager::_setBlockToAir(
    ServerWorld& world, const BlockPos& pos, const BlockState& oldState, PlayerId playerId)
{
    Block* airBlock = Block::getBlock(ResourceLocation("minecraft:air"));
    if (!airBlock) {
        return 0;
    }

    world.setBlockState(pos, &airBlock->defaultState());

    if (m_onBlockBreak) {
        m_onBlockBreak(playerId, pos, oldState);
    }

    return airBlock->defaultState().stateId();
}

// ============================================================================
// 构造函数
// ============================================================================

BlockInteractionManager::BlockInteractionManager(
    core::PlayerManager& playerManager, loot::LootTableManager& lootTableManager)
    : m_playerManager(playerManager)
    , m_lootTableManager(lootTableManager)
{}

ServerWorld* BlockInteractionManager::_getPlayerWorld(PlayerId playerId) const noexcept
{
    if (m_server == nullptr) {
        return nullptr;
    }
    return m_server->getPlayerWorld(playerId);
}

void BlockInteractionManager::setInventoryManager(InventoryManager* inventoryManager)
{
    m_inventoryManager = inventoryManager;
}

// ============================================================================
// 公共方法实现
// ============================================================================

Result<BlockInteractionResult> BlockInteractionManager::handleBlockInteraction(
    PlayerId playerId, const BlockPos& pos, network::BlockInteractionAction action)
{
    MC_TRACE_EVENT("server.world",
        "BlockInteractionManager::handleBlockInteraction",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        "action",
        static_cast<u8>(action),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 验证前置条件
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, true);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    // 获取方块状态
    const BlockState* state = _getNonAirBlockState(*world, pos);
    if (!state) {
        return BlockInteractionResult{false, "No block to interact with"};
    }

    // 处理不同动作
    switch (action) {
        case network::BlockInteractionAction::StartDestroyBlock:
            // 开始破坏 - 通常由 MiningManager 处理
            break;

        case network::BlockInteractionAction::AbortDestroyBlock:
            // 中止破坏
            break;

        case network::BlockInteractionAction::StopDestroyBlock:
            // 完成破坏
            if (_canBreakBlock(*world, playerId, pos, state)) {
                // 获取手持物品作为工具
                ItemStack tool = _getHeldTool(playerId);

                // 生成掉落物
                _generateBlockDrops(*world, pos, *state, playerId, tool.isEmpty() ? nullptr : &tool);

                // 调用方块的 spawnAfterBreak 回调（如 InfestedBlock 生成蠹虫）
                const Block& breakBlock = state->getBlock();
                breakBlock.spawnAfterBreak(*world, pos, *state, tool.isEmpty() ? nullptr : &tool, true);

                // 设置为空气
                _setBlockToAir(*world, pos, *state, playerId);

                return BlockInteractionResult{true, "Block destroyed"};
            }
            break;

        default:
            break;
    }

    return BlockInteractionResult{false, "Action not handled"};
}

Result<BlockPlacementResult> BlockInteractionManager::handleBlockPlacement(
    PlayerId playerId, const BlockPos& pos, const Vector3& hitPos, Direction face, const ItemStack& heldItem)
{
    MC_TRACE_EVENT("server.world",
        "BlockInteractionManager::handleBlockPlacement",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        "face",
        Directions::toString(face),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 检查世界修改权限
    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    auto worldError = _checkWorldModificationAllowed(*world);
    if (worldError) {
        return std::move(*worldError);
    }

    // 验证前置条件（不需要检查 Y 范围，因为放置位置可能在不同高度）
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, false);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    // 获取玩家数据用于游戏模式检查
    auto* playerData = _validatePlayer(playerId);
    if (!playerData) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 检查游戏模式
    if (playerData->gameMode == GameMode::Spectator) {
        return Error(ErrorCode::PermissionDenied, "Cannot place blocks in spectator mode");
    }

    // 获取物品对应的方块物品
    const Item* item = heldItem.getItem();
    if (!item) {
        return Error(ErrorCode::InvalidArgument, "No item in hand");
    }

    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItemByItemId(item->itemId());
    if (!blockItem) {
        return Error(ErrorCode::InvalidArgument, "Item is not a block item");
    }

    // 创建放置上下文（player 为 nullptr，因为我们通过 InventoryManager 管理）
    BlockItemUseContext context(*world, nullptr, heldItem, hitPos, pos, face, playerData->yaw);

    // 先检查是否可以放置（位置有效性检查）
    if (!context.canPlace()) {
        return BlockPlacementResult{false, false, false, pos, 0, "Cannot place block here"};
    }

    // 获取放置位置和状态（不实际放置）
    const BlockPos& placePos = context.placementPos();
    const BlockState* newState = blockItem->getStateForPlacement(context);
    if (!newState) {
        return BlockPlacementResult{false, false, false, pos, 0, "No placement state"};
    }

    // 在放置之前检查玩家碰撞
    if (_wouldCollideWithPlayer(playerId, placePos, *newState)) {
        return BlockPlacementResult{false, false, false, placePos, 0, "Cannot place block inside player"};
    }

    // 执行放置
    ActionResultType result = blockItem->tryPlace(context);
    if (result != ActionResultType::Success && result != ActionResultType::Consume) {
        return BlockPlacementResult{false, false, false, pos, 0, "Cannot place block here"};
    }

    // 消耗物品（非创造模式）
    bool itemConsumed = false;
    if (playerData->gameMode != GameMode::Creative && m_inventoryManager) {
        // 获取玩家物品栏
        PlayerInventory* inventory = m_inventoryManager->getInventory(playerId);
        if (inventory) {
            // 减少手持物品数量
            ItemStack selectedStack = inventory->getSelectedStack();
            if (!selectedStack.isEmpty() && selectedStack.getCount() > 0) {
                selectedStack.shrink(1);
                inventory->setItem(inventory->getSelectedSlot(), selectedStack);
                itemConsumed = true;
                // 同步到客户端
                m_inventoryManager->syncToClient(playerId);
            }
        }
    } else if (playerData->gameMode == GameMode::Creative) {
        itemConsumed = true; // 创造模式不实际消耗
    }

    if (m_onBlockPlace) {
        m_onBlockPlace(playerId, placePos, *newState);
    }

    return BlockPlacementResult{true, true, itemConsumed, placePos, newState->stateId(), "Block placed"};
}

Result<BlockInteractionResult> BlockInteractionManager::handleBlockUse(
    PlayerId playerId, const BlockPos& pos, Hand hand, const Vector3& hitPos, Direction face)
{
    MC_TRACE_EVENT("server.world",
        "BlockInteractionManager::handleBlockUse",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        "hand",
        hand == Hand::MainHand ? "main" : "off",
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 验证前置条件（方块使用不需要检查 Y 范围）
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, false);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    // 获取方块状态
    const BlockState* state = _getNonAirBlockState(*world, pos);
    if (!state) {
        return BlockInteractionResult{false, "No block to use"};
    }

    Block* block = Block::getBlock(state->blockId());
    if (!block) {
        return Error(ErrorCode::NotFound, "Block not found for state");
    }

    // 获取玩家数据用于交互
    auto* playerData = _validatePlayer(playerId);
    if (!playerData) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    Player interactionPlayer(playerId, playerData->username);
    const BlockRaycastResult hitResult = BlockRaycastResult::hit(hitPos, pos, face, 0.0f);

    ActionResultType result = block->onBlockActivated(*state, *world, pos, interactionPlayer, hand, hitResult);

    // 派发自定义方块组件回调 - onPlayerInteract
    auto& blockCompReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
    std::string blockTypeId = block->blockLocation().toString();
    if (blockCompReg.hasPlayerInteractCallback(blockTypeId)) {
        mc::mod::bedrock::addon::BlockComponentPlayerInteractEvent event;
        event.blockTypeId = blockTypeId;
        event.blockX = pos.x;
        event.blockY = pos.y;
        event.blockZ = pos.z;
        event.dimensionId = world->dimension();
        event.playerId = playerId;
        event.face = static_cast<i32>(face);
        event.faceX = hitPos.x - static_cast<f32>(pos.x);
        event.faceY = hitPos.y - static_cast<f32>(pos.y);
        event.faceZ = hitPos.z - static_cast<f32>(pos.z);
        blockCompReg.dispatchPlayerInteract(blockTypeId, event);
    }

    // 派发自定义物品组件回调 - onUseOn
    // 当玩家右键点击方块时，手持物品触发 onUseOn 回调
    ItemStack heldItem = _getHeldTool(playerId);
    if (!heldItem.isEmpty()) {
        const Item* heldItemPtr = heldItem.getItem();
        if (heldItemPtr != nullptr) {
            auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
            std::string itemTypeId = heldItemPtr->itemLocation().toString();
            if (itemCompReg.hasUseOnCallback(itemTypeId)) {
                mc::mod::bedrock::addon::ItemComponentUseOnEvent useOnEvent;
                useOnEvent.itemTypeId = itemTypeId;
                useOnEvent.sourceId = playerId;
                useOnEvent.blockX = pos.x;
                useOnEvent.blockY = pos.y;
                useOnEvent.blockZ = pos.z;
                useOnEvent.usedOnBlockTypeId = blockTypeId;
                useOnEvent.usedOnBlockPermutationTypeId = blockTypeId;
                useOnEvent.face = static_cast<i32>(face);
                useOnEvent.faceX = hitPos.x - static_cast<f32>(pos.x);
                useOnEvent.faceY = hitPos.y - static_cast<f32>(pos.y);
                useOnEvent.faceZ = hitPos.z - static_cast<f32>(pos.z);
                itemCompReg.dispatchUseOn(itemTypeId, useOnEvent);
            }
        }
    }

    // 如果方块交互成功，检查是否为告示牌并执行命令
    bool handled = (result == ActionResultType::Success || result == ActionResultType::Consume);
    if (handled && m_server != nullptr) {
        // 检查是否为告示牌方块
        BlockEntity* blockEntity = world->getBlockEntity(pos);
        if (blockEntity && blockEntity->getType() == BlockEntityType::Sign) {
            // 获取实际的 ServerPlayer 实体
            ServerPlayerEntityManager& entityManager = m_server->playerEntityManager();
            Player* playerEntity = entityManager.getPlayerEntity(playerId, *world);
            if (playerEntity != nullptr) {
                mc::ServerPlayer* serverPlayer = playerEntity->asServerPlayer();
                if (serverPlayer != nullptr) {
                    // 执行告示牌命令
                    _handleSignCommand(*world, pos, *serverPlayer);
                }
            }
        }
    }

    return BlockInteractionResult{handled, handled ? "Block used" : "Block use pass"};
}

Result<BlockBreakResult> BlockInteractionManager::handleBlockBreak(PlayerId playerId, const BlockPos& pos)
{
    MC_TRACE_EVENT("server.world",
        "BlockInteractionManager::handleBlockBreak",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 检查世界修改权限
    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    auto worldError = _checkWorldModificationAllowed(*world);
    if (worldError) {
        return std::move(*worldError);
    }

    // 验证前置条件
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, true);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    // 获取方块状态
    const BlockState* state = _getNonAirBlockState(*world, pos);
    if (!state) {
        return BlockBreakResult{false, 0, "No block to break"};
    }

    const BlockState oldState = *state;

    // 检查是否可破坏
    if (!_canBreakBlock(*world, playerId, pos, state)) {
        return BlockBreakResult{false, 0, "Cannot break this block"};
    }

    // 获取手持物品作为工具
    ItemStack tool = _getHeldTool(playerId);

    // 派发自定义方块组件回调 - onPlayerBreak
    auto& blockCompReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
    std::string blockTypeId = oldState.getBlock().blockLocation().toString();
    if (blockCompReg.hasPlayerBreakCallback(blockTypeId)) {
        mc::mod::bedrock::addon::BlockComponentPlayerBreakEvent event;
        event.blockTypeId = blockTypeId;
        event.blockX = pos.x;
        event.blockY = pos.y;
        event.blockZ = pos.z;
        event.dimensionId = world->dimension();
        event.playerId = playerId;
        event.brokenBlockPermutationTypeId = blockTypeId;
        blockCompReg.dispatchPlayerBreak(blockTypeId, event);
    }
    if (blockCompReg.hasBreakCallback(blockTypeId)) {
        mc::mod::bedrock::addon::BlockComponentBreakEvent breakEvent;
        breakEvent.blockTypeId = blockTypeId;
        breakEvent.blockX = pos.x;
        breakEvent.blockY = pos.y;
        breakEvent.blockZ = pos.z;
        breakEvent.dimensionId = world->dimension();
        breakEvent.brokenBlockPermutationTypeId = blockTypeId;
        breakEvent.entitySourceId = playerId;
        blockCompReg.dispatchBreak(blockTypeId, breakEvent);
    }

    // 生成掉落物
    _generateBlockDrops(*world, pos, oldState, playerId, tool.isEmpty() ? nullptr : &tool);

    // 调用方块的 spawnAfterBreak 回调（如 InfestedBlock 生成蠹虫）
    // 在掉落物生成之后、方块移除之前调用，此时工具信息可用
    const Block& oldBlock = oldState.getBlock();
    oldBlock.spawnAfterBreak(*world, pos, oldState, tool.isEmpty() ? nullptr : &tool, true);

    // 调用工具的 onBlockDestroyed 回调（用于耐久消耗等）
    if (!tool.isEmpty()) {
        const Item* toolItem = tool.getItem();
        if (toolItem != nullptr) {
            // 获取玩家实体用于 onBlockDestroyed 调用
            Player* playerEntity = _getPlayerEntity(playerId, *world);
            if (playerEntity != nullptr) {
                const_cast<Item*>(toolItem)->onBlockDestroyed(tool, *world, oldState, pos, *playerEntity);
            }

            // 派发自定义物品组件回调 - onMineBlock
            auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
            std::string itemTypeId = toolItem->itemLocation().toString();
            if (itemCompReg.hasMineBlockCallback(itemTypeId)) {
                mc::mod::bedrock::addon::ItemComponentMineBlockEvent mineEvent;
                mineEvent.itemTypeId = itemTypeId;
                mineEvent.sourceId = playerId;
                mineEvent.blockX = pos.x;
                mineEvent.blockY = pos.y;
                mineEvent.blockZ = pos.z;
                mineEvent.blockTypeId = blockTypeId;
                mineEvent.minedBlockPermutationTypeId = blockTypeId;
                mineEvent.itemStackAmount = tool.getCount();
                itemCompReg.dispatchMineBlock(itemTypeId, mineEvent);
            }
        }
    }

    // 设置为空气
    u32 newBlockStateId = _setBlockToAir(*world, pos, oldState, playerId);

    return BlockBreakResult{true, newBlockStateId, "Block destroyed"};
}

void BlockInteractionManager::setOnBlockBreak(
    std::function<void(PlayerId, const BlockPos&, const BlockState&)> callback)
{
    m_onBlockBreak = std::move(callback);
}

void BlockInteractionManager::setOnBlockPlace(
    std::function<void(PlayerId, const BlockPos&, const BlockState&)> callback)
{
    m_onBlockPlace = std::move(callback);
}

bool BlockInteractionManager::_canInteract(PlayerId playerId, const BlockPos& pos) const noexcept
{
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData) {
        return false;
    }

    // 计算距离（玩家眼睛位置到方块中心）
    const f64 eyeX = playerData->x;
    const f64 eyeY = playerData->y + static_cast<f64>(game::PLAYER_EYE_HEIGHT);
    const f64 eyeZ = playerData->z;
    const f64 targetX = pos.x + 0.5;
    const f64 targetY = pos.y + 0.5;
    const f64 targetZ = pos.z + 0.5;

    const f64 dx = targetX - eyeX;
    const f64 dy = targetY - eyeY;
    const f64 dz = targetZ - eyeZ;
    const f64 distanceSquared = dx * dx + dy * dy + dz * dz;

    // 最大交互距离 6 格
    constexpr f64 MAX_INTERACT_DISTANCE_SQ = 36.0; // 6 * 6
    return distanceSquared <= MAX_INTERACT_DISTANCE_SQ;
}

bool BlockInteractionManager::_canBreakBlock(
    ServerWorld& world, PlayerId playerId, const BlockPos& pos, const BlockState* state) const noexcept
{
    MC_UNUSED(world);
    if (!state || state->isAir() || state->hardness() < 0.0f) {
        return false;
    }
    return _canInteract(playerId, pos);
}

bool BlockInteractionManager::_wouldCollideWithPlayer(
    PlayerId playerId, const BlockPos& placePos, const BlockState& state) const noexcept
{
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (playerData == nullptr) {
        return false;
    }

    const auto& collisionShape = state.getCollisionShape();
    if (collisionShape.isEmpty()) {
        return false;
    }

    const f32 halfWidth = Player::PLAYER_WIDTH * 0.5f;
    const AxisAlignedBB playerBoundingBox(playerData->x - halfWidth,
        playerData->y,
        playerData->z - halfWidth,
        playerData->x + halfWidth,
        playerData->y + Player::PLAYER_HEIGHT,
        playerData->z + halfWidth);

    return collisionShape.intersects(playerBoundingBox, placePos.x, placePos.y, placePos.z);
}

void BlockInteractionManager::_generateBlockDrops(
    ServerWorld& world, const BlockPos& pos, const BlockState& state, PlayerId playerId, const ItemStack* tool)
{
    MC_TRACE_EVENT("server.world",
        "BlockInteractionManager::generateBlockDrops",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    Player* player = _getPlayerEntity(playerId, world);

    // 使用 BlockDropHandler 生成掉落物
    auto drops = BlockDropHandler::generateDrops(world, pos, state, player, tool, m_lootTableManager);

    if (!drops.empty()) {
        const std::string throwerUuid = player != nullptr ? player->uuid() : std::string();
        BlockDropHandler::spawnDrops(world, pos, drops, throwerUuid);
    }

    // 处理矿石经验掉落
    // 使用随机种子生成器
    const u64 seed = static_cast<u64>(static_cast<u64>(pos.x)) ^ static_cast<u64>(static_cast<u64>(pos.y) << 1) ^
        static_cast<u64>(static_cast<u64>(pos.z) << 2) ^ static_cast<u64>(std::hash<PlayerId>{}(playerId));
    math::Random rng(seed);
    BlockDropHandler::handleBlockBreakExperience(world, pos, state, tool, rng);
}

bool BlockInteractionManager::_handleSignCommand(ServerWorld& world, const BlockPos& pos, mc::ServerPlayer& player)
{
    // 当玩家右键点击告示牌时，执行告示牌上的命令

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::Sign) {
        return false;
    }

    auto* signEntity = static_cast<blockentity::SignEntity*>(blockEntity);
    if (!signEntity) {
        return false;
    }

    // 使用 SignCommandHelper 执行命令
    return SignCommandHelper::executeSignCommands(*signEntity, player);
}

} // namespace mc::server::interaction
