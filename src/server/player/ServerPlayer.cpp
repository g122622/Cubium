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

#include "ServerPlayer.hpp"

#include "../advancement/PlayerAdvancements.hpp"
#include "../advancement/TriggerInstantiation.hpp"
#include "../application/IServer.hpp"
#include "../application/MinecraftServer.hpp"
#include "../core/ConnectionManager.hpp"
#include "../dimension/ServerDimension.hpp"
#include "../dimension/ServerDimensionManager.hpp"
#include "../event/ServerEventBus.hpp"
#include "../event/events/ServerEvents.hpp"
#include "../world/ServerWorld.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/EffectTriggers.hpp"
#include "common/advancement/trigger/impl/InventoryChangedTrigger.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/player/SleepManager.hpp"
#include "common/entity/player/SpawnPointValidator.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/packet/SleepPacket.hpp"
#include "common/network/packet/TitlePacket.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include <spdlog/spdlog.h>

namespace mc {

ServerPlayer::ServerPlayer(EntityId id, const std::string& name)
    : Player(id, name)
{
    initAdvancements();
    setupInventoryCallback();
}

void ServerPlayer::initAdvancements()
{
    m_advancements = std::make_shared<server::PlayerAdvancements>(static_cast<PlayerId>(id()));
}

void ServerPlayer::setupInventoryCallback()
{
    // 设置物品栏变更回调，用于触发成就检测
    inventory().setChangeCallback([this](i32 slot, const ItemStack& oldItem, const ItemStack& newItem) {
        // 发布 InventoryChangedEvent
        server::event::InventoryChangedEvent event{0, // timestamp，需要从 world 获取
            static_cast<PlayerId>(id()),
            &inventory(),
            slot,
            oldItem.isEmpty() ? nullptr : &oldItem,
            newItem.isEmpty() ? nullptr : &newItem};
        server::event::ServerEventBus::instance().publish(event);
    });
}

void ServerPlayer::sendChatMessage(const std::string& message)
{
    network::ChatMessagePacket chatPacket(message, static_cast<PlayerId>(id()));
    network::PacketSerializer payload;
    chatPacket.serialize(payload);

    const auto fullPacket =
        server::core::ConnectionManager::encapsulatePacket(network::PacketType::ChatBroadcast, payload.buffer());

    if (!sendFullPacket(fullPacket)) {
        spdlog::debug("ServerPlayer: chat message not sent (player={}, no connection)", username());
    }
}

void ServerPlayer::sendSystemMessage(const std::string& message)
{
    network::ChatMessagePacket chatPacket(message, 0);
    network::PacketSerializer payload;
    chatPacket.serialize(payload);

    const auto fullPacket =
        server::core::ConnectionManager::encapsulatePacket(network::PacketType::ChatBroadcast, payload.buffer());

    if (!sendFullPacket(fullPacket)) {
        spdlog::warn("ServerPlayer: system message not sent (player={}, no connection)", username());
    }
}

void ServerPlayer::sendStatusMessage(const std::string& message, bool actionBar)
{
    // 参考 MC 1.16.5 PlayerEntity.sendStatusMessage(ITextComponent, boolean)
    // actionBar 参数用于控制消息显示位置：
    // - actionBar = true: 显示在物品栏上方的 Action Bar 区域
    // - actionBar = false: 显示在聊天区域

    if (!hasConnection()) {
        spdlog::debug("ServerPlayer: status message not sent (player={}, no connection)", username());
        return;
    }

    if (actionBar) {
        // 使用 TitlePacket 的 Actionbar 类型显示在 Action Bar 区域
        // 参考 MC 1.16.5: SChatPacket(message, ChatType.GAME_INFO, UUID)
        // 本项目使用 TitlePacket::Actionbar 实现相同效果
        network::TitlePacket packet = network::TitlePacket::createActionbar(message);
        auto payloadResult = packet.serialize();
        if (payloadResult.failed()) {
            spdlog::warn("ServerPlayer: failed to serialize actionbar packet (player={})", username());
            return;
        }

        const auto fullPacket =
            server::core::ConnectionManager::encapsulatePacket(network::PacketType::Title, payloadResult.value());

        if (!sendFullPacket(fullPacket)) {
            spdlog::debug("ServerPlayer: actionbar message not sent (player={}, no connection)", username());
        }
    } else {
        // 发送到聊天区域
        sendSystemMessage(message);
    }
}

void ServerPlayer::syncExperience()
{
    const auto payloadResult = network::SetExperiencePacket::fromPlayer(*this).serialize();
    if (payloadResult.failed()) {
        spdlog::warn("ServerPlayer: failed to serialize experience packet (player={})", username());
        return;
    }

    const auto fullPacket =
        server::core::ConnectionManager::encapsulatePacket(network::PacketType::SetExperience, payloadResult.value());

    if (!sendFullPacket(fullPacket)) {
        spdlog::warn("ServerPlayer: experience sync skipped (player={}, no connection)", username());
    }
}

void ServerPlayer::addExperience(i32 amount)
{
    Player::addExperience(amount);
    syncExperience();
}

void ServerPlayer::setExperienceLevel(i32 level)
{
    Player::setExperienceLevel(level);
    syncExperience();
}

void ServerPlayer::addExperienceLevels(i32 levels)
{
    Player::addExperienceLevels(levels);
    syncExperience();
}

bool ServerPlayer::consumeExperience(i32 amount)
{
    bool result = Player::consumeExperience(amount);
    if (result) {
        syncExperience();
    }
    return result;
}

bool ServerPlayer::consumeExperienceLevels(i32 levels)
{
    bool result = Player::consumeExperienceLevels(levels);
    if (result) {
        syncExperience();
    }
    return result;
}

void ServerPlayer::setExperience(i32 level, f32 progress, i32 totalExperience)
{
    Player::setExperience(level, progress, totalExperience);
    syncExperience();
}

// ========== 统计系统实现 ==========

void ServerPlayer::awardCraftedStat(const ResourceLocation& itemId, i32 count)
{
    m_statistics.incrementCrafted(itemId, count);
}

void ServerPlayer::onItemCrafted(ItemStack& stack, i32 amount)
{
    // MC 1.16.5: 更新合成统计
    if (!stack.isEmpty() && stack.getItem() != nullptr) {
        // 获取物品的资源位置
        const ResourceLocation& itemId = stack.getItem()->itemLocation();
        awardCraftedStat(itemId, amount);

        // MC 1.16.5: 调用 Item.onCreated（地图等物品的特殊初始化）
        // 注意：Item::onCreated 方法在当前项目中尚未实现
        // 如果需要，可以在 Item 类中添加虚方法 onCreated(ItemStack, IWorld, Player)
        // stack.getItem()->onCreated(stack, getWorld(), *this);
    }
}

void ServerPlayer::unlockRecipe(const ResourceLocation& recipeId)
{
    // MC 1.16.5: 触发配方解锁成就
    // 参考: net.minecraft.advancements.CriteriaTriggers.RECIPE_UNLOCKED.trigger(player, recipe)
    if (m_advancements != nullptr) {
        auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::RecipeUnlockedTrigger>();
        if (trigger != nullptr) {
            // 使用 AbstractCriterionTrigger::trigger() 模板方法
            // 通过 TriggerInstantiation.hpp 中定义的实现
            trigger->trigger(*m_advancements, [&recipeId](const advancement::RecipeUnlockedTriggerInstance& instance) {
                return instance.test(recipeId);
            });
        }
    }

    // TODO: 更新配方书（当配方书系统实现后）
    // m_recipeBook.unlock(recipeId);
}

// ========== 睡眠系统实现 ==========

entity::SleepResult ServerPlayer::trySleep(const BlockPos& bedPos)
{
    // 参考 MC 1.16.5 ServerPlayerEntity.trySleep()

    // 1. 检查是否已经在睡眠
    if (isSleeping()) {
        return entity::SleepResult::OTHER_PROBLEM;
    }

    // 检查玩家是否存活
    if (isDead()) {
        return entity::SleepResult::OTHER_PROBLEM;
    }

    // 获取世界引用（server::ServerWorld* 可隐式转换为 IWorld*）
    if (m_world == nullptr) {
        return entity::SleepResult::OTHER_PROBLEM;
    }
    IWorld* world = m_world;

    // 2. 检查维度是否允许睡眠
    DimensionType dimType = DimensionType::fromId(world->dimension());
    if (!dimType.bedWorks()) {
        // 在下界或末地，床会爆炸（由 BedBlock 处理）
        return entity::SleepResult::NOT_POSSIBLE_HERE;
    }

    // 获取床的朝向（从床的方块状态获取）
    const BlockState* bedState = world->getBlockState(bedPos);
    if (bedState == nullptr || !bedState->hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        return entity::SleepResult::OTHER_PROBLEM;
    }
    Direction bedFacing = bedState->get(BlockStateProperties::HORIZONTAL_FACING());

    // 3. 检查距离床是否太远（水平 3 格，垂直 2 格）
    Vector3 playerPos(position().x, position().y, position().z);
    if (!entity::SleepManager::isPlayerNearBed(playerPos, bedPos)) {
        return entity::SleepResult::TOO_FAR_AWAY;
    }

    // 4. 检查床是否被阻挡
    if (entity::SleepManager::isBedObstructed(*world, bedPos, bedFacing)) {
        return entity::SleepResult::OBSTRUCTED;
    }

    // 5. 设置重生点
    setSpawnPoint(world->dimension(), bedPos, false);

    // 6. 检查时间是否允许睡眠
    bool isThundering = world->isThundering();
    bool isRaining = world->isRaining();
    i64 currentTime = world->dayTime();

    if (!entity::SleepManager::canSleepAtTime(currentTime, isThundering, isRaining)) {
        return entity::SleepResult::NOT_POSSIBLE_NOW;
    }

    // 7. 非创造模式检查周围怪物
    if (!abilities().creativeMode) {
        if (entity::SleepManager::isBedSurroundedByMonsters(*world, bedPos, *this)) {
            return entity::SleepResult::NOT_SAFE;
        }
    }

    // 8. 开始睡眠
    startSleeping(bedPos);

    // 9. 发送睡眠包给客户端
    sendSleepPacket(bedPos);

    // 10. 更新世界睡眠标志
    if (m_world != nullptr) {
        m_world->updateAllPlayersSleepingFlag();
    }

    spdlog::info("ServerPlayer: player {} started sleeping at ({}, {}, {})", username(), bedPos.x, bedPos.y, bedPos.z);

    return entity::SleepResult::OK;
}

void ServerPlayer::stopSleepInBed(bool resetTimer, bool updateSleepingFlag)
{
    if (!isSleeping()) {
        return;
    }

    // 获取床位置用于后续处理
    std::optional<BlockPos> bedPos = getSleepingPosition();

    // 停止睡眠（这会清除睡眠状态和位置）
    stopSleeping();

    // 设置计时器
    if (resetTimer) {
        setSleepTimer(0);
    } else {
        setSleepTimer(100); // 用于唤醒动画
    }

    // [网络同步] 发送唤醒包给客户端
    sendWakeUpPacket();

    // 同步玩家位置
    if (bedPos.has_value() && m_world != nullptr) {
        // 计算起床位置
        const BlockState* bedState = m_world->getBlockState(bedPos.value());
        if (bedState != nullptr && bedState->hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
            Direction bedFacing = bedState->get(BlockStateProperties::HORIZONTAL_FACING());
            std::optional<Vector3> wakePos =
                entity::SleepManager::findWakeUpPosition(*m_world, bedPos.value(), bedFacing);
            if (wakePos.has_value()) {
                setPosition(wakePos.value());
            }
        }
    }

    // 更新世界睡眠标志
    if (updateSleepingFlag && m_world != nullptr) {
        m_world->updateAllPlayersSleepingFlag();
    }

    spdlog::info("ServerPlayer: player {} stopped sleeping", username());
}

void ServerPlayer::wakeUp()
{
    stopSleepInBed(true, true);
}

// ========== 重生系统实现 ==========

Vector3d ServerPlayer::determineRespawnPosition() const
{
    // 参考 MC 1.16.5 PlayerList.func_232644_a_ (respawnPlayer)
    // 1. 检查玩家个人重生点
    auto spawnPoint = getSpawnPoint();
    if (spawnPoint.has_value()) {
        // 获取重生点维度对应的世界
        DimensionId spawnDimId = spawnPoint->getDimensionId();
        const BlockPos& spawnPos = spawnPoint->getPos();
        bool spawnForced = isSpawnForced();

        // 尝试获取对应维度的世界
        IWorld* spawnWorld = nullptr;
        if (m_server != nullptr) {
            ServerDimension* spawnDimension = m_server->dimensionManager().getDimension(spawnDimId);
            if (spawnDimension != nullptr) {
                spawnWorld = spawnDimension->world();
            }
        }

        if (spawnWorld != nullptr) {
            // 验证重生点是否有效
            // 参考 MC 1.16.5 PlayerEntity.func_242374_a_
            SpawnPointValidationResult validationResult =
                SpawnPointValidator::validate(*spawnWorld, spawnPoint.value(), spawnForced, true);

            if (validationResult == SpawnPointValidationResult::Valid) {
                // 重生点有效，查找安全的生成位置
                auto safePos =
                    SpawnPointValidator::findSafeSpawnPosition(*spawnWorld, spawnPoint.value(), spawnForced, true);

                if (safePos.has_value()) {
                    const Vector3& pos = safePos.value();
                    return Vector3d(static_cast<f64>(pos.x), static_cast<f64>(pos.y), static_cast<f64>(pos.z));
                }
            }

            // 重生点无效，清除它并发送消息
            // 参考 MC 1.16.5: 发送 SChangeGameStatePacket.field_241764_a_
            spdlog::info("ServerPlayer: spawn point invalid for player {} (reason: {}), falling back to world spawn",
                username(),
                static_cast<i32>(validationResult));

            // 如果重生点无效，直接清除它（防止每次重生都检查）
            // 注意：这里使用 const_cast 是因为此方法是 const 的
            // 但清除重生点是一个必要的副作用
            const_cast<ServerPlayer*>(this)->clearSpawnPoint();
        }
    }

    // 2. 使用世界出生点
    if (m_world != nullptr) {
        return m_world->worldSpawnPoint();
    }

    // 3. 默认位置
    return Vector3d(0.0, static_cast<f64>(world::SEA_LEVEL) + 1.0, 0.0);
}

DimensionId ServerPlayer::determineRespawnDimension() const
{
    // 参考 MC 1.16.5 PlayerList.func_232644_a_ (respawnPlayer)
    // 1. 检查玩家个人重生点的维度
    auto spawnPoint = getSpawnPoint();
    if (spawnPoint.has_value()) {
        DimensionId spawnDimId = spawnPoint->getDimensionId();
        bool spawnForced = isSpawnForced();

        // 尝试获取对应维度的世界进行验证
        IWorld* spawnWorld = nullptr;
        if (m_server != nullptr) {
            ServerDimension* spawnDimension = m_server->dimensionManager().getDimension(spawnDimId);
            if (spawnDimension != nullptr) {
                spawnWorld = spawnDimension->world();
            }
        }

        if (spawnWorld != nullptr) {
            // 验证重生点
            SpawnPointValidationResult validationResult =
                SpawnPointValidator::validate(*spawnWorld, spawnPoint.value(), spawnForced, true);

            if (validationResult == SpawnPointValidationResult::Valid) {
                return spawnDimId;
            }
        }

        // 重生点无效，返回主世界
        // 注意：MC 1.16.5 中，如果重生点无效，玩家会在主世界重生
    }

    // 2. 默认返回主世界
    return DimensionId(0);
}

void ServerPlayer::sendSleepPacket(const BlockPos& bedPos)
{
    if (!hasConnection()) {
        return;
    }

    network::SleepPacket sleepPacket(static_cast<u32>(id()), bedPos);
    auto payloadResult = sleepPacket.serialize();
    if (payloadResult.failed()) {
        spdlog::warn("ServerPlayer: failed to serialize sleep packet (player={})", username());
        return;
    }

    const auto fullPacket =
        server::core::ConnectionManager::encapsulatePacket(network::PacketType::Sleep, payloadResult.value());

    static_cast<void>(sendFullPacket(fullPacket));
}

void ServerPlayer::sendWakeUpPacket()
{
    if (!hasConnection()) {
        return;
    }

    network::SleepPacket wakeUpPacket = network::SleepPacket::createWakeUp(static_cast<u32>(id()));
    auto payloadResult = wakeUpPacket.serialize();
    if (payloadResult.failed()) {
        spdlog::warn("ServerPlayer: failed to serialize wake up packet (player={})", username());
        return;
    }

    const auto fullPacket =
        server::core::ConnectionManager::encapsulatePacket(network::PacketType::Sleep, payloadResult.value());

    static_cast<void>(sendFullPacket(fullPacket));
}

bool ServerPlayer::sendFullPacket(const std::vector<u8>& packet) const
{
    if (!hasConnection()) {
        return false;
    }

    m_connection->send(packet.data(), packet.size());
    return true;
}

// ========== 维度传送实现 ==========

bool ServerPlayer::onPortalTriggered()
{
    // 参考 MC 1.16.5 ServerPlayerEntity.tickPortal()
    // 当传送门触发时，确定目标维度并传送

    // 获取当前维度
    DimensionId currentDim = dimension();

    // 确定目标维度
    // 主世界 <-> 下界，末地 -> 主世界
    DimensionId targetDim;
    switch (currentDim) {
        case DimensionManager::NETHER:
            targetDim = DimensionManager::OVERWORLD;
            break;
        case DimensionManager::OVERWORLD:
            targetDim = DimensionManager::NETHER;
            break;
        case DimensionManager::THE_END:
            targetDim = DimensionManager::OVERWORLD;
            break;
        default:
            // 未知维度，不传送
            spdlog::warn("ServerPlayer: unknown dimension {}, cannot teleport", currentDim);
            return false;
    }

    // 执行传送
    return changeDimension(targetDim);
}

bool ServerPlayer::changeDimension(DimensionId targetDim)
{
    // 参考 MC 1.16.5 ServerPlayerEntity.changeDimension()

    if (m_server == nullptr) {
        spdlog::warn("ServerPlayer: cannot change dimension, no server reference");
        return false;
    }

    if (isRiding()) {
        stopRiding();
    }

    // 清除乘客（复制列表以避免迭代时修改）
    if (hasPassengers()) {
        auto passengers = getPassengers(); // 复制
        for (EntityId passengerId : passengers) {
            if (m_world != nullptr) {
                if (Entity* passenger = m_world->getEntity(passengerId)) {
                    passenger->stopRiding();
                }
            }
        }
    }

    DimensionId currentDim = dimension();
    Vector3d currentPos(position().x, position().y, position().z);

    // 计算目标位置（坐标转换）
    Vector3d targetPos =
        Teleporter::transformPosition(currentPos, DimensionType::fromId(currentDim), DimensionType::fromId(targetDim));

    // 根据 MC 1.16.5 逻辑：
    // 下界传送：搜索已存在的传送门，找不到则创建
    // 末地传送：固定位置
    if (targetDim == DimensionManager::THE_END) {
        // 末地传送：固定出生位置
        targetPos = Teleporter::getEndSpawnPosition();
    } else {
        // 下界/主世界传送：搜索传送门
        // 获取目标维度的世界
        ServerDimension* targetDimension = m_server->dimensionManager().getDimension(targetDim);
        if (targetDimension != nullptr && targetDimension->world() != nullptr) {
            IWorld* targetWorld = targetDimension->world();

            // 根据目标维度选择传送器
            if (targetDim == DimensionManager::NETHER || currentDim == DimensionManager::NETHER) {
                // 使用下界传送器
                NetherTeleporter teleporter;

                // 先尝试查找已存在的传送门
                auto portalInfo = teleporter.findPortal(*targetWorld, targetPos);

                if (portalInfo.has_value() && portalInfo->valid) {
                    // 找到已存在的传送门，使用其位置
                    targetPos = portalInfo->position;
                    spdlog::debug("ServerPlayer: found existing portal at ({:.1f}, {:.1f}, {:.1f})",
                        targetPos.x,
                        targetPos.y,
                        targetPos.z);
                } else {
                    // 没找到传送门，创建新传送门
                    PortalInfo newPortal = teleporter.createPortal(*targetWorld, targetPos);
                    if (newPortal.valid) {
                        targetPos = newPortal.position;
                        // 记录传送门位置
                        BlockPos portalBlock(math::floorTo<BlockCoord>(targetPos.x),
                            math::floorTo<BlockCoord>(targetPos.y),
                            math::floorTo<BlockCoord>(targetPos.z));
                        targetDimension->recordPortalPosition(portalBlock);
                        spdlog::info("ServerPlayer: created new portal at ({:.1f}, {:.1f}, {:.1f})",
                            targetPos.x,
                            targetPos.y,
                            targetPos.z);
                    }
                }
            }
        }
        // 如果无法获取目标世界，使用转换后的坐标（容错）
    }

    // 重置传送门状态
    setInPortal(false);
    resetPortalTime();
    triggerPortalCooldown();

    spdlog::info("ServerPlayer: {} teleporting from dimension {} to {} at ({:.1f}, {:.1f}, {:.1f})",
        username(),
        currentDim,
        targetDim,
        targetPos.x,
        targetPos.y,
        targetPos.z);

    // 通过 ServerDimensionManager 执行实际的维度切换
    PlayerId playerId = static_cast<PlayerId>(id());
    bool success = m_server->dimensionManager().transferPlayerToDimension(playerId, targetDim, targetPos);

    if (success) {
        // 更新实体的维度属性
        setDimension(targetDim);
        setPosition(static_cast<f32>(targetPos.x), static_cast<f32>(targetPos.y), static_cast<f32>(targetPos.z));
    }

    return success;
}

// ========== 队伍系统实现 ==========

scoreboard::Team* ServerPlayer::getTeam()
{
    // 参考 MC 1.16.5 Entity.getTeam()
    // 通过服务器的记分板获取玩家所在队伍
    if (m_server == nullptr) {
        return nullptr;
    }

    // 获取服务器的记分板
    server::ServerScoreboard& serverScoreboard = m_server->scoreboard();
    // ServerScoreboard 继承自 Scoreboard，可以直接调用 getPlayersTeam
    return serverScoreboard.getPlayersTeam(username());
}

const scoreboard::Team* ServerPlayer::getTeam() const
{
    // 参考 MC 1.16.5 Entity.getTeam()
    // 通过服务器的记分板获取玩家所在队伍
    if (m_server == nullptr) {
        return nullptr;
    }

    // 获取服务器的记分板
    const server::ServerScoreboard& serverScoreboard = m_server->scoreboard();
    return serverScoreboard.getPlayersTeam(username());
}

} // namespace mc
