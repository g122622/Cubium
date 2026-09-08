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

#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/EffectTriggers.hpp"
#include "common/advancement/trigger/impl/InventoryChangedTrigger.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/tamable/TameableEntity.hpp"
#include "common/entity/interfaces/IAngerable.hpp"
#include "common/entity/player/SleepManager.hpp"
#include "common/entity/player/SleepResult.hpp"
#include "common/entity/player/SpawnPointValidator.hpp"
#include "common/entity/serialization/EntityDeserializer.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/scoreboard/criteria/DeathCountCriteria.hpp"
#include "common/scoreboard/criteria/KillCountCriteria.hpp"
#include "common/stats/Stats.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "common/world/storage/player/PlayerDataManager.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "server/application/IServer.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/stats/StatType.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {

ServerPlayer::ServerPlayer(EntityInstanceId id, const std::string& name, ecs::EntityRegistry& registry)
    : Player(id, name, registry)
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
    // 1.21.11 玩家聊天经 PlayerChatMessage（含签名/会话）下发；离线模式无签名链路，
    // 降级用 SystemChat(overlay=false) 把裸文本送入聊天窗口。真在线签名聊天属独立子项。
    if (!hasConnection()) {
        spdlog::debug("ServerPlayer: S->C chat dropped (no connection, player={})", username());
        return;
    }
    mc::network::ir::play::SystemChat pkt;
    pkt.content = ::mc::text::plainTextToNbtBytes(message);
    pkt.overlay = false; // 聊天窗口
    static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}}));
}

void ServerPlayer::sendSystemMessage(const std::string& message)
{
    // 1.21.11 SystemChat(overlay=false)：content 为 Component NBT，显示在聊天窗口。
    if (!hasConnection()) {
        spdlog::debug("ServerPlayer: S->C system message dropped (no connection, player={})", username());
        return;
    }
    mc::network::ir::play::SystemChat pkt;
    pkt.content = ::mc::text::plainTextToNbtBytes(message);
    pkt.overlay = false; // 聊天窗口
    static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}}));
}

void ServerPlayer::openSignEditor(const BlockPos& pos, bool isFrontSide)
{
    // 先发送告示牌当前的 BlockEntity 数据给客户端，确保编辑器打开时能显示已有文本
    // 对应 MC Java: SignBlock.openTextEdit() 前，客户端通过区块数据已持有 BlockEntity
    if (m_world != nullptr) {
        const BlockEntity* entity = m_world->getBlockEntity(pos);
        if (entity != nullptr) {
            // 1.21.11 BlockEntityData：blockPosPacked + blockEntityType + CompoundTag（无长度前缀）。
            auto tag = std::make_shared<nbt::CompoundTag>(entity->getUpdateTag());
            mc::network::ir::play::BlockEntityData bePkt;
            bePkt.blockPosPacked = pos.asLong();
            bePkt.blockEntityType = static_cast<i32>(entity->getType());
            bePkt.tag = std::move(tag);
            if (!_sendIrPacket(mc::network::ir::IrPacket{
                    mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(bePkt)}})) {
                spdlog::warn("ServerPlayer: block entity data packet not sent (player={}, no connection)", username());
            }
        }
    }

    // 1.21.11 OpenSignEditor：blockPosPacked + isFrontText。
    mc::network::ir::play::OpenSignEditor pkt;
    pkt.blockPosPacked = pos.asLong();
    pkt.isFrontText = isFrontSide;
    if (!_sendIrPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}})) {
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
        // 1.21.11 SetActionBarText：text 为 Component NBT（自定界，无外层 VarInt 长度）。
        // 裸字符串经 plainTextToNbtBytes 折叠为 StringTag，对齐 vanilla 纯文本 Component。
        mc::network::ir::play::SetActionBarText pkt;
        pkt.text = ::mc::text::plainTextToNbtBytes(message);
        static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}}));
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

    if (!_sendIrPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}})) {
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

    if (!_sendIrPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}})) {
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

    // 全员睡眠判定由 MinecraftServer::tick 每帧跨维度聚合轮询（checkAllPlayersSleeping），
    // 此处不再主动通知世界重算睡眠标志。

    spdlog::info("ServerPlayer: player {} started sleeping at ({}, {}, {})", username(), bedPos.x, bedPos.y, bedPos.z);

    return entity::SleepResult::OK;
}

void ServerPlayer::stopSleepInBed(bool resetTimer)
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

    // 全员睡眠判定由 MinecraftServer::tick 每帧跨维度聚合轮询（checkAllPlayersSleeping），
    // 此处不再主动通知世界重算睡眠标志。

    spdlog::info("ServerPlayer: player {} stopped sleeping", username());
}

