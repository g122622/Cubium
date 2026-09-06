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

#include "EntityManager.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/ecs/systems/BrainTickSystem.hpp"
#include "common/entity/ecs/systems/EntityLegacyTickSystem.hpp"
#include "common/entity/ecs/systems/FireTickSystem.hpp"
#include "common/entity/ecs/systems/PortalTickSystem.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/mod/bedrock/addon/binding/ScriptHandleRegistry.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {

EntityManager::EntityManager(ecs::EntityRegistry& registry)
    : m_registry(registry)
{
    // 注册 EntityTick 阶段的 OOP tick 桥接 System。
    // 回调委托 _tickEntities()（含模拟距离门控/ServerPlayer 永远 tick 等成熟逻辑），
    // 避免 EntityManager ↔ Scheduler 循环依赖，且保持原三步 tick 编排语义不变。
    m_scheduler.registerSystem(ecs::SystemPhase::EntityTick,
        std::make_shared<ecs::EntityLegacyTickSystem>([this](ecs::EntityRegistry&) { this->_tickEntities(); }));

    // PostEntityTick 阶段：状态递减/环境交互类真实 System（第二批新增）。
    // PortalTickSystem：传送冷却递减 + tickPortal 逻辑（从 baseTick/tick 抽出）。
    // FireTickSystem：fire 链递减/伤害/水中熄灭/雨中扑灭（从 baseTick 抽出）。
    // 两者在 EntityTick 之后执行，可读到本帧 updateEnvironmentState 产出的环境状态。
    m_scheduler.registerSystem(ecs::SystemPhase::PostEntityTick, std::make_shared<ecs::PortalTickSystem>());
    m_scheduler.registerSystem(ecs::SystemPhase::PostEntityTick, std::make_shared<ecs::FireTickSystem>());

    // PostEntityTick 阶段：Brain tick 系统（批次6子目标3）。
    // 注册在 PortalTickSystem/FireTickSystem 之后，使 Brain tick 在所有实体 OOP tick
    // + portal/fire 递减之后执行。回调委托 _tickBrains()（复用 _tickEntities 门控框架）。
    // Brain 仍是 OOP 成员，本 System 只搬 tick 调度决策，不 ECS 化 Brain 数据。
    m_scheduler.registerSystem(ecs::SystemPhase::PostEntityTick,
        std::make_shared<ecs::BrainTickSystem>([this](ecs::EntityRegistry&) { this->_tickBrains(); }));
}

EntityInstanceId EntityManager::addEntity(std::unique_ptr<Entity> entity)
{
    if (!entity) {
        return 0;
    }

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    EntityInstanceId id = entity->id();

    // 如果ID为0或已存在，分配新ID
    if (id == 0 || m_entities.find(id) != m_entities.end()) {
        id = allocateId();
        // 设置实体的ID
        entity->setId(id);
    }

    // 维护 UUID 索引
    const std::string& uuid = entity->uuid();
    if (!uuid.empty()) {
        if (m_uuidToEntity.find(uuid) != m_uuidToEntity.end()) {
            // UUID 冲突：输出警告但不阻止添加
            spdlog::warn("Duplicate entity UUID {}: entity {} will override existing mapping", uuid, id);
        }
        m_uuidToEntity[uuid] = entity.get();
    }

    // 绑定反向指针并登记到空间索引（按当前位置一次性注册）。须在 move 进 m_entities
    // 前完成——move 后 entity 为 nullptr。setEntityManager 后若实体再 setPosition 会经
    // reapplyPosition 通知索引迁移，正确。
    entity->setEntityManager(this);
    m_spatialIndex.addEntity(*entity);

    m_entities[id] = std::move(entity);
    return id;
}

