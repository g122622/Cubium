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
    // MC 1.16.5: IRideable.ride()
    if (!mount.isAlive()) {
        return false;
    }

    // 获取第一个乘客
    const auto& passengerList = mount.getPassengers();
    Entity* controllingPassenger = passengerList.empty() ? nullptr : (mount.world() ? mount.world()->getEntity(passengerList[0]) : nullptr);

    // 检查是否可以被骑乘控制
    if (mount.isBeingRidden() && canBeSteered() && controllingPassenger != nullptr) {
        // 获取玩家（控制乘客）
        Player* player = dynamic_cast<Player*>(controllingPassenger);
        if (player != nullptr) {
            // 同步朝向
            mount.setRotation(player->yaw(), player->pitch() * 0.5f);

            // MC 1.16.5: 设置步进高度
            // mount.stepHeight = 1.0F;

            // 更新加速状态
            if (helper.saddledRaw && helper.boostingTick++ > helper.boostTimeRaw) {
                helper.saddledRaw = false;
            }

            // 检查是否可以控制方向
            if (mount.canPassengerSteer()) {
                f32 speed = getSteeringSpeed();

                // 加速计算
                if (helper.saddledRaw) {
                    // MC 1.16.5: f += f * 1.15F * MathHelper.sin((float)helper.field_233611_b_ / (float)helper.boostTimeRaw * (float)Math.PI);
                    f32 progress = static_cast<f32>(helper.boostingTick) / static_cast<f32>(helper.boostTimeRaw);
                    f32 boostFactor = 1.0f + 1.15f * std::sin(progress * math::PI);
                    speed *= boostFactor;
                }

                // 设置AI移动速度
                // mount.setAIMoveSpeed(speed);

                // 执行移动
                travelTowards(Vector3(0.0f, 0.0f, 1.0f));

                // 重置位置旋转增量
                // mount.newPosRotationIncrements = 0;
            } else {
                // 无法控制时停止移动
                // mount.func_233629_a_(mount, false);
                mount.setVelocity(Vector3(0.0f, 0.0f, 0.0f));
            }

            return true;
        }
    }

    // 未被骑乘或无法控制时，使用普通移动
    // MC 1.16.5: mount.stepHeight = 0.5F;
    // mount.jumpMovementFactor = 0.02F;
    travelTowards(travelVec);
    return false;
}

} // namespace entity
} // namespace mc