void ServerPlayer::wakeUp()
{
    stopSleepInBed(true);
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
    // 降级保留：完整 1.21.11 EntityMetadata（SynchedEntityData）体系未实现，无法发 Pose=
    // SLEEPING 元数据。当前以 EntityEvent 自定义 event 字节承载（仅我方双端互通，MC Java
    // EntityEvent 无此值）。真 Java 客户端不会收到睡眠可视化，待 EntityMetadata 体系落地
    // 后改发 Pose 元数据。此降级不阻塞离线互通基线。
    MC_UNUSED(bedPos);
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = static_cast<i32>(id());
    pkt.eventId = 46; // 自定义：玩家开始睡觉（MC Java EntityEvent 无此值，仅我方互通用）
    static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}}));
}

void ServerPlayer::_sendWakeUpPacket()
{
    if (!hasConnection()) {
        return;
    }

    // 1.21.11 起床走实体元数据 Pose 还原（无独立 Sleep 包）。
    // 降级保留：同 _sendSleepPacket，完整 EntityMetadata 体系未实现，当前以 EntityEvent
    // 自定义 event 字节承载（仅我方互通）。待 EntityMetadata 落地后改发 Pose=STANDING。
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = static_cast<i32>(id());
    pkt.eventId = 47; // 自定义：玩家起床（仅我方互通用）
    static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}}));
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
    BlockPos pos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));

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

    return _performDimensionTransfer(targetDim, targetPos);
}