std::unique_ptr<Entity> EntityManager::removeEntity(EntityInstanceId id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return nullptr;
    }

    auto entity = std::move(it->second);

    // 维护 UUID 索引
    const std::string& uuid = entity->uuid();
    if (!uuid.empty()) {
        auto uuidIt = m_uuidToEntity.find(uuid);
        // 仅当映射指向当前实体时才移除（防止移除后添加的同 UUID 实体被误删）
        if (uuidIt != m_uuidToEntity.end() && uuidIt->second == entity.get()) {
            m_uuidToEntity.erase(uuidIt);
        }
    }

    // 从空间索引移除并解绑反向指针。须在 erase 前——此时 entity 仍持有对象所有权有效。
    // 返回的 unique_ptr 随后析构实体，索引侧须先清理避免悬挂。
    m_spatialIndex.removeEntity(*entity);
    entity->setEntityManager(nullptr);

    m_entities.erase(it);

    // 路径B：实体从 EntityManager 移除（如区块卸载 ServerWorld.cpp:2276 丢弃 unique_ptr 立即 free）。
    // 此时 JS 侧 owned=false 的 Entity/组件句柄仍持裸 Entity*，必须立即 invalidate（置 ptr=nullptr），
    // 否则后续 getComponent("minecraft:onfire").isOnFire() 等回调解引用悬垂指针 UAF 段错误。
    // 路径A（remove()/discard → graveyard 延迟析构）由 ~Entity 兜底 invalidate，但路径B可能绕过
    // graveyard（unique_ptr 直接丢弃），故此处为路径B的唯一兜底入口。
    mc::mod::bedrock::addon::ScriptHandleRegistry::instance().invalidateAll(id);

    return entity;
}

void EntityManager::requestDimensionTransfer(std::function<void()> transferAction)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_pendingDimensionTransfers.push_back(std::move(transferAction));
}

bool EntityManager::hasEntity(EntityInstanceId id) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_entities.find(id) != m_entities.end();
}

size_t EntityManager::entityCount() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_entities.size();
}

Entity* EntityManager::getEntity(EntityInstanceId id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

const Entity* EntityManager::getEntity(EntityInstanceId id) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

Entity* EntityManager::getEntityByUuid(const std::string& uuid)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_uuidToEntity.find(uuid);
    return it != m_uuidToEntity.end() ? it->second : nullptr;
}

const Entity* EntityManager::getEntityByUuid(const std::string& uuid) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_uuidToEntity.find(uuid);
    return it != m_uuidToEntity.end() ? it->second : nullptr;
}

bool EntityManager::hasEntityWithUuid(const std::string& uuid) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_uuidToEntity.find(uuid) != m_uuidToEntity.end();
}

std::vector<Entity*> EntityManager::getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    // 走 3D section 空间索引：枚举 box 覆盖的 section，逐实体 AABB 精筛。
    // isRemoved 双保险：entity->remove() 仅标记，下次 tick 的 _removeDeadEntitiesInternal
    // 才从索引移除，故标记到清理之间仍可能命中，须过滤。
    m_spatialIndex.collectEntitiesInAABB(box, except, [&](Entity& entity) -> bool {
        if (!entity.isRemoved()) {
            result.push_back(&entity);
        }
        return true; // 继续遍历
    });

    return result;
}

std::vector<Entity*> EntityManager::getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    // 走 3D section 空间索引：球转外接盒枚举 + position 距离平方精筛（见 collectEntitiesInRange）。
    // isRemoved 双保险：见 getEntitiesInAABB 注释。
    m_spatialIndex.collectEntitiesInRange(pos, range, except, [&](Entity& entity) -> bool {
        if (!entity.isRemoved()) {
            result.push_back(&entity);
        }
        return true; // 继续遍历
    });

    return result;
}

std::vector<Entity*> EntityManager::getEntitiesByType(const std::string& typeId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    // 字符串 typeId → const EntityType* 指针（同一注册表对象，与 entity->entityType() 指针比较等价）。
    // 未注册类型返回 nullptr，collectEntitiesByType 直接返回空。
    const entity::EntityType* type = entity::EntityRegistry::instance().getType(typeId);
    if (type == nullptr) {
        return result;
    }

    // 走 3D section 空间索引：遍历各 section 的该类型子列表（懒加载）。
    // isRemoved 双保险：见 getEntitiesInAABB 注释。
    m_spatialIndex.collectEntitiesByType(type, [&](Entity& entity) -> bool {
        if (!entity.isRemoved()) {
            result.push_back(&entity);
        }
        return true; // 继续遍历
    });

    return result;
}

