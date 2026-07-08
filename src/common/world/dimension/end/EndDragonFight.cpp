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

#include "EndDragonFight.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/boss/EnderDragonEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/EndGatewayEntity.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"

#include <algorithm>

#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// Data 序列化
// ============================================================================

EndDragonFight::Data EndDragonFight::Data::fromJson(const nlohmann::json& json)
{
    Data data;
    data.needsStateScanning = json.value("NeedsStateScanning", true);
    data.dragonKilled = json.value("DragonKilled", false);
    data.previouslyKilled = json.value("PreviouslyKilled", false);

    // 反序列化末影龙 UUID（MC Java 中键名为 "Dragon"）
    if (json.contains("Dragon") && json["Dragon"].is_string()) {
        data.dragonUUID = json["Dragon"].get<std::string>();
    }

    if (json.contains("Gateways") && json["Gateways"].is_array()) {
        std::vector<i32> gateways;
        gateways.reserve(json["Gateways"].size());
        for (const auto& val : json["Gateways"]) {
            gateways.push_back(val.get<i32>());
        }
        data.gateways = std::move(gateways);
    }
    // nullopt 表示首次初始化，需要从世界种子生成折跃门列表

    return data;
}

[[nodiscard]] nlohmann::json EndDragonFight::Data::toJson() const
{
    nlohmann::json json;
    json["NeedsStateScanning"] = needsStateScanning;
    json["DragonKilled"] = dragonKilled;
    json["PreviouslyKilled"] = previouslyKilled;

    // 序列化末影龙 UUID
    if (dragonUUID.has_value() && !dragonUUID->empty()) {
        json["Dragon"] = *dragonUUID;
    }

    if (gateways.has_value()) {
        json["Gateways"] = *gateways;
    }

    return json;
}

// ============================================================================
// 构造函数
// ============================================================================

EndDragonFight::EndDragonFight(u64 worldSeed, const std::optional<Data>& data)
    : m_worldSeed(worldSeed)
{
    if (data.has_value()) {
        _loadData(*data);
    } else {
        // 新世界：初始化折跃门列表
        m_gateways.reserve(GATEWAY_COUNT);
        for (i32 i = 0; i < GATEWAY_COUNT; ++i) {
            m_gateways.push_back(i);
        }

        // 使用世界种子创建随机数生成器并打乱
        math::Random rng(worldSeed);
        rng.shuffle(m_gateways);

        // 新世界首次创建，不需要扫描旧世界状态
        m_needsStateScanning = false;
        m_dragonKilled = false;
        m_previouslyKilled = false;
    }
}

// ============================================================================
// 核心逻辑
// ============================================================================

void EndDragonFight::tick(IWorld& world)
{
    // 1. 旧存档状态扫描：首次加载旧存档时检查出口传送门和末影龙存活状态
    if (m_needsStateScanning) {
        if (_isArenaLoaded(world)) {
            _scanState(world);
            m_needsStateScanning = false;
        }
    }

    // 2. Boss 栏可见性更新（每 tick）
    // 对应 MC Java: this.dragonEvent.setVisible(!this.dragonKilled);
    m_dragonBossBar->setVisible(!m_dragonKilled);

    // 3. 玩家扫描：每 TIME_BETWEEN_PLAYER_SCANS tick 更新 Boss 栏可见玩家列表
    // 对应 MC Java: if (++this.ticksSinceLastPlayerScan >= 20) { this.updatePlayers(); ... }
    if (++m_ticksSinceLastPlayerScan >= TIME_BETWEEN_PLAYER_SCANS) {
        _updatePlayers(world);
        m_ticksSinceLastPlayerScan = 0;
    }

    // 4. 仅有可见玩家时才执行重的战斗逻辑
    // 对应 MC Java: if (!this.dragonEvent.getPlayers().isEmpty()) { ... }
    if (!m_dragonBossBar->hasPlayers()) {
        return;
    }

    // 5. 龙失联检查：当龙未死亡但长时间未被 updateDragon() 调用时，重新查找或创建龙
    // 对应 MC Java: if (!this.dragonKilled) {
    //     if ((this.dragonUUID == null || ++this.ticksSinceDragonSeen >= 1200) && flag) {
    //         this.findOrCreateDragon();
    //         this.ticksSinceDragonSeen = 0;
    //     }
    // }
    if (!m_dragonKilled) {
        const bool arenaLoaded = _isArenaLoaded(world);
        const bool uuidEmpty = m_dragonUUID.empty();
        if ((uuidEmpty || ++m_ticksSinceDragonSeen >= MAX_TICKS_BEFORE_DRAGON_RESPAWN) && arenaLoaded) {
            _findOrCreateDragon(world);
            m_ticksSinceDragonSeen = 0;
        }
    }
}