bool ServerPlayer::teleportToDimension(DimensionId targetDim, const Vector3d& pos, const Vector2f& rot)
{
    if (m_server == nullptr) {
        spdlog::warn("ServerPlayer: cannot teleportToDimension, no server reference");
        return false;
    }

    // 对齐 vanilla Entity.teleportTo → teleport → teleportCrossDimension：
    // 先 stopRiding/ejectPassengers，再迁移。与 changeDimension 的乘客处理一致。
    if (isRiding()) {
        stopRiding();
    }
    if (hasPassengers()) {
        auto passengers = getPassengers();
        for (EntityInstanceId passengerId : passengers) {
            if (m_world != nullptr) {
                if (Entity* passenger = m_world->getEntity(passengerId)) {
                    passenger->stopRiding();
                }
            }
        }
    }

    DimensionId currentDim = dimension();

    // 同维度：不走跨维度迁移，直接同维度 setPosition/setRotation（对齐 vanilla teleportSameDimension）。
    // /tp 同维度坐标已有 TeleportCommand::teleportPlayers 同维度路径处理，此处仅兜底 teleportToDimension
    // 被同维度调用的情形（理论上 TeleportCommand 仅在跨维度时调此）。
    if (currentDim == targetDim) {
        setPosition(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
        setRotation(rot.x, rot.y);
        return true;
    }

    // 跨维度：复用 changeDimension 的迁移逻辑，但用 /tp 命令显式坐标（不调 Teleporter）。
    // 重置传送门状态（与 changeDimension 一致，避免迁移后立即触发门冷却逻辑）。
    setInPortal(false);
    resetPortalTime();
    triggerPortalCooldown();

    spdlog::info("ServerPlayer: {} tp cross-dimension from {} to {} at ({:.1f}, {:.1f}, {:.1f})",
        username(),
        currentDim,
        targetDim,
        pos.x,
        pos.y,
        pos.z);

    if (!_performDimensionTransfer(targetDim, pos)) {
        return false;
    }

    // /tp 朝向由命令指定（changeDimension 不改朝向，保持原朝向）。迁移完成后设置。
    setRotation(rot.x, rot.y);
    return true;
}

bool ServerPlayer::_performDimensionTransfer(DimensionId targetDim, const Vector3d& targetPos)
{
    // 通过 ServerDimensionManager 执行实际的维度切换
    // 缺陷C修复：用 playerId()（Player.hpp:146，返回 m_playerId；SimulatedPlayer 占位 0）
    // 而非 id()（EntityInstanceId，由 EntityManager 分配，值很大）。transferPlayerToDimension
    // 内 getPlayerDimension/playerLeaveDimension 按 PlayerId 索引，用 id() 会查不到致链路空转。
    bool success = m_server->dimensionManager().transferPlayerToDimension(playerId(), targetDim, targetPos);

    if (success) {
        // 迁移前 m_world 仍指向源世界（transferPlayerToDimension 不改 m_world）
        mc::server::ServerWorld* sourceWorld = m_world;

        // 更新实体的维度属性
        setDimension(targetDim);
        setPosition(static_cast<f32>(targetPos.x), static_cast<f32>(targetPos.y), static_cast<f32>(targetPos.z));

        // 更新 m_world 指针到目标维度的 ServerWorld
        ServerDimension* targetDimension = m_server->dimensionManager().getDimension(targetDim);
        mc::server::ServerWorld* targetWorld = (targetDimension != nullptr) ? targetDimension->world() : nullptr;
        if (targetWorld != nullptr) {
            setWorld(targetWorld);
        }

        // 缺陷B修复：迁移 EntityManager 归属。否则实体 m_world 指向目标世界但对象仍留源
        // ServerWorld.EntityManager，源世界 tick 仍 tick 它并读目标世界方块 -> 数据不一致。
        //
        // unique_ptr move 不移动 Entity 对象（仅转移所有权），故 JS 侧 / 其他裸指针持有者仍有效。
        // 顺序：先 setDimension/setPosition/setWorld 再迁移，使迁移期间实体字段已是目标维度语义。
        //
        // 崩溃修复：changeDimension 可能在 entity->tick() 调用栈内被触发（doBlockCollisions
        // → EndPortalBlock::onEntityCollision → changeDimension）。此时源 EntityManager 的
        // _tickEntities 正遍历 m_entities 并持有当前迭代器，同步调 removeEntity 会 erase 当前
        // 节点，for 循环 ++it 解引用失效迭代器→SIGSEGV。故把 removeEntity+spawnEntity 封装为
        // 延迟回调，通过 requestDimensionTransfer 入队源 EntityManager，由 tick() 在 _tickEntities
        // 遍历完成后（m_scheduler.tick 返回后）统一执行。此时 erase 当前节点安全（遍历已结束）。
        if (sourceWorld != nullptr && targetWorld != nullptr && sourceWorld != targetWorld) {
            EntityInstanceId entityId = id();
            // 捕获源/目标 EntityManager 指针（ServerWorld 生命周期 >= ServerPlayer，安全）。
            EntityManager& sourceEntityManager = sourceWorld->entityManager();
            EntityManager& targetEntityManager = targetWorld->entityManager();
            sourceEntityManager.requestDimensionTransfer([entityId, &sourceEntityManager, &targetEntityManager]() {
                auto entityPtr = sourceEntityManager.removeEntity(entityId);
                if (entityPtr != nullptr) {
                    // addEntity 内部再次 setEntityManager(this)（幂等），并向目标空间索引登记。
                    // 若实体 ID 在目标 EntityManager 已被占用，addEntity 会分配新 ID 并 setId；
                    // 跨维度迁移保留原 ID 是常态（各 EntityManager 的 ID 空间独立，碰撞罕见）。
                    [[maybe_unused]] EntityInstanceId newId = targetEntityManager.addEntity(std::move(entityPtr));
                } else {
                    // removeEntity 失败：实体不在源 EntityManager（理论上不该发生，因 m_world 指向源世界）。
                    // 记日志但不回滚——transferPlayerToDimension 已更新 m_playerDimensions，回滚会引入
                    // 更严重的不一致。
                    spdlog::warn("ServerPlayer::_performDimensionTransfer: removeEntity returned null for "
                                 "entity {} during dimension migration",
                        entityId);
                }
            });
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

void ServerPlayer::die(DamageSource& cause)
{
    // 对齐 MC Java 1.21.11 ServerPlayer.die（ServerPlayer.java:879-939）。
    // 玩家死亡需在 LivingEntity::die 通用逻辑之上，补充玩家特有处理：
    // 死亡消息广播、统计更新、状态清除、最后死亡位置记录。
    // 通用死亡逻辑（gameEvent(ENTITY_DIE)、dropAllDeathLoot、broadcastEntityState(3)、
    // setPose(DYING)、clearFire、setLastDeathLocation）由 Player::die → LivingEntity::die 承载。

    // 1. gameEvent(ENTITY_DIE) + dropAllDeathLoot + broadcastEntityState(3) + setPose(DYING)
    //    + clearFire + setLastDeathLocation（由 Player::die → LivingEntity::die 完成）
    Player::die(cause);

    // 2. SHOW_DEATH_MESSAGES 游戏规则检查（ServerPlayer.java:881-910）
    if (m_world != nullptr) {
        const bool showDeathMessages =
            m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::SHOW_DEATH_MESSAGES);

        // 死亡消息：从 CombatTracker 取 getDeathMessage()。
        std::string deathMessage = combatTracker().getDeathMessage();

        // 对齐 vanilla ServerPlayer.die（ServerPlayer.java:882-909）：
        //   flag ? send KillPacket(getId(), getDeathMessage()) + 按队伍广播
        //        : send KillPacket(getId(), EMPTY)
        // 无论 showDeathMessages 与否，都发送 ClientboundPlayerCombatKillPacket 驱动客户端死亡画面。
        mc::network::ir::play::PlayerCombatKill killPkt;
        killPkt.playerId = static_cast<i32>(id());
        killPkt.message = ::mc::text::plainTextToNbtBytes(showDeathMessages ? deathMessage : std::string{});
        static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(killPkt)}}));

        // 对齐 vanilla ServerPlayer.die（ServerPlayer.java:900-907）：
        //   Team team = this.getTeam();
        //   if (team == null || team.getDeathMessageVisibility() == ALWAYS)
        //       broadcastSystemMessage(component, false);
        //   else if (visibility == HIDE_FOR_OTHER_TEAMS)
        //       broadcastSystemToTeam(this, component);
        //   else if (visibility == HIDE_FOR_OWN_TEAM)
        //       broadcastSystemToAllExceptTeam(this, component);
        // Cubium 无 PlayerList，这里通过 playerManager().forEachPlayer(...) 遍历在线玩家，
        // 对每个 ServerPlayerData 用 scoreboard.getPlayersTeam(username) 判断同队，
        // 直接构造 SystemChat 包经 playerData.send(...) 下发（等价于 sendSystemMessage）。
        if (showDeathMessages && m_server != nullptr) {
            scoreboard::Team* deadTeam = getTeam();
            const scoreboard::TeamVisibility visibility =
                deadTeam != nullptr ? deadTeam->getDeathMessageVisibility() : scoreboard::TeamVisibility::Always;

            // 死亡消息 Component NBT wire 字节（复用 SystemChat.content 的构造方式）。
            const std::vector<u8> deathMessageNbt = ::mc::text::plainTextToNbtBytes(deathMessage);
            const PlayerId selfPlayerId = playerId();

            // 广播判定（对齐 PlayerList.broadcastSystemMessage/ToTeam/ToAllExceptTeam）：
            //   ALWAYS / team==null → 发给所有在线玩家（含死亡玩家自己，原版行为）
            //   HIDE_FOR_OTHER_TEAMS → 仅发给同队玩家，排除死亡玩家自己
            //                          （原版 broadcastSystemToTeam 用 serverplayer != this 排除）
            //   HIDE_FOR_OWN_TEAM    → 仅发给非同队玩家
            //                          （死亡玩家自己因同队被 getTeam() != team 自然排除）
            //   NEVER                → 不发给任何玩家（三分支均不匹配）
            m_server->playerManager().forEachPlayer(
                [m_server = m_server, &deathMessageNbt, deadTeam, visibility, selfPlayerId](
                    server::ServerPlayerData& playerData) {
                    bool shouldSend = false;
                    if (visibility == scoreboard::TeamVisibility::Always) {
                        shouldSend = true;
                    } else if (visibility == scoreboard::TeamVisibility::HideForOtherTeams) {
                        // 仅同队可见，且排除死亡玩家自己（对齐 broadcastSystemToTeam 的 != this）。
                        shouldSend = deadTeam != nullptr && deadTeam->hasMember(playerData.username) &&
                            playerData.playerId != selfPlayerId;
                    } else if (visibility == scoreboard::TeamVisibility::HideForOwnTeam) {
                        // 仅非同队可见：接收者队伍 != 死亡玩家队伍。
                        // 死亡玩家自己 receiverTeam == deadTeam，自然被排除。
                        scoreboard::Team* receiverTeam = m_server->scoreboard().getPlayersTeam(playerData.username);
                        shouldSend = receiverTeam != deadTeam;
                    }
                    // NEVER：shouldSend 保持 false。

                    if (!shouldSend) {
                        return;
                    }

                    mc::network::ir::play::SystemChat chatPkt;
                    chatPkt.content = deathMessageNbt;
                    chatPkt.overlay = false;
                    static_cast<void>(
                        playerData.send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                            mc::network::ir::PlayPacket{std::move(chatPkt)}}));
                });
        }
    }

    // 3. removeEntitiesOnShoulder()（ServerPlayer.java:912）
    removeEntitiesOnShoulder();

    // 4. FORGIVE_DEAD_PLAYERS → tellNeutralMobsThatIDied()（ServerPlayer.java:913-915）
    if (m_world != nullptr && m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::FORGIVE_DEAD_PLAYERS)) {
        tellNeutralMobsThatIDied();
    }

    // 5. dropAllDeathLoot 已由 LivingEntity::die 处理（ServerPlayer.java:918 在基类处理）

    // 6. forAllObjectives(DEATH_COUNT)（ServerPlayer.java:921）
    // 对齐原版：this.serverScoreboard.forAllObjectives(ObjectiveCriteria.DEATH_COUNT, this, ScoreAccess::add)
    // Cubium 采用方案 A：直接调用 DeathCountCriteria::onPlayerDeath。
    // 原因：Scoreboard::forAllObjectives 内部用 getScore（仅查找不创建），
    // 首次死亡不会创建分数条目。而 DeathCountCriteria::onPlayerDeath 内部用
    // getOrCreateScore（创建式），天然对齐原版 getOrCreateScore 语义。
    if (auto* scoreboard = getScoreboard()) {
        if (auto* deathCount =
                scoreboard::ScoreCriteriaRegistry::instance().getCriteria(scoreboard::DeathCountCriteria::NAME)) {
            deathCount->onPlayerDeath(username(), *scoreboard);
        }
    }

    // 7. getKillCredit() → awardStat(ENTITY_KILLED_BY) + awardKillScore + createWitherRose
    //    （ServerPlayer.java:922-927）
    // getKillCredit→awardKillScore 已在 LivingEntity::die 第 1 步调用：
    //   LivingEntity* killCredit = getKillCredit();
    //   if (killCredit != nullptr) killCredit->awardKillScore(*this, cause);
    // awardKillScore 由 ServerPlayer 重写，递增 totalKillCount/playerKillCount 判据与
    // PLAYER_KILLS/MOB_KILLS 统计（见 ServerPlayer::awardKillScore）。
    // TODO: createWitherRose（LivingEntity.java:1453）——凋零玫瑰生成逻辑未实现。
    // TODO: awardStat(ENTITY_KILLED_BY)——"被实体击杀"统计，待 ENTITY_KILLED_BY 常量补全后接入。

    // 8. broadcastEntityState((byte)3) 已由 LivingEntity::die 处理（ServerPlayer.java:929 在基类处理）

    // 9. awardStat(DEATHS) + resetStat(TIME_SINCE_DEATH) + resetStat(TIME_SINCE_REST)
    //    （ServerPlayer.java:930-932）
    m_statistics.incrementCustom(ResourceLocation(stats::DEATHS));
    m_statistics.reset(server::stats::StatType::Custom, ResourceLocation(stats::TIME_SINCE_DEATH));
    m_statistics.reset(server::stats::StatType::Custom, ResourceLocation(stats::TIME_SINCE_REST));

    // 10. clearFire() + setTicksFrozen(0) + setSharedFlagOnFire(false)（ServerPlayer.java:933-935）
    //     clearFire 已由 Player::die 处理；此处补充冰冻重置。
    setTicksFrozen(0);
    // TODO: setSharedFlagOnFire(false) — Cubium 无 setSharedFlag，等价 removeFlag(EntityFlags::OnFire)。

    // 11. getCombatTracker().recheckStatus()（ServerPlayer.java:936）
    combatTracker().recheckStatus();

    // 12. setLastDeathLocation 已由 Player::die 处理（ServerPlayer.java:937 在基类处理）

    // 13. connection.markClientUnloadedAfterDeath()（ServerPlayer.java:938）
    // 对齐 vanilla ServerPlayer.die 末尾 this.connection.markClientUnloadedAfterDeath()：
    // 仅置 waitingForRespawn = true。该标志使 hasClientLoaded() 返回 false，
    // 直到玩家执行 PERFORM_RESPAWN 触发 restartClientLoadTimerAfterRespawn() 才被清除。
    // Cubium 中该方法定义在 ServerPlayerData 上（对齐原版 ServerGamePacketListenerImpl），
    // 故通过 playerManager 获取本玩家 ServerPlayerData 后调用。
    if (m_server != nullptr) {
        if (auto* playerData = m_server->playerManager().getPlayer(playerId())) {
            playerData->markClientUnloadedAfterDeath();
        }
    }
}

