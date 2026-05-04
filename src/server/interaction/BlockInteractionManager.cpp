#include "BlockInteractionManager.hpp"
#include "InventoryManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/drop/BlockDropHandler.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace mc::server::interaction {

// TODO 有不少方法中的逻辑重复，考虑抽离公共逻辑以减少重复代码
BlockInteractionManager::BlockInteractionManager(
    ServerWorld& world,
    core::PlayerManager& playerManager,
    loot::LootTableManager& lootTableManager)
    : m_world(world)
    , m_playerManager(playerManager)
    , m_lootTableManager(lootTableManager)
{
}

void BlockInteractionManager::setInventoryManager(InventoryManager* inventoryManager)
{
    m_inventoryManager = inventoryManager;
}

Result<BlockInteractionResult> BlockInteractionManager::handleBlockInteraction(
    PlayerId playerId,
    const BlockPos& pos,
    network::BlockInteractionAction action)
{
    MC_TRACE_EVENT("server.world",
        "BlockInteractionManager::handleBlockInteraction",
        "pos", pos.toString(),
        "playerId", playerId,
        "action", static_cast<u8>(action),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    // 获取玩家数据
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData || !playerData->loggedIn) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 验证距离
    if (!canInteract(playerId, pos)) {
        return Error(ErrorCode::InvalidArgument, "Block too far away");
    }

    // 验证 Y 范围
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return Error(ErrorCode::InvalidArgument, "Block Y out of range");
    }

    // 获取方块状态
    const BlockState* state = m_world.getBlockState(pos);
    if (!state || state->isAir()) {
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
            if (canBreakBlock(playerId, pos, state)) {
                // 获取手持物品作为工具
                ItemStack tool;
                if (m_inventoryManager) {
                    tool = m_inventoryManager->getHeldItem(playerId);
                }

                // 生成掉落物
                generateBlockDrops(pos, *state, playerId, tool.isEmpty() ? nullptr : &tool);

                // 设置为空气
                Block* airBlock = Block::getBlock(ResourceLocation("minecraft:air"));
                if (airBlock) {
                    m_world.setBlock(pos, &airBlock->defaultState());

                    if (m_onBlockBreak) {
                        m_onBlockBreak(playerId, pos, *state);
                    }
                }

                return BlockInteractionResult{true, "Block destroyed"};
            }
            break;

        default:
            break;
    }

    return BlockInteractionResult{false, "Action not handled"};
}

