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

#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/EffectTriggers.hpp"
#include "common/advancement/trigger/impl/InventoryChangedTrigger.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/player/SleepManager.hpp"
#include "common/entity/player/SpawnPointValidator.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/packet/BlockEntityDataPacket.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/packet/SetCameraPacket.hpp"
#include "common/network/packet/SignPackets.hpp"
#include "common/network/packet/SleepPacket.hpp"
#include "common/network/packet/TitlePacket.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "common/world/storage/player/PlayerDataManager.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "server/advancement/TriggerInstantiation.hpp"
#include "server/application/IServer.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc {

ServerPlayer::ServerPlayer(EntityInstanceId id, const std::string& name)
    : Player(id, name)
{
    initAdvancements();
    setupInventoryCallback();
}

void ServerPlayer::initAdvancements()
{
    m_advancements = std::make_shared<server::PlayerAdvancements>(static_cast<PlayerId>(id()));
    m_advancements->setServerPlayer(this);
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

    // 设置末影箱物品栏变更回调，标记玩家数据为脏以触发自动保存
    enderChestInventory().setOnChanged([this]() {
        // m_server 在 ServerPlayerEntityManager::createPlayerEntity 中通过 setServer 注入。
        // 测试环境或尚未完成初始化时 m_server 可能为 nullptr，需做空指针守卫避免崩溃。
        if (m_server == nullptr) {
            return;
        }
        if (auto* storage = getServer()->sharedStorage()) {
            if (auto* pdm = storage->playerDataManager()) {
                pdm->markDirty(uuid());
            }
        }
    });
}

void ServerPlayer::sendChatMessage(const std::string& message)
{
    // 1.21.11 无独立 S→C chat IR 包（玩家聊天经 PlayerChatMessage/SystemChat）。
    // TODO(Phase6): 对齐 1.21.11 PlayerChatMessage/SystemChat codec 后再发送。
    (void)message;
    spdlog::debug("ServerPlayer: S->C chat dropped (no chat IR yet, player={})", username());
}

void ServerPlayer::sendSystemMessage(const std::string& message)
{
    // 1.21.11 无独立 S→C chat IR 包。
    // TODO(Phase6): 对齐 1.21.11 SystemChat codec 后再发送。
    (void)message;
    spdlog::debug("ServerPlayer: S->C system message dropped (no chat IR yet, player={})", username());
}

void ServerPlayer::openSignEditor(const BlockPos& pos, bool isFrontSide)
{
    // 先发送告示牌当前的 BlockEntity 数据给客户端，确保编辑器打开时能显示已有文本
    // 对应 MC Java: SignBlock.openTextEdit() 前，客户端通过区块数据已持有 BlockEntity
    if (m_world != nullptr) {
        const BlockEntity* entity = m_world->getBlockEntity(pos);
        if (entity != nullptr) {
            nbt::CompoundTag tag = entity->getUpdateTag();
            std::vector<u8> nbtData = network::BlockEntityDataPacket::serializeNbtToBytes(tag);
            if (!nbtData.empty()) {
                // 1.21.11 BlockEntityData：blockPosPacked + blockEntityType + tag(NBT opaque)。
                // TODO(Phase6): tag 透传旧 NBT 字节，未对齐 1.21.11 CompoundTag codec。
                mc::network::ir::play::BlockEntityData bePkt;
                bePkt.blockPosPacked = pos.asLong();
                bePkt.blockEntityType = static_cast<i32>(entity->getType());
                bePkt.tag = std::move(nbtData);
                if (!_sendIrPacket(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                        mc::network::ir::PlayPacket{std::move(bePkt)}})) {
                    spdlog::warn(
                        "ServerPlayer: block entity data packet not sent (player={}, no connection)", username());
                }
            }
        }
    }

    // 1.21.11 OpenSignEditor：blockPosPacked + isFrontText。
    mc::network::ir::play::OpenSignEditor pkt;
    pkt.blockPosPacked = pos.asLong();
    pkt.isFrontText = isFrontSide;
    if (!_sendIrPacket(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)}})) {
        spdlog::warn("ServerPlayer: open sign editor packet not sent (player={}, no connection)", username());
    }
}