void ServerPlayer::removeEntitiesOnShoulder()
{
    // 对齐 MC Java 1.21.11 ServerPlayer.removeEntitiesOnShoulder（ServerPlayer.java:808-814）：
    //   if (this.timeEntitySatOnShoulder + 20L < this.level().getGameTime()) {
    //       this.respawnEntityOnShoulder(this.getShoulderEntityLeft());
    //       this.setShoulderEntityLeft(new CompoundTag());
    //       this.respawnEntityOnShoulder(this.getShoulderEntityRight());
    //       this.setShoulderEntityRight(new CompoundTag());
    //   }
    // 守卫避免玩家刚让鹦鹉落肩（timeEntitySatOnShoulder 近期）就被立即生成回世界。
    if (m_world == nullptr) {
        return;
    }
    const u64 gameTime = m_world->getGameTime();
    if (static_cast<u64>(m_timeEntitySatOnShoulder) + 20ULL < gameTime) {
        respawnEntityOnShoulder(m_shoulderEntityLeft);
        m_shoulderEntityLeft = nbt::tags::compound_tag{};
        respawnEntityOnShoulder(m_shoulderEntityRight);
        m_shoulderEntityRight = nbt::tags::compound_tag{};
    }
}

void ServerPlayer::respawnEntityOnShoulder(const nbt::tags::compound_tag& shoulderNbt)
{
    // 对齐 MC Java 1.21.11 ServerPlayer.respawnEntityOnShoulder（ServerPlayer.java:816-832）：
    //   if (!p_446462_.isEmpty()) {
    //       EntityType.create(TagValueInput.create(...), serverlevel, EntitySpawnReason.LOAD)
    //           .ifPresent(p_445299_ -> {
    //               if (p_445299_ instanceof TamableAnimal tamableanimal) {
    //                   tamableanimal.setOwner(this);
    //               }
    //               p_445299_.setPos(this.getX(), this.getY() + 0.7F, this.getZ());
    //               serverlevel.addWithUUID(p_445299_);
    //           });
    //   }
    // Cubium 适配：compound_tag 无 isEmpty()，用 value.empty() 判空。
    // 空检查用 shoulderNbt.value.empty()。
    if (shoulderNbt.value.empty()) {
        return;
    }
    if (m_world == nullptr) {
        return;
    }

    // 从 NBT 反序列化实体（对齐 EntityType.create(TagValueInput.create(...))）。
    // EntityDeserializer::deserialize 接受 const compound_tag& 和 ecs::EntityRegistry&，
    // 返回 Result<unique_ptr<Entity>>。ServerPlayer 无 entityRegistry()，通过 m_world->entityRegistry() 获取。
    auto* registry = m_world->entityRegistry();
    if (registry == nullptr) {
        return;
    }
    auto deserializeResult = entity::serialization::EntityDeserializer::deserialize(shoulderNbt, *registry);
    if (deserializeResult.failed()) {
        // TODO: 对齐原版 ProblemReporter.ScopedCollector 错误收集机制，记录反序列化失败。
        return;
    }
    // Result<T>::value() 返回 T&（左值引用），需 std::move 取出 unique_ptr 所有权。
    auto spawnedEntity = deserializeResult.value();
    if (spawnedEntity == nullptr) {
        return;
    }

    // 若是可驯服实体，设置主人为本玩家（对齐 tamableanimal.setOwner(this)）。
    if (auto* tameable = dynamic_cast<TameableEntity*>(spawnedEntity.get())) {
        tameable->setTamed(true);
        tameable->setOwnerId(uuidBytes());
    }

    // 设置生成位置为玩家上方 0.7 格（对齐 setPos(getX(), getY() + 0.7F, getZ())）。
    spawnedEntity->setPosition(static_cast<f32>(x()), static_cast<f32>(y()) + 0.7F, static_cast<f32>(z()));

    // 生成到世界（对齐 serverlevel.addWithUUID(p_445299_)）。
    // spawnEntity 接受 unique_ptr<Entity>（所有权转移），返回服务端分配的 EntityInstanceId。
    static_cast<void>(m_world->spawnEntity(std::move(spawnedEntity)));
}