void EndDragonFight::setDragonKilled(IWorld& world)
{
    // 1. Boss 栏：设置百分比为 0，隐藏
    // 对应 MC Java: this.dragonEvent.setProgress(0.0F); this.dragonEvent.setVisible(false);
    m_dragonBossBar->setPercent(0.0f);
    m_dragonBossBar->setVisible(false);

    // 2. 创建激活态出口传送门（讲台）
    EndTeleporter::createExitPortal(world, BlockPos(0, 0, 0), true);

    // 3. 生成一个末地折跃门（如果还有剩余）
    _spawnNewGateway(world);

    // 4. 首次击杀时在祭坛顶部放置龙蛋
    if (!m_previouslyKilled) {
        _placeDragonEgg(world);
    }

    // 5. 更新状态标志
    m_previouslyKilled = true;
    m_dragonKilled = true;

    // 6. 清空末影龙 UUID（龙已死亡，不再追踪）
    m_dragonUUID.clear();
}

void EndDragonFight::updateDragon(entity::EnderDragonEntity& dragon)
{
    // 对应 MC Java: EndDragonFight.updateDragon(EnderDragon)
    // if (p_64097_.getUUID().equals(this.dragonUUID)) {
    //     this.dragonEvent.setProgress(p_64097_.getHealth() / p_64097_.getMaxHealth());
    //     this.ticksSinceDragonSeen = 0;
    //     if (p_64097_.hasCustomName()) {
    //         this.dragonEvent.setName(p_64097_.getDisplayName());
    //     }
    // }

    if (dragon.uuid() != m_dragonUUID) {
        return;
    }

    // 同步血量百分比
    const f32 maxHp = dragon.maxHealth();
    const f32 hp = dragon.health();
    const f32 percent = (maxHp > 0.0f) ? (hp / maxHp) : 0.0f;
    m_dragonBossBar->setPercent(percent);

    // 重置失联计数器
    m_ticksSinceDragonSeen = 0;

    // 同步名称（仅当龙有自定义名称时）
    if (dragon.hasCustomName()) {
        m_dragonBossBar->setName(dragon.getDisplayName());
    }
}

void EndDragonFight::setDragonBossBar(std::unique_ptr<IDragonBossBar> bossBar)
{
    if (bossBar == nullptr) {
        m_dragonBossBar = std::make_unique<NullDragonBossBar>();
    } else {
        m_dragonBossBar = std::move(bossBar);
    }
}

// ============================================================================
// 数据保存
// ============================================================================

EndDragonFight::Data EndDragonFight::saveData() const
{
    Data data;
    data.needsStateScanning = m_needsStateScanning;
    data.dragonKilled = m_dragonKilled;
    data.previouslyKilled = m_previouslyKilled;

    // 序列化末影龙 UUID
    if (!m_dragonUUID.empty()) {
        data.dragonUUID = m_dragonUUID;
    }

    // gateways 始终保存（即使为空列表），区别于加载时的 nullopt
    data.gateways = m_gateways;
    return data;
}

// ============================================================================
// 私有方法
// ============================================================================

void EndDragonFight::_loadData(const Data& data)
{
    m_needsStateScanning = data.needsStateScanning;
    m_dragonKilled = data.dragonKilled;
    m_previouslyKilled = data.previouslyKilled;

    // 恢复末影龙 UUID
    if (data.dragonUUID.has_value()) {
        m_dragonUUID = *data.dragonUUID;
    }

    if (data.gateways.has_value()) {
        // 从存档恢复折跃门列表
        m_gateways = *data.gateways;
    } else {
        // 旧存档没有折跃门数据，从世界种子重新生成
        m_gateways.reserve(GATEWAY_COUNT);
        for (i32 i = 0; i < GATEWAY_COUNT; ++i) {
            m_gateways.push_back(i);
        }
        math::Random rng(m_worldSeed);
        rng.shuffle(m_gateways);
    }
}