void ServerPlayer::sendStatusMessage(const std::string& message, bool actionBar)
{
    // actionBar 参数用于控制消息显示位置：
    // - actionBar = true: 显示在物品栏上方的 Action Bar 区域
    // - actionBar = false: 显示在聊天区域

    if (!hasConnection()) {
        return;
    }

    if (actionBar) {
        // 1.21.11 SetActionBarText：text 为 Component opaque。
        // TODO(Phase6): text 仅以 JSON 字符串字节承载，未对齐 1.21.11 ComponentType codec。
        mc::network::ir::play::SetActionBarText pkt;
        pkt.text = std::vector<u8>(message.begin(), message.end());
        static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)}}));
    } else {
        // 发送到聊天区域
        sendSystemMessage(message);
    }
}

void ServerPlayer::syncExperience()
{
    // 1.21.11 SetExperience：experienceProgress + experienceLevel + totalExperience。
    mc::network::ir::play::SetExperience pkt;
    pkt.experienceProgress = experienceProgress();
    pkt.experienceLevel = experienceLevel();
    pkt.totalExperience = totalExperience();

    if (!_sendIrPacket(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)}})) {
        spdlog::warn("ServerPlayer: experience sync skipped (player={}, no connection)", username());
    }
}

bool ServerPlayer::sendVelocityPacket()
{
    if (!hasConnection()) {
        return false;
    }

    // 1.21.11 SetEntityMotion：entityId + LpVec3(速度)。
    // 速度单位：IR 直接用 m/tick（f64），codec 内做 LpVec3 编码；旧协议用 1/8000 截断，此处不再截断。
    mc::network::ir::play::SetEntityMotion pkt;
    pkt.entityId = static_cast<i32>(id());
    const auto vel = velocity();
    pkt.x = static_cast<f64>(vel.x);
    pkt.y = static_cast<f64>(vel.y);
    pkt.z = static_cast<f64>(vel.z);

    if (!_sendIrPacket(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)}})) {
        spdlog::warn("ServerPlayer: velocity packet not sent (player={}, no connection)", username());
        return false;
    }

    return true;
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

void ServerPlayer::awardUsedStat(const ResourceLocation& itemId, i32 count)
{
    m_statistics.increment(server::stats::StatType::Used, itemId, count);
}

void ServerPlayer::awardCraftedStat(const ResourceLocation& itemId, i32 count)
{
    m_statistics.incrementCrafted(itemId, count);
}

void ServerPlayer::awardCustomStat(const ResourceLocation& statId, i32 count)
{
    m_statistics.incrementCustom(statId, count);
}

void ServerPlayer::onEquippedItemBroken(const Item& item, EquipmentSlot slot)
{
    // 基类实现：广播装备破损动画 + 播放音效
    Player::onEquippedItemBroken(item, slot);

    // 玩家额外更新物品损坏统计
    // 对应 MC 原版 ServerPlayer.onEquippedItemBroken() 中的 awardStat(Stats.ITEM_BROKEN)
    m_statistics.incrementBroken(item.itemLocation());
}

void ServerPlayer::onItemCrafted(ItemStack& stack, i32 amount)
{
    // 更新合成统计
    if (!stack.isEmpty() && stack.getItem() != nullptr) {
        // 获取物品的资源位置
        const ResourceLocation& itemId = stack.getItem()->itemLocation();
        awardCraftedStat(itemId, amount);

        // 调用物品合成回调（地图缩放/锁定等后处理）
        stack.onCraftedBy(*this, amount);
    }
}

void ServerPlayer::unlockRecipe(const ResourceLocation& recipeId)
{
    // 触发配方解锁成就
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

    // 更新配方书
    m_recipeBook.unlock(recipeId);
    m_recipeBook.markNew(recipeId);
}

