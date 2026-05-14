#include "IRideable.hpp"
#include "../core/BoostHelper.hpp"
#include "../core/MobEntity.hpp"
#include "../core/Entity.hpp"
#include "../entities/player/Player.hpp"
#include "../../world/IWorld.hpp"
#include "../../util/math/MathConstants.hpp"
#include "../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc {
namespace entity {

bool IRideable::ride(MobEntity& mount, BoostHelper& helper, const Vector3& travelVec) {
    // MC 1.16.5: IRideable.ride(MobEntity, BoostHelper, Vector3d)
    if (!mount.isAlive()) {
        return false;
    }

    // 获取第一个乘客
    const auto& passengerList = mount.getPassengers();
    Entity* controllingPassenger = passengerList.empty() ? nullptr : (mount.world() ? mount.world()->getEntity(passengerList[0]) : nullptr);

    // MC 1.16.5: if (mount.isBeingRidden() && mount.canBeSteered() && entity instanceof PlayerEntity)
    if (mount.isBeingRidden() && canBeSteered() && controllingPassenger != nullptr) {
        Player* player = dynamic_cast<Player*>(controllingPassenger);
        if (player != nullptr) {
            // MC 1.16.5: 同步朝向
            // mount.rotationYaw = entity.rotationYaw;
            // mount.prevRotationYaw = mount.rotationYaw;
            // mount.rotationPitch = entity.rotationPitch * 0.5F;
            // mount.setRotation(mount.rotationYaw, mount.rotationPitch);
            // mount.renderYawOffset = mount.rotationYaw;
            // mount.rotationYawHead = mount.rotationYaw;
            mount.setRotation(player->yaw(), player->pitch() * 0.5f);

            // MC 1.16.5: mount.stepHeight = 1.0F;
            // 骑乘时步进高度增加到1.0，允许跨越1格高的方块
            // mount.setStepHeight(1.0f);

            // MC 1.16.5: mount.jumpMovementFactor = mount.getAIMoveSpeed() * 0.1F;
            // 骑乘时跳跃移动因子基于AI移动速度
            // mount.setJumpMovementFactor(mount.getAIMoveSpeed() * 0.1f);

            // MC 1.16.5: 更新加速状态
            // if (helper.saddledRaw && helper.field_233611_b_++ > helper.boostTimeRaw) {
            //     helper.saddledRaw = false;
            // }
            if (helper.saddledRaw && helper.field_233611_b_++ > helper.boostTimeRaw) {
                helper.saddledRaw = false;
            }

            // MC 1.16.5: if (mount.canPassengerSteer())
            if (mount.canPassengerSteer()) {
                // MC 1.16.5: float f = this.getMountedSpeed();
                f32 speed = getSteeringSpeed();
                MC_UNUSED(speed);

                // MC 1.16.5: 加速计算
                // if (helper.saddledRaw) {
                //     f += f * 1.15F * MathHelper.sin((float)helper.field_233611_b_ / (float)helper.boostTimeRaw * (float)Math.PI);
                // }
                if (helper.saddledRaw) {
                    f32 progress = static_cast<f32>(helper.field_233611_b_) / static_cast<f32>(helper.boostTimeRaw);
                    f32 boostFactor = 1.0f + 1.15f * std::sin(progress * math::PI);
                    MC_UNUSED(boostFactor);
                }

                // MC 1.16.5: mount.setAIMoveSpeed(f);
                // mount.setAIMoveSpeed(speed);

                // MC 1.16.5: this.travelTowards(new Vector3d(0.0D, 0.0D, 1.0D));
                travelTowards(Vector3(0.0f, 0.0f, 1.0f));

                // MC 1.16.5: mount.newPosRotationIncrements = 0;
                // 重置位置旋转增量，用于客户端插值
            } else {
                // MC 1.16.5: mount.func_233629_a_(mount, false);
                // mount.setMotion(Vector3d.ZERO);
                mount.setVelocity(Vector3(0.0f, 0.0f, 0.0f));
            }

            return true;
        }
    }

    // MC 1.16.5: 未被骑乘或无法控制时
    // mount.stepHeight = 0.5F;
    // mount.jumpMovementFactor = 0.02F;
    // mount.setStepHeight(0.5f);
    // mount.setJumpMovementFactor(0.02f);
    travelTowards(travelVec);
    return false;
}

} // namespace entity
} // namespace mc