void EndDragonFight::_findOrCreateDragon(IWorld& world)
{
    // 对应 MC Java: EndDragonFight.findOrCreateDragon()
    // 查找世界中已存在的末影龙实体，若存在则复用其 UUID，否则创建新龙。
    //
    // MC 原版：
    //   List<? extends EnderDragon> list = this.level.getDragons();
    //   if (list.isEmpty()) { this.createNewDragon(); }
    //   else { this.dragonUUID = list.get(0).getUUID(); }

    // 仅取存活的末影龙（MC 原版 getDragons() 内部过滤 isAlive）
    std::vector<Entity*> dragons = world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);

    // 过滤已死亡/已移除的实体（防御性，正常情况下 getEntitiesByType 不返回死亡实体）
    dragons.erase(
        std::remove_if(dragons.begin(), dragons.end(), [](Entity* e) { return e == nullptr || e->isRemoved(); }),
        dragons.end());

    if (!dragons.empty()) {
        // 复用已存在的龙：仅记录 UUID，不重复生成
        Entity* dragon = dragons[0];
        m_dragonUUID = dragon->uuid();
        m_dragonKilled = false;
        spdlog::info("EndDragonFight: Found existing dragon entity ({}) - reusing UUID.", m_dragonUUID);
        return;
    }

    // 世界中无末影龙：创建新龙
    spdlog::info("EndDragonFight: No dragon entity found, respawning it.");
    _createNewDragon(world);
}

bool EndDragonFight::_createNewDragon(IWorld& world)
{
    // 对应 MC Java: EndDragonFight.createNewDragon()
    // 通过 EntityType 工厂创建末影龙，设置位置/朝向/初始阶段，加入世界。

    // 1. 从实体注册表获取末影龙 EntityType
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* dragonType = registry.getType(entity::EntityTypes::ENDER_DRAGON);
    if (dragonType == nullptr) {
        spdlog::warn("EndDragonFight: Ender dragon entity type not registered, cannot create new dragon.");
        return false;
    }

    // 2. 工厂方法创建实例（Entity 构造时自动生成随机 UUID）
    std::unique_ptr<Entity> dragonEntity = dragonType->create(&world);
    if (dragonEntity == nullptr) {
        spdlog::warn("EndDragonFight: Ender dragon factory returned nullptr.");
        return false;
    }

    // 3. 设置生成位置 (0, DRAGON_SPAWN_Y, 0) 和随机朝向
    // MC 原版：snapTo(origin.x, 128+origin.y, origin.z, random.nextFloat()*360, 0)
    // origin 默认 (0,0,0)，故生成位置为 (0, 128, 0)
    math::Random& rng = world.getRandom();
    const f32 yaw = rng.nextFloat() * 360.0f;
    dragonEntity->setPosition(Vector3(0.0f, static_cast<f32>(DRAGON_SPAWN_Y), 0.0f));
    dragonEntity->setRotation(yaw, 0.0f);

    // 4. 设置初始阶段为 HoldingPattern
    // MC 原版：enderdragon.getPhaseManager().setPhase(EnderDragonPhase.HOLDING_PATTERN)
    // Cubium 的 EnderDragonEntity 默认阶段已是 HoldingPattern（构造函数初始化），
    // 但显式调用 setPhase 以对齐 MC 语义，并为未来阶段系统扩展预留接入点。
    auto* dragon = dynamic_cast<entity::EnderDragonEntity*>(dragonEntity.get());
    if (dragon != nullptr) {
        dragon->setPhase(entity::EnderDragonEntity::Phase::HoldingPattern);
    }

    // 5. 在 spawnEntity 之前记录 UUID，因为 spawnEntity 会 transfer ownership
    const std::string newDragonUUID = dragonEntity->uuid();

    // 6. 加入世界（ServerWorld::spawnEntity 内部完成 ID 分配、EntityTracker 注册、区块跟踪）
    const EntityId spawnedId = world.spawnEntity(std::move(dragonEntity));
    if (spawnedId == 0) {
        spdlog::warn("EndDragonFight: Failed to spawn new dragon (spawnEntity returned 0).");
        return false;
    }

    // 7. 记录新龙的 UUID，重置状态
    m_dragonUUID = newDragonUUID;
    m_dragonKilled = false;
    m_ticksSinceDragonSeen = 0;

    spdlog::info("EndDragonFight: Spawned new dragon entity (id={}, uuid={}).", spawnedId, m_dragonUUID);
    return true;
}

