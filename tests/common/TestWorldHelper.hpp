#pragma once

#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace mc {
namespace test {

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
class BaseTestWorld : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return fluid::Fluid::getFluidState(0);
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
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
    [[nodiscard]] bool isClientSide() override { return false; }

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

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

protected:
    BaseTestWorld() = default;

    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
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