size_t ServerPlayer::unlockRecipes(const std::vector<ResourceLocation>& recipes)
{
    return m_recipeBook.add(recipes.begin(), recipes.end(), [this](const ResourceLocation& recipeId) {
        // 触发成就
        if (m_advancements != nullptr) {
            auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::RecipeUnlockedTrigger>();
            if (trigger != nullptr) {
                trigger->trigger(
                    *m_advancements, [&recipeId](const advancement::RecipeUnlockedTriggerInstance& instance) {
                        return instance.test(recipeId);
                    });
            }
        }
    });
}

size_t ServerPlayer::lockRecipes(const std::vector<ResourceLocation>& recipes)
{
    return m_recipeBook.remove(recipes.begin(), recipes.end());
}

// ========== 睡眠系统实现 ==========

entity::SleepResult ServerPlayer::trySleep(const BlockPos& bedPos)
{
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
    i64 currentTime = world->dayTimeOfDay();

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
    _sendSleepPacket(bedPos);

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

    // 发送唤醒包给客户端
    _sendWakeUpPacket();

    // 清除床的占用状态，并计算起床位置
    if (bedPos.has_value() && m_world != nullptr) {
        const BlockState* bedState = m_world->getBlockState(bedPos.value());

        // 在 setBlockState 之前提取所需属性值，避免悬挂指针
        bool hasOccupied = (bedState != nullptr && bedState->hasProperty(BlockStateProperties::OCCUPIED()));
        bool hasFacing = (bedState != nullptr && bedState->hasProperty(BlockStateProperties::HORIZONTAL_FACING()));
        Direction bedFacing = hasFacing ? bedState->get(BlockStateProperties::HORIZONTAL_FACING()) : Direction::None;

        // 清除床的占用状态
        if (hasOccupied) {
            BlockState newBedState = bedState->with(BlockStateProperties::OCCUPIED(), false);
            m_world->setBlockState(bedPos.value(), &newBedState, 3);
        }

        // 使用 BedBlock::findStandUpPosition 计算起床位置
        if (hasFacing) {
            Vector3 wakePos = blocks::BedBlock::findStandUpPosition(*m_world, bedPos.value(), bedFacing, yaw());

            // 计算面向床的方向（yaw）：从起床位置指向床底中心的方向
            Vector3d bedCenter(bedPos.value().x + 0.5, bedPos.value().y, bedPos.value().z + 0.5);
            Vector3d dirToBed = bedCenter - Vector3d(wakePos.x, wakePos.y, wakePos.z);
            f32 dirLen = std::sqrt(dirToBed.x * dirToBed.x + dirToBed.z * dirToBed.z);
            if (dirLen > 0.001) {
                dirToBed.x /= dirLen;
                dirToBed.z /= dirLen;
                f32 yawDeg = static_cast<f32>(math::toDegrees(std::atan2(dirToBed.z, dirToBed.x))) - 90.0f;
                yawDeg = math::wrapDegrees(yawDeg);
                setRotation(yawDeg, 0.0f);
            }

            setPosition(wakePos.x, wakePos.y, wakePos.z);
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
    }

    // 2. 默认返回主世界
    return DimensionId(0);
}

void ServerPlayer::_sendSleepPacket(const BlockPos& bedPos)
{
    if (!hasConnection()) {
        return;
    }

    // 1.21.11 睡眠走实体元数据 Pose 序列号（无独立 Sleep 包）。
    // TODO(Phase6): 对齐 1.21.11 EntityMetadata（SynchedEntityData）后用 Pose=SLEEPING 同步；
    //   当前无元数据 IR，暂以 EntityEvent 自定义 event 字节承载（我方互通）。
    MC_UNUSED(bedPos);
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = static_cast<i32>(id());
    pkt.eventId = 46; // 自定义：玩家开始睡觉（MC Java EntityEvent 无此值，仅我方互通用）
    static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)}}));
}

void ServerPlayer::_sendWakeUpPacket()
{
    if (!hasConnection()) {
        return;
    }

    // 1.21.11 起床走实体元数据 Pose 还原（无独立 Sleep 包）。
    // TODO(Phase6): 对齐 1.21.11 EntityMetadata 后用 Pose=STANDING 同步。
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = static_cast<i32>(id());
    pkt.eventId = 47; // 自定义：玩家起床（仅我方互通用）
    static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)}}));
}