void EndDragonFight::_scanState(IWorld& world)
{
    spdlog::info("EndDragonFight: Scanning for legacy world dragon fight state...");

    const bool hasActivePortal = _hasActiveExitPortal(world);

    if (hasActivePortal) {
        spdlog::info("EndDragonFight: Found active exit portal - dragon has been killed before.");
        m_previouslyKilled = true;
    } else {
        spdlog::info("EndDragonFight: No active exit portal found - dragon has not been killed yet.");
        m_previouslyKilled = false;

        // 检查是否存在讲台结构，如果不存在则创建非激活讲台
        // 讲台位于原点 (0, 0, 0)，通过 getHeight 获取表面高度后检查基岩
        const i32 surfaceY = world.getHeight(0, 0);
        const BlockState* surfaceBlock = world.getBlockState(0, surfaceY - 1, 0);
        bool hasPodium = (surfaceBlock != nullptr && surfaceBlock->is(VanillaBlocks::BEDROCK));

        if (!hasPodium) {
            // 未找到讲台结构，创建非激活讲台（不含传送门方块）
            spdlog::info("EndDragonFight: No exit portal structure found, creating inactive portal.");
            EndTeleporter::createExitPortal(world, BlockPos(0, 0, 0), false);
        }
    }

    // 检查世界中是否存在末影龙实体
    // 对应 MC Java: EndDragonFight.scanState() 中 this.level.getDragons()
    auto dragons = world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);

    if (dragons.empty()) {
        // 没有末影龙实体 -> 龙已死亡
        m_dragonKilled = true;
        m_dragonUUID.clear();
        spdlog::info("EndDragonFight: No dragon entity found - dragon is killed.");
    } else {
        // 找到末影龙实体 -> 记录 UUID，龙仍存活
        Entity* dragon = dragons[0];
        m_dragonUUID = dragon->uuid();
        m_dragonKilled = false;
        spdlog::info("EndDragonFight: Found dragon entity ({}) - dragon is alive.", m_dragonUUID);

        // 如果同时无活跃传送门，则丢弃该龙（discard），因为无传送门的龙是无效状态
        // 对应 MC Java: enderdragon.discard()
        if (!hasActivePortal) {
            spdlog::info("EndDragonFight: Dragon exists but no active portal - discarding dragon.");
            dragon->discard();
            m_dragonUUID.clear();
        }
    }

    // 最终安全检查：如果从未杀过龙但龙被标记为已死，则修正为未死
    // 这确保初始世界中 dragonKilled 不会被错误地设为 true
    // 对应 MC Java: if (!this.previouslyKilled && this.dragonKilled) { this.dragonKilled = false; }
    if (!m_previouslyKilled && m_dragonKilled) {
        m_dragonKilled = false;
    }
}