void ServerPlayer::tellNeutralMobsThatIDied()
{
    // 对齐 MC Java 1.21.11 ServerPlayer.tellNeutralMobsThatIDied（ServerPlayer.java:820-826）：
    //   AABB aabb = new AABB(this.blockPosition()).inflate(32.0, 10.0, 32.0);
    //   this.level()
    //       .getEntitiesOfClass(Mob.class, aabb, EntitySelector.NO_SPECTATORS)
    //       .stream()
    //       .filter(p_9188_ -> p_9188_ instanceof NeutralMob)
    //       .forEach(p_423216_ -> ((NeutralMob)p_423216_).playerDied(this.level(), this));
    //
    // 原版 NeutralMob.playerDied（NeutralMob.java）：
    //   default void playerDied(ServerLevel p_376731_, Player p_21677_) {
    //       if (p_376731_.getGameRules().get(GameRules.FORGIVE_DEAD_PLAYERS)) {
    //           EntityReference<LivingEntity> entityreference = this.getPersistentAngerTarget();
    //           if (entityreference != null && entityreference.matches(p_21677_)) {
    //               this.stopBeingAngry();
    //           }
    //       }
    //   }
    //
    // Cubium 适配：
    // - blockPosition() → BlockPos(Vector3 position())，对三轴 floor
    // - inflate(32, 10, 32) → AxisAlignedBB::fromBlock(x,y,z).expand(32.0f, 10.0f, 32.0f)
    // - getEntitiesOfClass(Mob, aabb, NO_SPECTATORS) → getEntitiesInAABB(aabb) + 手动过滤
    // - instanceof NeutralMob → dynamic_cast<IAngerable*>（Cubium 用 IAngerable 近似 NeutralMob）
    // - playerDied → IAngerable 已有方法 setAngry(false) + setAttackTarget(nullptr)
    //
    // 注意：原版仅对持久愤怒目标 == 死亡玩家的中立生物调 stopBeingAngry()。
    // Cubium 的 IAngerable 无持久愤怒目标概念（getPersistentAngerTarget/stopBeingAngry 不存在），
    // 此处对范围内所有愤怒中立生物统一清除愤怒——这是与原版的已知偏差。
    if (m_world == nullptr) {
        return;
    }

    // 构造搜索盒：以玩家脚下方块为中心，向各轴扩展 32/10/32。
    const BlockPos playerBlockPos{position()};
    const AxisAlignedBB searchBox =
        AxisAlignedBB::fromBlock(playerBlockPos.x, playerBlockPos.y, playerBlockPos.z).expand(32.0f, 10.0f, 32.0f);

    // 获取范围内所有实体，过滤非旁观者 + 实现 IAngerable 的中立生物。
    // 注意：getEntitiesInAABB 返回碰撞箱与 searchBox 相交的所有实体，无类型过滤。
    const auto nearbyEntities = m_world->getEntitiesInAABB(searchBox);
    for (Entity* entity : nearbyEntities) {
        if (entity == nullptr) {
            continue;
        }
        // 对齐 EntitySelector.NO_SPECTATORS：排除旁观者。
        if (entity->isSpectator()) {
            continue;
        }
        // 对齐 filter(p -> p instanceof NeutralMob)：Cubium 用 IAngerable 近似 NeutralMob。
        if (auto* angry = dynamic_cast<entity::IAngerable*>(entity)) {
            // 对齐 NeutralMob.playerDied → stopBeingAngry()：
            // Cubium IAngerable 无 stopBeingAngry，用 setAngry(false) + setAttackTarget(nullptr) 近似。
            // 仅当该中立生物当前处于愤怒状态时才清除（避免无谓状态变更）。
            if (angry->isAngry()) {
                angry->setAngry(false);
                angry->setAttackTarget(nullptr);
            }
        }
    }
}

