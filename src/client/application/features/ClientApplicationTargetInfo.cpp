#include "../ClientApplication.hpp"

#include "common/world/IWorld.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/util/math/random/Random.hpp"

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
    {
    }

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
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return fluid::Fluid::getFluidState(0); }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    [[nodiscard]] bool isClientSide() override { return true; }

    // tickManager 不适用于只读的客户端世界适配器
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        MC_ASSERT_RELEASE_MSG(false, "ClientWorldBlockReader does not support tickManager");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        MC_ASSERT_RELEASE_MSG(false, "ClientWorldBlockReader does not support tickManager");
    }

    // getRandom 不适用于只读的客户端世界适配器
    [[nodiscard]] math::Random& getRandom() override {
        MC_ASSERT_RELEASE_MSG(false, "ClientWorldBlockReader does not support getRandom");
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        MC_ASSERT_RELEASE_MSG(false, "ClientWorldBlockReader does not support getRandom");
    }

private:
    const ClientWorld& m_world;
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
        mc::RaycastContext context(ray, 5.0f);  // 生存模式5格
        ClientWorldBlockReader blockReader(m_world);
        m_raycastResult = mc::raycastBlocks(context, blockReader);
    } else {
        m_raycastResult = BlockRaycastResult::miss();
    }
}

} // namespace mc::client