bool ServerPlayer::_sendIrPacket(mc::network::ir::IrPacket packet) const
{
    if (!hasConnection()) {
        return false;
    }

    auto result = m_connection->send(std::move(packet));
    if (!result.success()) {
        spdlog::warn("ServerPlayer: IR packet send failed (player={}): {}", username(), result.error().message());
        return false;
    }
    return true;
}

// ========== 维度传送实现 ==========

bool ServerPlayer::onPortalTriggered()
{
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

void ServerPlayer::onInsideBlock(const BlockState& blockState)
{
    // 触发 EnterBlockTrigger 成就
    if (m_world == nullptr) {
        return;
    }

    // 检查方块是否为空气
    if (blockState.isAir()) {
        return;
    }

    // 获取当前位置
    BlockPos pos(static_cast<i32>(std::floor(m_position.x)),
        static_cast<i32>(std::floor(m_position.y)),
        static_cast<i32>(std::floor(m_position.z)));

    // 发布 EnterBlockEvent
    m_world->onEnterBlock(static_cast<PlayerId>(id()), pos, &blockState);
}

bool ServerPlayer::changeDimension(DimensionId targetDim)
{
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
        for (EntityInstanceId passengerId : passengers) {
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

    // 下界传送：搜索已存在的传送门，找不到则创建
    // 末地传送：固定位置
    if (targetDim == DimensionManager::THE_END) {
        // 末地传送：固定出生位置
        targetPos = Teleporter::getEndSpawnPosition();

        // 创建末地出生平台（黑曜石平台和清空空间）
        ServerDimension* targetDimension = m_server->dimensionManager().getDimension(targetDim);
        if (targetDimension != nullptr && targetDimension->world() != nullptr) {
            EndTeleporter::createEndSpawnPlatform(*targetDimension->world());
        }
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

        // 更新 m_world 指针到目标维度的 ServerWorld
        ServerDimension* targetDimension = m_server->dimensionManager().getDimension(targetDim);
        if (targetDimension != nullptr) {
            setWorld(targetDimension->world());
        }
    }

    return success;
}

// ========== 队伍系统实现 ==========

scoreboard::Team* ServerPlayer::getTeam()
{
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
    // 通过服务器的记分板获取玩家所在队伍
    if (m_server == nullptr) {
        return nullptr;
    }

    // 获取服务器的记分板
    const server::ServerScoreboard& serverScoreboard = m_server->scoreboard();
    return serverScoreboard.getPlayersTeam(username());
}

scoreboard::Scoreboard* ServerPlayer::getScoreboard()
{
    if (m_server == nullptr) {
        return nullptr;
    }
    return &m_server->scoreboard();
}

const scoreboard::Scoreboard* ServerPlayer::getScoreboard() const
{
    if (m_server == nullptr) {
        return nullptr;
    }
    return &m_server->scoreboard();
}

bool ServerPlayer::canHarmPlayer(const Player& target) const
{
    // 检查 PvP 游戏规则
    if (m_world != nullptr && !m_world->isPvpAllowed()) {
        return false;
    }
    // 委托给基类检查队伍友伤规则
    return Player::canHarmPlayer(target);
}

bool ServerPlayer::hurt(DamageSource& source, f32 amount)
{
    // PvP 保护检查：如果伤害来源是玩家，检查攻击者能否伤害本玩家
    Entity* sourceEntity = source.getEntity();
    if (sourceEntity != nullptr) {
        Player* attackingPlayer = dynamic_cast<Player*>(sourceEntity);
        if (attackingPlayer != nullptr && attackingPlayer != this) {
            if (!attackingPlayer->canHarmPlayer(*this)) {
                return false;
            }
        }
    }

    // 委托给基类处理（创造模式无敌检查等）
    return Player::hurt(source, amount);
}

void ServerPlayer::indicateDamage(f64 d0, f64 d1)
{
    // 基类设置 m_hurtDir；服务端额外广播受伤动画包（携带 hurtDir）给追踪者与受害者自己。
    Player::indicateDamage(d0, d1);
    if (m_world != nullptr) {
        m_world->broadcastHurtAnimation(m_id, m_hurtDir);
    }
}

void ServerPlayer::attack(Entity& target)
{
    // 旁观者模式下攻击实体等同于设置旁观目标
    // 对应 MC Java: ServerPlayer.attack() -> this.setCamera(p_9220_)
    if (isSpectator()) {
        setCamera(&target);
        return;
    }

    // 非旁观者模式：正常攻击
    Player::attack(target);
}

// ========== 旁观者跟踪系统实现 ==========

void ServerPlayer::tick()
{
    Player::tick();
    tickSpectator();
}

bool ServerPlayer::setCamera(Entity* target)
{
    // 设置新的 camera 目标
    // setCameraEntityId() 会触发 onCameraEntityChanged()，后者负责传送和发送 SetCameraPacket
    if (target != nullptr) {
        setCameraEntityId(target->id());
    } else {
        // nullptr 表示恢复自身视角
        setCameraEntityId(std::nullopt);
    }

    return true;
}

void ServerPlayer::onCameraEntityChanged(
    std::optional<EntityInstanceId> oldCameraId, std::optional<EntityInstanceId> newCameraId)
{
    // 当摄像机目标变更时：
    // 1. 如果有新的旁观目标，将玩家传送到目标实体位置
    // 2. 发送 SetCameraPacket 给客户端以同步摄像机状态
    // 对应 MC Java: ServerPlayer.setCamera() 的传送和发包逻辑

    if (newCameraId.has_value()) {
        // 传送到新的旁观目标实体位置
        if (m_world != nullptr) {
            Entity* target = m_world->getEntity(newCameraId.value());
            if (target != nullptr) {
                setPosition(target->position());
                setRotation(target->yaw(), target->pitch());
                snapshotInterpolationState();
            }
        }
    }

    // 发送 SetCameraPacket 给客户端
    // cameraEntityId 为玩家自身 ID 时表示恢复正常视角
    u32 cameraId = newCameraId.value_or(static_cast<EntityInstanceId>(id()));
    _sendSetCameraPacket(cameraId);

    spdlog::info("ServerPlayer: player {} spectating entity {}",
        username(),
        newCameraId.has_value() ? static_cast<i32>(newCameraId.value()) : -1);
}

void ServerPlayer::resetCamera()
{
    if (isSpectating()) {
        setCamera(nullptr);
    }
}

void ServerPlayer::tickSpectator()
{
    // 仅在旁观者模式下且有旁观目标时处理
    if (!isSpectator() || !isSpectating()) {
        return;
    }

    EntityInstanceId cameraEntityId = getCameraEntityId().value();

    // 获取目标实体
    if (m_world == nullptr) {
        resetCamera();
        return;
    }

    Entity* target = m_world->getEntity(cameraEntityId);
    if (target == nullptr || target->isRemoved()) {
        // 目标实体已消失或死亡，停止旁观
        resetCamera();
        return;
    }

    // 每tick将旁观者位置同步到目标实体位置
    // 使用 absSnapTo 语义：同时更新 prevPosition 和 position，避免插值动画
    setPosition(target->position());
    setRotation(target->yaw(), target->pitch());
    snapshotInterpolationState();

    // 检查玩家是否按住潜行键，如果是则停止旁观
    if (isInputSneaking()) {
        resetCamera();
    }
}

void ServerPlayer::_sendSetCameraPacket(u32 cameraEntityId)
{
    if (!hasConnection()) {
        return;
    }

    // 1.21.11 SetCamera：cameraId（VarInt）。
    mc::network::ir::play::SetCamera pkt;
    pkt.cameraId = static_cast<i32>(cameraEntityId);

    if (!_sendIrPacket(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)}})) {
        spdlog::warn("ServerPlayer: SetCamera packet not sent (player={}, no connection)", username());
    }
}

} // namespace mc
