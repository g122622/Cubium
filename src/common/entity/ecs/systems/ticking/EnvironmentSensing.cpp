#include "common/entity/ecs/systems/ticking/EnvironmentSensing.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/EnvironmentStateComponent.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/fluid/Fluid.hpp"

#include <algorithm>
#include <cmath>

namespace mc::ecs::sys {

void environmentSensing(entt::basic_registry<EntityId>& /*registry*/,
    mc::ecs::EntityView<entt::get_t<EnvironmentStateComponent, EntityOwnerComponent>> view)
{
    // 遍历所有挂 EnvironmentStateComponent 的实体。EntityOwnerComponent 持 OOP Entity&，
    // 供调用虚函数 eyeHeight()/boundingBox() 与取世界/物理引擎句柄查流体。
    // 七字段每帧重写（先重置再累加），与原 baseTick 每帧调 updateEnvironmentState 语义一致。
    for (auto [entityId, env, owner] : view.each()) {
        Entity* entity = owner.tryGetEntity();
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }

        // 眼睛位置（用于判断眼睛是否在水下）。eyeHeight() 经虚函数派发到各子类。
        const f32 eyeY = entity->position().y + entity->eyeHeight();

        // 重置流体状态（原 updateEnvironmentState 行 910-915 逐字搬迁）。
        env.inWater = false;
        env.inLava = false;
        env.waterHeight = 0.0f;
        env.lavaHeight = 0.0f;
        env.eyesInWater = false;
        env.eyesInLava = false;

        IWorld* world = entity->world();
        PhysicsEngine* physicsEngine = entity->physicsEngine();
        if (world == nullptr && physicsEngine == nullptr) {
            // 无世界也无物理引擎：保留刚重置的默认值，fluidHeight 亦清零。
            env.fluidHeight = 0.0f;
            continue;
        }

        // 获取碰撞箱并收缩一点以避免边界问题。
        AxisAlignedBB box = entity->boundingBox().shrink(0.001);

        // 计算碰撞箱覆盖的方块范围。
        const i32 minX = static_cast<i32>(std::floor(box.minX));
        const i32 maxX = static_cast<i32>(std::floor(box.maxX));
        const i32 minY = static_cast<i32>(std::floor(box.minY));
        const i32 maxY = static_cast<i32>(std::floor(box.maxY));
        const i32 minZ = static_cast<i32>(std::floor(box.minZ));
        const i32 maxZ = static_cast<i32>(std::floor(box.maxZ));

        // 遍历碰撞箱内的所有方块。
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = minZ; z <= maxZ; ++z) {
                    // 获取流体状态。主路径走 world->getFluidState，fallback 走物理引擎的碰撞世界。
                    const fluid::FluidState* fluidState = nullptr;
                    if (world) {
                        fluidState = world->getFluidState(x, y, z);
                    } else if (physicsEngine) {
                        const ICollisionWorld* collisionWorld = physicsEngine->getWorld();
                        if (collisionWorld) {
                            const BlockState* blockState = collisionWorld->getBlockState(x, y, z);
                            fluidState = blockState != nullptr ? blockState->getFluidState() : nullptr;
                        }
                    }

                    if (fluidState == nullptr || fluidState->isEmpty()) {
                        continue;
                    }

                    // 计算流体高度（方块底 Y + 流体实际高度）。
                    f32 fluidTopY = static_cast<f32>(y) + fluidState->getHeight();

                    // 检查流体是否在碰撞箱内。
                    if (fluidTopY > box.minY) {
                        // 计算浸入高度。
                        f32 submergedHeight = fluidTopY - box.minY;

                        // 判断流体类型。
                        const ResourceLocation& fluidId = fluidState->getFluid().fluidLocation();
                        bool isWater = fluidId.namespace_() == "minecraft" &&
                            (fluidId.path() == "water" || fluidId.path() == "flowing_water");
                        bool isLava = fluidId.namespace_() == "minecraft" &&
                            (fluidId.path() == "lava" || fluidId.path() == "flowing_lava");

                        if (isWater) {
                            env.inWater = true;
                            env.waterHeight = std::max(env.waterHeight, submergedHeight);

                            // 检查眼睛是否在水下（眼睛位置下移 0.11111111 检测）。
                            constexpr f32 EYE_OFFSET = 0.11111111f;
                            f32 adjustedEyeY = eyeY - EYE_OFFSET;
                            if (fluidTopY > adjustedEyeY) {
                                env.eyesInWater = true;
                            }
                        } else if (isLava) {
                            env.inLava = true;
                            env.lavaHeight = std::max(env.lavaHeight, submergedHeight);

                            // 检查眼睛是否在岩浆中。
                            constexpr f32 EYE_OFFSET = 0.11111111f;
                            f32 adjustedEyeY = eyeY - EYE_OFFSET;
                            if (fluidTopY > adjustedEyeY) {
                                env.eyesInLava = true;
                            }
                        }
                    }
                }
            }
        }

        // 兼容旧代码：fluidHeight = max(waterHeight, lavaHeight)。
        env.fluidHeight = std::max(env.waterHeight, env.lavaHeight);
    }
}

} // namespace mc::ecs::sys