void EntityManager::forEachEntity(const std::function<bool(Entity*)>& callback)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto& [id, entity] : m_entities) {
        if (!callback(entity.get())) {
            break;
        }
    }
}

void EntityManager::forEachEntity(const std::function<bool(const Entity*)>& callback) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (const auto& [id, entity] : m_entities) {
        if (!callback(entity.get())) {
            break;
        }
    }
}

void EntityManager::tick()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "EntityManager::tick");

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 1. 委托调度器执行 EntityTick（逐实体 OOP tick）+ PostEntityTick（fire/portal 等 System）。
    //    EntityLegacyTickSystem 经回调调 _tickEntities()，内含模拟距离门控与 ServerPlayer 短路。
    m_scheduler.tick(m_registry);

    // 1.5 处理本 tick 期间入队的延迟跨维度迁移请求。changeDimension 可能在 entity->tick()
    //     调用栈内触发（doBlockCollisions→onEntityCollision→changeDimension），此时 _tickEntities
    //     正遍历 m_entities，同步 removeEntity 会 erase 当前节点致 ++it 失效→SIGSEGV。故迁移
    //     延迟到遍历结束（此时 m_scheduler.tick 已返回）后执行，erase 安全。
    _processPendingDimensionTransfers();

    // 2. 释放上一 tick 入 graveyard 的实体。此时引用者已在步骤 1 通过 isAlive()==false 放手，可安全析构。
    //    必须在 entity tick 之后：若放开头，graveyard 实体在 goal 跑 shouldContinueExecuting 前就析构了。
    m_graveyard.clear();

    // 3. 移除本帧死亡实体（入 graveyard，延迟到下一 tick 末尾析构）。
    _removeDeadEntitiesInternal();

    // 4. DEBUG 一致性断言：校验空间索引与主存储一致（每实体在正确 section、玩家专表一致）。
    //    仅调试构建生效（_assertConsistent 内 #ifndef NDEBUG 守卫），及早暴露索引漂移。
#ifndef NDEBUG
    m_spatialIndex._assertConsistent(m_entities);
#endif
}

void EntityManager::_tickEntities()
{
    // 0. 循环外快照玩家区块坐标。getPlayers() 持递归锁安全，但每实体调用代价高，
    //    故一次取玩家列表并预算其区块坐标，供冻结判定复用。
    //    对齐原版 ServerLevel.tick：ServerPlayer 永远 tick；其余实体仅当其所在区块
    //    相对任一玩家切比雪夫距离 <= simulationDistance 才 tick，否则冻结（连 tick()
    //    都不调用，等价于不进 tickNonPassenger）。死亡消散中的实体同样受此门控——
    //    原版服务端 shouldTickDeath 恒真但 tickDeath 受 tick() 调用制约，故超出模拟
    //    距离的死亡实体其 deathTime 停滞、尸体滞留，此为本轮严格对齐的行为。
    const bool freezeEnabled = m_simulationDistance < 32;
    std::vector<world::chunk::ChunkPos> playerChunks;
    if (freezeEnabled) {
        const auto players = getPlayers();
        playerChunks.reserve(players.size());
        for (const auto* player : players) {
            playerChunks.emplace_back(player->position());
        }
    }

    // 1. 更新所有实体。期间 goal 通过 isAlive() 检测上一 tick 入 graveyard 的目标
    //    （对象仍存活、m_removed=true）并 resetTask 清空裸指针，避免悬垂解引用。
    for (auto& [id, entity] : m_entities) {
        // spdlog::info("Ticking entity: id={}, detail={}", id, entity->toString());
        if (entity->isRemoved()) {
            continue;
        }
        // ServerPlayer 永远 tick（对齐原版 instanceof ServerPlayer 短路）。
        if (entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Server.Tick, "EntityManager::tick.perEntity", "entityId", id, "name", entity->getTypeId());
            entity->tick();
            continue;
        }
        // 非玩家实体：模拟距离门控。>=32 时 freezeEnabled=false 跳过判定等价全量 tick。
        if (freezeEnabled && !_isEntityInSimulationRange(*entity, playerChunks)) {
            continue; // 冻结：不调 tick()
        }
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Tick, "EntityManager::tick.perEntity", "entityId", id, "name", entity->getTypeId());
        entity->tick();
    }
}