void ServerPlayer::awardKillScore(Entity& killedEntity, const DamageSource& source)
{
    // 对齐 MC Java 1.21.11 ServerPlayer.awardKillScore（ServerPlayer.java:950-967）。
    // 击杀者（本玩家）递增各击杀判据目标的分数，并递进统计：
    //   super.awardKillScore(killedEntity, source)            —— 基类空实现
    //   scoreboard.forAllObjectives(KILL_COUNT_ALL, this, inc) —— 总击杀计数
    //   if (killedEntity instanceof Player) {
    //       awardStat(PLAYER_KILLS);
    //       scoreboard.forAllObjectives(KILL_COUNT_PLAYERS, this, inc);
    //   } else {
    //       awardStat(MOB_KILLS);
    //   }
    //   handleTeamKill(this, killedEntity, TEAM_KILL);         —— Cubium 未实现，见 TODO
    //   handleTeamKill(killedEntity, this, KILLED_BY_TEAM);    —— Cubium 未实现，见 TODO
    //   CriteriaTriggers.PLAYER_KILLED_ENTITY.trigger(...)     —— 已由事件系统承担，见注释

    // 基类 LivingEntity::awardKillScore 为空实现（对齐 Entity.awardKillScore），无需显式调用。

    scoreboard::Scoreboard* scoreboard = getScoreboard();
    if (scoreboard == nullptr) {
        return;
    }

    // 取 KILL_COUNT_ALL / KILL_COUNT_PLAYERS 判据实例。
    // 对齐 vanilla ObjectiveCriteria.KILL_COUNT_ALL（"totalKillCount"）与
    // KILL_COUNT_PLAYERS（"playerKillCount"）。
    auto& criteriaRegistry = scoreboard::ScoreCriteriaRegistry::instance();

    // 1. 总击杀计数判据递增（KILL_COUNT_ALL）。
    if (auto* totalKillCount = criteriaRegistry.getCriteria(scoreboard::TotalKillCountCriteria::NAME)) {
        // 注意：Cubium 的 forAllObjectives 内部使用 getScore（仅查找不创建），
        // 与原版 getOrCreateScore（创建语义）不同。这意味着首次击杀时若该玩家在该目标上
        // 尚无分数条目，则不会创建、不会递增——这是与原版的已知偏差。
        // TODO: 对齐原版 getOrCreateScore 语义（将 forAllObjectives 改为创建式查找）。
        scoreboard->forAllObjectives(
            *totalKillCount, username(), [](scoreboard::Score& score) { score.incrementScore(); });
    }

    // 2. 按被杀者类型递进统计与判据。
    const bool killedIsPlayer = (dynamic_cast<Player*>(&killedEntity) != nullptr);
    if (killedIsPlayer) {
        // 击杀玩家：递增 playerKillCount 判据 + PLAYER_KILLS 统计。
        if (auto* playerKillCount = criteriaRegistry.getCriteria(scoreboard::PlayerKillCountCriteria::NAME)) {
            scoreboard->forAllObjectives(
                *playerKillCount, username(), [](scoreboard::Score& score) { score.incrementScore(); });
        }
        awardCustomStat(ResourceLocation(stats::PLAYER_KILLS), 1);
    } else {
        // 击杀生物：递增 MOB_KILLS 统计。
        awardCustomStat(ResourceLocation(stats::MOB_KILLS), 1);
    }

    // 3. 队伍击杀判据递增（handleTeamKill）。
    // 原版 handleTeamKill 按击杀者/被杀者所属队伍颜色，
    // 在 teamkill.{color} / killedByTeam.{color} 判据上递增分数。
    // TODO: Cubium 未实现 TeamKillCriteria / KilledByTeamCriteria 判据注册，
    //       亦无 handleTeamKill 方法。待队伍击杀判据体系落地后补全。
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
    // 对齐 Java ServerPlayer.tick()（ServerPlayer.java:577）：this.connection.tickClientLoadTimeout()
    // 每 tick 递减客户端加载超时计时器。死亡时由 markClientUnloadedAfterDeath 置位 waitingForRespawn，
    // 重生时由 restartClientLoadTimerAfterRespawn 清除 waitingForRespawn 并重启 60 tick 计时器。
    // TODO: restartClientLoadTimerAfterRespawn 的调用点依赖 ServerboundClientCommandPacket(PERFORM_RESPAWN)
    //       包链路，Cubium 当前缺失该包，待补全后接入。
    if (m_server != nullptr) {
        if (auto* playerData = m_server->playerManager().getPlayer(playerId())) {
            playerData->tickClientLoadTimeout();
        }
    }
    Player::tick();
    tickSpectator();

    // 对齐 Java Inventory.tick(this)：每 tick 驱动所有携带物品的 inventoryTick。
    // 这是 FilledMapItem::inventoryTick → _updateMapData → MapData::setColor/markDirty
    // 这条地形上色+置脏链的唯一活驱动点；不接此线，MapData::m_colors 永远全零、
    // isDirty() 首个 tick 后永假，ServerWorld::_pushMapDataToHolders 无真实数据可推。
    // 客户端侧由 FilledMapItem::inventoryTick 的 world.isClientSide() 守卫兜底。
    inventory().tick();

    // 反飞行阈值校验的 tick 末簿记：滚动 firstGood 到 lastGood 作为下一 tick 基线，
    // 并复位移动包计数（known←received，received 在下一包到达时自增）。
    // 对齐 Java ServerGamePacketListenerImpl.tick：knownMovePacketCount = receivedMovePacketCount。
    rollFirstGoodToLastGood();
    syncMovePacketCounters();
    // 载具反飞行基线同步滚动（骑乘时由 handleMoveVehiclePacket 维护 lastGood）。
    rollVehicleFirstGoodToLastGood();

    // 方块变更 ACK 批量发送（对齐 Java ServerGamePacketListenerImpl.tick() :282-286）：
    // 一个 tick 内收到多个带 sequence 的包（use_item_on/use_item/Start/Abort/StopDestroy）时，
    // ackBlockChangesUpTo 取 max 累积；tick 末若 > -1 则发一个 ClientboundBlockChangedAckPacket
    // 并复位。此前本项目为"收包即立即发"，每包一个 ACK，流量 N 倍于原版且与原版批量语义不符。
    // hasConnection() 守卫确保无连接玩家（SimulatedPlayer）不发。
    if (m_ackBlockChangesUpTo > -1 && hasConnection()) {
        mc::network::ir::play::BlockChangedAck ack;
        ack.sequence = m_ackBlockChangesUpTo;
        static_cast<void>(_sendIrPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(ack)}}));
        m_ackBlockChangesUpTo = -1;
    }

    // 服务端物理驱动：无连接玩家（SimulatedPlayer）没有客户端发来 ServerboundMovePlayerPacket
    // 驱动物理，而 Player::aiStep（Player.cpp:2211-2218）被有意掏空（重力/移动移到 updatePhysics，
    // 真实在线玩家位置由客户端包权威写入 ServerPlayHandler.cpp:474-504，避免双重移动）。
    // 故 SimulatedPlayer 物理完全空转：velocity/fallDistance 恒 0、位置恒为 spawn 坐标不下落，
    // 致 canSmashAttack 恒 false（fallDistance 不累积）、moveToLocation 无法真正移动等系统性失效。
    // 此处对无连接玩家显式调用 updatePhysics（与客户端 ClientApplication.cpp:408 同入口）接通服务端
    // 物理驱动。hasConnection() 守卫确保真实玩家不受影响（继续走客户端包权威，无双重移动）。
    if (!hasConnection()) {
        updatePhysics();
    }
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

    if (!_sendIrPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}})) {
        spdlog::warn("ServerPlayer: SetCamera packet not sent (player={}, no connection)", username());
    }
}

} // namespace mc
