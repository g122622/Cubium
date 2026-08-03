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

#include "client/application/ClientApplication.hpp"

#include "client/world/ClientWorld.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/ray/Ray.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <vector>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {

namespace {

/**
 * @brief ClientWorld 到 IBlockReader 的轻量适配器
 *
 * 射线检测接口当前要求 IBlockReader，
 * 而 ClientWorld 实现的是 ICollisionWorld（方法签名兼容）。
 */
class ClientWorldBlockReader final : public mc::IBlockReader {
public:
    explicit ClientWorldBlockReader(const ClientWorld& world)
        : m_world(world)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        return m_world.getBlockState(x, y, z);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override
    {
        return m_world.isWithinWorldBounds(x, y, z);
    }

    // IWorld 接口实现 - 委托到 ClientWorld
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        // 射线检测桩：无方块流体处返回空流体默认状态（走 fluidId 路径，EMPTY()->defaultState()）。
        return &fluid::Fluids::EMPTY()->defaultState();
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override { return m_world.getHeight(x, z); }
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override { return m_world.getBlockLight(x, y, z); }
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override { return m_world.getSkyLight(x, y, z); }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
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
    [[nodiscard]] DimensionId dimension() const override { return m_world.dimensionId(); }
    [[nodiscard]] u64 seed() const override { return m_world.seed(); }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    [[nodiscard]] bool isClientSide() const override { return true; }

    // tickManager 不适用于只读的客户端世界适配器
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        MC_ASSERT_RELEASE_MSG(false, "ClientWorldBlockReader does not support tickManager");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        MC_ASSERT_RELEASE_MSG(false, "ClientWorldBlockReader does not support tickManager");
    }

    // getRandom 不适用于只读的客户端世界适配器
    [[nodiscard]] math::Random& getRandom() override
    {
        MC_ASSERT_RELEASE_MSG(false, "ClientWorldBlockReader does not support getRandom");
    }
    [[nodiscard]] const math::Random& getRandom() const override
    {
        MC_ASSERT_RELEASE_MSG(false, "ClientWorldBlockReader does not support getRandom");
    }

    // WorldBorder 接口
    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    const ClientWorld& m_world;
    world::border::WorldBorder m_worldBorder;
};

} // namespace

void ClientApplication::updateRaycastResult()
{
    // 执行射线检测
    if (m_player && m_mouseCaptured) {
        // 获取玩家眼睛位置
        glm::vec3 eyePos = m_camera.position();

        // 获取视线方向
        glm::vec3 forward = m_camera.forward();

        // 创建射线
        mc::Vector3 origin(eyePos.x, eyePos.y, eyePos.z);
        mc::Vector3 direction(forward.x, forward.y, forward.z);
        mc::Ray ray(origin, direction);

        // 执行射线检测（创造模式使用更远的距离）
        mc::RaycastContext context(ray, 5.0f); // 生存模式5格
        ClientWorldBlockReader blockReader(m_world);
        m_raycastResult = mc::raycastBlocks(context, blockReader);
    } else {
        m_raycastResult = BlockRaycastResult::miss();
    }
}

} // namespace mc::client