void EntityManager::_tickBrains()
{
    // 复用 _tickEntities 的遍历+门控框架。playerChunks 独立快照（ServerPlayer 数量少，
    // 重复快照成本可忽略；若共用须把快照提到 tick() 顶层改变三步编排，不值得）。
    const bool freezeEnabled = m_simulationDistance < 32;
    std::vector<world::chunk::ChunkPos> playerChunks;
    if (freezeEnabled) {
        const auto players = getPlayers();
        playerChunks.reserve(players.size());
        for (const auto* player : players) {
            playerChunks.emplace_back(player->position());
        }
    }

    for (auto& [id, entity] : m_entities) {
        if (entity->isRemoved()) {
            continue;
        }
        // 非玩家实体模拟距离门控（Player 无 Brain，门控对其无意义但保持框架一致）。
        if (freezeEnabled && entity->entityType() != entity::VanillaEntityTypeKeys::PLAYER &&
            !_isEntityInSimulationRange(*entity, playerChunks)) {
            continue; // 冻结：不调 brain().tick()
        }

        // 类型识别：当前仅 VillagerEntity 持 Brain。dynamic_cast 失败早退。
        auto* villager = dynamic_cast<entity::VillagerEntity*>(entity.get());
        if (villager == nullptr) {
            continue;
        }

        // 判空 + 参数获取（逐字搬迁 VillagerEntity::tick 原 line 137-144）。
        // brain() 返回 *m_brain 引用（构造恒 make_unique<VillagerBrain> 非空），故只判 world。
        IWorld* world = villager->world();
        if (world == nullptr) {
            continue;
        }
        const i64 gameTime = static_cast<i64>(world->currentTick());
        const i32 dayTime = static_cast<i32>(world->dayTimeOfDay());
        villager->brain().tick(world, villager, gameTime, dayTime, villager->getRandom());
    }
}

void EntityManager::removeDeadEntities()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // 与 tick 时序对齐：先释放上一批 graveyard，再收集本批死亡实体入 graveyard。
    m_graveyard.clear();
    _removeDeadEntitiesInternal();
}

void EntityManager::_removeDeadEntitiesInternal()
{
    // 内部方法，假设已持有锁
    for (auto it = m_entities.begin(); it != m_entities.end();) {
        if (it->second->isRemoved()) {
            // 维护 UUID 索引（erase 时同步清，不等 graveyard 析构时才清）
            const std::string& uuid = it->second->uuid();
            if (!uuid.empty()) {
                auto uuidIt = m_uuidToEntity.find(uuid);
                if (uuidIt != m_uuidToEntity.end() && uuidIt->second == it->second.get()) {
                    m_uuidToEntity.erase(uuidIt);
                }
            }

            // 从空间索引移除并解绑反向指针（须在 move 入 graveyard 前，此时对象仍有效）。
            // 顺手修复现存隐患：原 _removeDeadEntitiesInternal 不通知任何 tracker，死亡实体
            // 残留索引导致后续查询命中已死实体（靠 isRemoved 双保险兜底，但索引应保持干净）。
            m_spatialIndex.removeEntity(*it->second);
            it->second->setEntityManager(nullptr);

            // 延迟析构：实体对象入 graveyard，下一 tick 末尾才真正析构。
            // 避免持裸指针的 goal 在本 tick 末尾或下 tick 开头解引用悬垂内存。
            m_graveyard.push_back(std::move(it->second));
            it = m_entities.erase(it); // it->second 已被 move 成 nullptr，erase 仅移除 map 条目
        } else {
            ++it;
        }
    }
}

void EntityManager::_onEntityPositionChanged(Entity& entity)
{
    // 假设已持有 m_mutex（Entity::reapplyPosition 在持锁上下文执行）。
    // 转调空间索引：实体跨 section 移动时迁移，使索引实时准确。
    m_spatialIndex.onEntityPositionChanged(entity);
}

