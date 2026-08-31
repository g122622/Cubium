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

#pragma once

#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace mc {
namespace test {

/**
 * @brief 测试共享 ECS 实体注册表
 *
 * ECS 重构后所有 Entity 派生类构造函数末尾新增 ecs::EntityRegistry& 参数（构造时
 * 在此 registry 内 create 出 entt 实体并 attach 基础组件）。tests/ 下大量测试需
 * 构造实体而无真实 ServerWorld，统一经此共享 registry 获取引用。
 *
 * 与生产代码 IntegratedServer::_getMenuPlayer / ContainerManager /
 * BlockInteractionManager 的"静态局部 registry"先例一致。
 *
 * 注意：Entity 析构为 = default，不回 registry.destroy()，故此 registry 会随测试
 * 进程累积 entt 实体；但测试为 per-case 进程隔离（见 crosstest-isolation-model），
 * 单进程内累积量受控，且测试仅消费 OOP 层方法不依赖 ECS 查询语义，可接受。
 *
 * 需要与某个 IWorld 的 registry 保持一致的场景（实体经 addEntity 进入真实世界），
 * 应改用 m_world.entityRegistry() 取该世界自身的 registry，而非本函数。
 */
inline ecs::EntityRegistry& testEcsRegistry()
{
    static ecs::EntityRegistry s_testRegistry{"test"};
    return s_testRegistry;
}

/**
 * @brief 测试用的空 TickManager 实现
 *
 * 用于测试中需要 IWorld::tickManager() 但不需要实际功能的场景。
 * 注意：由于 TickManager 的方法不是 virtual，这里使用组合模式，
 * 但需要特殊处理来满足接口要求。
 */
class DummyTickManager : public world::tick::TickManager {
public:
    DummyTickManager();

private:
    static IWorld& dummyWorld();
};

/**
 * @brief 测试用基础世界桩
 *
 * 提供 tests 中最常见的 IWorld 默认实现，避免每个测试重复样板代码。
 * 需要特殊行为的测试只覆写自己关心的方法即可。
 */
class BaseTestWorld : public IBlockReader {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        // 走 fluidId 路径取 EMPTY 流体默认状态（Fluids::EMPTY()->defaultState()）。
        return fluid::Fluids::EMPTY() != nullptr ? &fluid::Fluids::EMPTY()->defaultState() : nullptr;
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    // 返回世界最大建造高度，原硬编码 64 会导致 PistonBlock::canPush 等检查
    // pos.y >= getHeight() 时误判 y=64 方块超出高度（64>=64）。MC 主世界高度上限为 320。
    [[nodiscard]] i32 getHeight(i32, i32) const override { return world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BaseTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BaseTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    // id 反查：经 registerEntityForLookup 登记的栈/堆实体可被 getEntity(id) 查到。
    // Brain 实体类记忆（ATTACK_TARGET 等）已改存 EntityInstanceId，消费点经
    // world->getEntity(id) 反查，测试世界需提供此最小实现。
    void registerEntityForLookup(Entity* entity)
    {
        if (entity != nullptr) {
            m_lookupEntities[entity->id()] = entity;
        }
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_lookupEntities.find(id);
        return it != m_lookupEntities.end() ? it->second : nullptr;
    }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_lookupEntities.find(id);
        return it != m_lookupEntities.end() ? it->second : nullptr;
    }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    // 提供 ECS 实体注册表，供测试构造 Entity 派生类（构造函数透传 registry）。
    // 统一返回 testEcsRegistry() 全局静态实例，保证测试内所有实体构造在同一 registry，
    // 避免出现"m_world 的 registry 与 fixture 的 registry 不一致"的歧义。
    [[nodiscard]] ecs::EntityRegistry* entityRegistry() override { return &testEcsRegistry(); }
    [[nodiscard]] const ecs::EntityRegistry* entityRegistry() const override { return &testEcsRegistry(); }

protected:
    BaseTestWorld() = default;

    world::border::WorldBorder m_worldBorder;
    world::gamerule::GameRules m_gameRules;
    mutable math::Random m_random{12345};
    std::unordered_map<EntityInstanceId, Entity*> m_lookupEntities;
};

/**
 * @brief 带区块存储的测试世界基类
 *
 * 适用于需要少量 ChunkData/高度/生物群系支撑的测试。
 */
class BaseChunkBackedTestWorld : public BaseTestWorld {
public:
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override
    {
        const auto it = m_chunks.find(ChunkPos(x, z));
        return it != m_chunks.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override
    {
        return m_chunks.find(ChunkPos(x, z)) != m_chunks.end();
    }

    ChunkData& ensureChunk(ChunkCoord x, ChunkCoord z)
    {
        const ChunkPos chunkPos(x, z);
        auto it = m_chunks.find(chunkPos);
        if (it == m_chunks.end()) {
            it = m_chunks.emplace(chunkPos, std::make_unique<ChunkData>(x, z)).first;
        }
        return *it->second;
    }

protected:
    std::unordered_map<ChunkPos, std::unique_ptr<ChunkData>> m_chunks;
};

} // namespace test
} // namespace mc
