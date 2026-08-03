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

#include "IRideable.hpp"
#include "../../util/math/MathConstants.hpp"
#include "../../world/IWorld.hpp"
#include "../core/Entity.hpp"
#include "../core/MobEntity.hpp"
#include "../entities/player/Player.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/interfaces/BoostHelper.hpp"
#include <cmath>

namespace mc {
namespace entity {

bool IRideable::ride(MobEntity& mount, BoostHelper& helper, const Vector3& travelVec)
{
    if (!mount.isAlive()) {
        return false;
    }

    // 获取第一个乘客
    const auto& passengerList = mount.getPassengers();
    Entity* controllingPassenger =
        passengerList.empty() ? nullptr : (mount.world() ? mount.world()->getEntity(passengerList[0]) : nullptr);

    if (mount.isBeingRidden() && canBeSteered() && controllingPassenger != nullptr) {
        Player* player = dynamic_cast<Player*>(controllingPassenger);
        if (player != nullptr) {
            // 同步朝向
            mount.setRotation(player->yaw(), player->pitch() * 0.5f);

            // 骑乘时步进高度增加到1.0，允许跨越1格高的方块
            mount.setStepHeight(1.0f);

            // 更新加速状态
            if (helper.saddledRaw && helper.field_233611_b_++ > helper.boostTimeRaw) {
                helper.saddledRaw = false;
            }

            if (mount.canPassengerSteer()) {
                f32 speed = getSteeringSpeed();

                // 加速计算
                if (helper.saddledRaw) {
                    f32 progress = static_cast<f32>(helper.field_233611_b_) / static_cast<f32>(helper.boostTimeRaw);
                    speed += speed * 1.15f * std::sin(progress * math::PI);
                }

                mount.setAIMoveSpeed(speed);

                travelTowards(Vector3(0.0f, 0.0f, 1.0f));

                // 重置位置旋转增量，用于客户端插值
            } else {
                mount.setVelocity(Vector3(0.0f, 0.0f, 0.0f));
            }

            return true;
        }
    }

    // 未被骑乘或无法控制时
    mount.setStepHeight(0.5f);
    travelTowards(travelVec);
    return false;
}

} // namespace entity
} // namespace mc