void EntityManager::_processPendingDimensionTransfers()
{
    // _tickEntities 遍历已完成（m_scheduler.tick 已返回），此时 erase 当前节点安全。
    // swap 出队列后逐个执行迁移回调（removeEntity + spawnEntity），避免遍历期间 erase
    // 致 for 循环 ++it 解引用失效迭代器→SIGSEGV。
    if (m_pendingDimensionTransfers.empty()) {
        return;
    }

    std::vector<std::function<void()>> pending;
    pending.swap(m_pendingDimensionTransfers);

    for (auto& transferAction : pending) {
        transferAction();
    }
}

bool EntityManager::_isEntityInSimulationRange(
    const Entity& entity, const std::vector<world::chunk::ChunkPos>& playerChunks) const
{
    // 内部方法，假设已持有锁。调用方已保证 m_simulationDistance < 32（>=32 时短路全量）。
    if (playerChunks.empty()) {
        return false; // 无在线玩家：非玩家实体一律冻结（对齐原版无票据则无 EntityTickingRange）
    }

    const world::chunk::ChunkPos entityChunk(entity.position());
    const i32 maxDistance = m_simulationDistance;
    for (const auto& playerChunk : playerChunks) {
        const i32 dx = std::abs(entityChunk.x - playerChunk.x);
        const i32 dz = std::abs(entityChunk.z - playerChunk.z);
        if (dx <= maxDistance && dz <= maxDistance) {
            return true; // 切比雪夫距离 <= simulationDistance（任一玩家满足即 tick，多玩家取并集）
        }
    }
    return false;
}

EntityInstanceId EntityManager::allocateId()
{
    // 内部方法，假设已持有锁
    // ID 单调递增、永不复用（见头文件说明）。
    return m_nextId++;
}

std::unordered_map<entity::EntityClassification, i32> EntityManager::countEntitiesByClassification() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::unordered_map<entity::EntityClassification, i32> counts;

    // 初始化所有分类为 0
    counts[entity::EntityClassification::Monster] = 0;
    counts[entity::EntityClassification::Creature] = 0;
    counts[entity::EntityClassification::Ambient] = 0;
    counts[entity::EntityClassification::Axolotls] = 0;
    counts[entity::EntityClassification::UndergroundWaterCreature] = 0;
    counts[entity::EntityClassification::WaterCreature] = 0;
    counts[entity::EntityClassification::WaterAmbient] = 0;
    counts[entity::EntityClassification::Misc] = 0;

    // 遍历所有实体，统计各分类数量
    auto& registry = entity::EntityRegistry::instance();
    for (const auto& [id, entity] : m_entities) {
        if (entity && !entity->isRemoved()) {
            // 通过实体类型ID获取分类
            const std::string& typeId = entity->getTypeId();
            const entity::EntityType* type = registry.getType(typeId);
            if (type) {
                entity::EntityClassification classification = type->classification();

                // 跳过持久化的 Mob（命名/桶装等）：createState 不把持久化实体计入
                // 容量计数，否则会挤占刷新名额。
                if (classification != entity::EntityClassification::Misc) {
                    const auto* mob = dynamic_cast<const MobEntity*>(entity.get());
                    if (mob != nullptr && (mob->isNoDespawnRequired() || mob->preventDespawn())) {
                        continue;
                    }
                }

                counts[classification]++;
            }
        }
    }

    return counts;
}

i32 EntityManager::getCountByClassification(entity::EntityClassification classification) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    i32 count = 0;

    auto& registry = entity::EntityRegistry::instance();
    for (const auto& [id, entity] : m_entities) {
        if (entity && !entity->isRemoved()) {
            const std::string& typeId = entity->getTypeId();
            const entity::EntityType* type = registry.getType(typeId);
            if (type && type->classification() == classification) {
                count++;
            }
        }
    }

    return count;
}

std::vector<Entity*> EntityManager::getPlayers() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    // 走玩家专表 O(1)：addEntity/removeEntity 时按 PLAYER 身份同步维护。
    // isRemoved 双保险：见 getEntitiesInAABB 注释。
    for (Entity* player : m_spatialIndex.players()) {
        if (player != nullptr && !player->isRemoved()) {
            result.push_back(player);
        }
    }

    return result;
}

} // namespace mc