bool EndDragonFight::_hasActiveExitPortal(IWorld& world)
{
    // 扫描原点周围区块，查找 END_PORTAL 方块
    // 活跃出口传送门包含 END_PORTAL 方块，只有龙被击杀后才会存在
    const BlockState* endPortalState = VanillaBlocks::getState(VanillaBlocks::END_PORTAL);
    if (endPortalState == nullptr) {
        return false;
    }

    for (ChunkCoord cx = -ARENA_CHUNK_RADIUS; cx <= ARENA_CHUNK_RADIUS; ++cx) {
        for (ChunkCoord cz = -ARENA_CHUNK_RADIUS; cz <= ARENA_CHUNK_RADIUS; ++cz) {
            const ChunkData* chunk = world.getChunk(cx, cz);
            if (chunk == nullptr) {
                continue;
            }

            // 出口传送门位于原点附近，Y 坐标通常在 0~75 之间
            // 仅扫描可能存在传送门的高度范围以提升性能
            for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
                for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                        const BlockState* state = chunk->getBlockState(x, y, z);
                        if (state == endPortalState) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool EndDragonFight::_isArenaLoaded(IWorld& world)
{
    // 检查原点周围的区块是否已加载
    for (ChunkCoord cx = -ARENA_CHUNK_RADIUS; cx <= ARENA_CHUNK_RADIUS; ++cx) {
        for (ChunkCoord cz = -ARENA_CHUNK_RADIUS; cz <= ARENA_CHUNK_RADIUS; ++cz) {
            if (!world.hasChunk(cx, cz)) {
                return false;
            }
        }
    }
    return true;
}

void EndDragonFight::_spawnNewGateway(IWorld& world)
{
    if (m_gateways.empty()) {
        return;
    }

    // 从列表尾部取出索引
    const i32 gatewayIndex = m_gateways.back();
    m_gateways.pop_back();

    // 计算折跃门位置
    const f64 angle =
        2.0 * (-math::PI_DOUBLE + (math::PI_DOUBLE / static_cast<f64>(GATEWAY_COUNT)) * static_cast<f64>(gatewayIndex));
    const i32 x = static_cast<i32>(std::floor(static_cast<f64>(GATEWAY_DISTANCE) * std::cos(angle)));
    const i32 z = static_cast<i32>(std::floor(static_cast<f64>(GATEWAY_DISTANCE) * std::sin(angle)));

    const BlockPos gatewayPos(x, GATEWAY_Y, z);

    _spawnNewGatewayAt(world, gatewayPos);
}

void EndDragonFight::_spawnNewGatewayAt(IWorld& world, const BlockPos& pos)
{
    // 播放折跃门生成效果
    world.playEvent(world::WorldEvents::GATEWAY_SPAWN_EFFECTS, pos, 0);

    // 直接放置折跃门结构（3x5x3 基岩十字框架 + END_GATEWAY 方块）
    blockentity::EndGatewayEntity::createGatewayStructure(world, pos);
}

void EndDragonFight::_placeDragonEgg(IWorld& world)
{
    // 获取 X=0, Z=0 处的高度（MOTION_BLOCKING 语义）
    const i32 topY = world.getHeight(0, 0);

    // 龙蛋放置在最高阻挡运动方块的上方
    // world.getHeight() 返回最高方块Y+1，所以 topY 就是龙蛋应放置的Y坐标
    const BlockState* dragonEggState = VanillaBlocks::getState(VanillaBlocks::DRAGON_EGG);
    if (dragonEggState != nullptr) {
        world.setBlockState(BlockPos(0, topY, 0), dragonEggState);
    }
}

void EndDragonFight::_updatePlayers(IWorld& world)
{
    // 对应 MC Java: EndDragonFight.updatePlayers()
    // 扫描 PLAYER_TRACKING_RADIUS 半径内的存活玩家，一次性替换 Boss 栏可见玩家列表。
    // replacePlayers 内部计算差集，仅对新增/移除的玩家发送 Add/Remove 包，
    // 已追踪且仍在范围内的玩家不受影响（无闪烁）。

    // validPlayer 谓词：ENTITY_STILL_ALIVE 且距离 (0, 128, 0) 在 PLAYER_TRACKING_RADIUS 内
    // MC Java: withinDistance(origin.x, 128 + origin.y, origin.z, 192.0)
    // Cubium 的末地原点为 (0, 0, 0)，故追踪中心为 (0, 128, 0)

    const Vector3 trackCenter(0.0f, 128.0f, 0.0f);
    const f32 trackRadiusSq = PLAYER_TRACKING_RADIUS * PLAYER_TRACKING_RADIUS;

    std::set<PlayerId> inRange;

    std::vector<Entity*> players = world.getPlayers();
    for (Entity* entity : players) {
        if (entity == nullptr || !entity->isAlive()) {
            continue;
        }

        Player* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 3D 球形距离检测（MC Java 的 withinDistance 是 3D 检测）
        const Vector3 delta(entity->x() - trackCenter.x, entity->y() - trackCenter.y, entity->z() - trackCenter.z);
        const f32 distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (distSq <= trackRadiusSq) {
            inRange.insert(player->playerId());
        }
    }

    m_dragonBossBar->replacePlayers(inRange);
}

std::unique_ptr<text::ITextComponent> EndDragonFight::createDefaultBossName()
{
    // 对应 MC Java: Component.translatable("entity.minecraft.ender_dragon")
    return std::make_unique<text::TranslationTextComponent>("entity.minecraft.ender_dragon");
}

} // namespace mc