Result<BlockPlacementResult> BlockInteractionManager::handleBlockPlacement(
    PlayerId playerId,
    const BlockPos& pos,
    const Vector3& hitPos,
    Direction face,
    const ItemStack& heldItem)
{
    MC_TRACE_EVENT("server.world",
        "BlockInteractionManager::handleBlockPlacement",
        "pos", pos.toString(),
        "playerId", playerId,
        "face", Directions::toString(face),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    // 调试世界禁止方块放置
    if (m_world.isDebugWorld()) {
        return Error(ErrorCode::PermissionDenied, "Cannot place blocks in debug world");
    }

    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData || !playerData->loggedIn) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 验证距离
    if (!canInteract(playerId, pos)) {
        return Error(ErrorCode::InvalidArgument, "Block too far away");
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
    BlockItemUseContext context(m_world, nullptr, heldItem, hitPos, pos, face, playerData->yaw);

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
    if (wouldCollideWithPlayer(playerId, placePos, *newState)) {
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
        itemConsumed = true;  // 创造模式不实际消耗
    }

    if (m_onBlockPlace) {
        m_onBlockPlace(playerId, placePos, *newState);
    }

    return BlockPlacementResult{true, true, itemConsumed, placePos, newState->stateId(), "Block placed"};
}

Result<BlockInteractionResult> BlockInteractionManager::handleBlockUse(
    PlayerId playerId,
    const BlockPos& pos,
    Hand hand,
    const Vector3& hitPos,
    Direction face)
{
    MC_TRACE_EVENT("server.world",
        "BlockInteractionManager::handleBlockUse",
        "pos", pos.toString(),
        "playerId", playerId,
        "hand", hand == Hand::MainHand ? "main" : "off",
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData || !playerData->loggedIn) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    if (!canInteract(playerId, pos)) {
        return Error(ErrorCode::InvalidArgument, "Block too far away");
    }

    const BlockState* state = m_world.getBlockState(pos);
    if (!state || state->isAir()) {
        return BlockInteractionResult{false, "No block to use"};
    }

    Block* block = Block::getBlock(state->blockId());
    if (!block) {
        return Error(ErrorCode::NotFound, "Block not found for state");
    }

    Player interactionPlayer(playerId, playerData->username);
    const BlockRaycastResult hitResult = BlockRaycastResult::hit(hitPos, pos, face, 0.0f);

    ActionResultType result = block->onBlockActivated(
        *state,
        m_world,
        pos,
        interactionPlayer,
        hand,
        hitResult);

    const bool handled = (result == ActionResultType::Success || result == ActionResultType::Consume);
    return BlockInteractionResult{handled, handled ? "Block used" : "Block use pass"};
}

Result<BlockBreakResult> BlockInteractionManager::handleBlockBreak(
    PlayerId playerId,
    const BlockPos& pos)
{
    MC_TRACE_EVENT("server.world",
        "BlockInteractionManager::handleBlockBreak",
        "pos", pos.toString(),
        "playerId", playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    // 调试世界禁止方块破坏
    if (m_world.isDebugWorld()) {
        return Error(ErrorCode::PermissionDenied, "Cannot break blocks in debug world");
    }

    // 获取玩家数据
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData || !playerData->loggedIn) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 验证距离
    if (!canInteract(playerId, pos)) {
        return Error(ErrorCode::InvalidArgument, "Block too far away");
    }

    // 验证 Y 范围
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return Error(ErrorCode::InvalidArgument, "Block Y out of range");
    }

    // 获取方块状态
    const BlockState* state = m_world.getBlockState(pos);
    if (!state || state->isAir()) {
        return BlockBreakResult{false, 0, "No block to break"};
    }

    const BlockState oldState = *state;

    // 检查是否可破坏
    if (!canBreakBlock(playerId, pos, state)) {
        return BlockBreakResult{false, 0, "Cannot break this block"};
    }

    // 获取手持物品作为工具
    ItemStack tool;
    if (m_inventoryManager) {
        tool = m_inventoryManager->getHeldItem(playerId);
    }

    // 生成掉落物
    generateBlockDrops(pos, oldState, playerId, tool.isEmpty() ? nullptr : &tool);

    // 设置为空气
    Block* airBlock = Block::getBlock(ResourceLocation("minecraft:air"));
    u32 newBlockStateId = airBlock ? airBlock->defaultState().stateId() : 0;

    if (airBlock) {
        m_world.setBlock(pos, &airBlock->defaultState());

        if (m_onBlockBreak) {
            m_onBlockBreak(playerId, pos, oldState);
        }
    }

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

bool BlockInteractionManager::canInteract(PlayerId playerId, const BlockPos& pos) const
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

bool BlockInteractionManager::canBreakBlock(
    PlayerId playerId,
    const BlockPos& pos,
    const BlockState* state) const
{
    if (!state || state->isAir() || state->hardness() < 0.0f) {
        return false;
    }
    return canInteract(playerId, pos);
}

bool BlockInteractionManager::wouldCollideWithPlayer(
    PlayerId playerId,
    const BlockPos& placePos,
    const BlockState& state) const
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
    const AxisAlignedBB playerBoundingBox(
        playerData->x - halfWidth,
        playerData->y,
        playerData->z - halfWidth,
        playerData->x + halfWidth,
        playerData->y + Player::PLAYER_HEIGHT,
        playerData->z + halfWidth);

    return collisionShape.intersects(playerBoundingBox, placePos.x, placePos.y, placePos.z);
}

void BlockInteractionManager::generateBlockDrops(
    const BlockPos& pos,
    const BlockState& state,
    PlayerId playerId,
    const ItemStack* tool)
{
    // 使用 BlockDropHandler 生成掉落物
    auto drops = BlockDropHandler::generateDrops(
        m_world, pos, state, nullptr, tool, m_lootTableManager);

    if (!drops.empty()) {
        BlockDropHandler::spawnDrops(
            m_world,
            pos,
            drops,
            "");
    }

    // 处理矿石经验掉落
    // 使用随机种子生成器
    const u64 seed = static_cast<u64>(static_cast<u64>(pos.x)) ^
                     static_cast<u64>(static_cast<u64>(pos.y) << 1) ^
                     static_cast<u64>(static_cast<u64>(pos.z) << 2) ^
                     static_cast<u64>(std::hash<PlayerId>{}(playerId));
    math::Random rng(seed);
    BlockDropHandler::handleBlockBreakExperience(
        m_world,
        pos,
        state,
        tool,
        rng);
}

} // namespace mc::server::interaction