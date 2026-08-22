#pragma once

#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/systems/base/EntityView.hpp"

namespace mc::ecs::sys {

/**
 * @brief 骑乘载具乘客位置同步 tick 系统（free function）
 *
 * 承载原 Entity::baseTick() 末尾的 updatePassengers() 调用（载具主动同步自身所有乘客位置），
 * 逐字搬迁为遍历组件的真实 system。注册到 SystemPhase::PostEntityTick 阶段——在 EntityTick
 * （逐实体 OOP Entity::tick() 含 aiStep/travel/move 移动）**之后**执行，载具本帧已移动到新位置，
 * 乘客位置同步读到载具当前位置，同帧收敛，消除原 baseTick 末尾同步的 1-tick 滞后。
 *
 * ## 时序对比（阶段 E 迁移核心收益）
 * - 阶段 E 前：baseTick 末尾调 updatePassengers（baseTick 是实体 tick 第一步，载具本 tick 后续
 *   aiStep/travel/move 尚未应用），乘客读到上一帧载具位置，滞后 1 tick。
 * - 阶段 E 后：rideTick 在 PostEntityTick（所有实体 OOP tick 完成后），载具已移动，乘客同帧收敛。
 * - 注意：PostMovement 阶段在 EntityTick **之前**（见 SystemPhase 枚举顺序），载具本帧移动仍未
 *   发生，故必须注册 PostEntityTick 而非 PostMovement（计划原假设 PostMovement 是对枚举顺序的
 *   误读，见 ecs/README 坑23）。
 *
 * ## 经虚 updatePassengerPosition 派发（阶段 E 关键架构修复）
 * updatePassengers() 内部对每个乘客调虚 updatePassengerPosition()（基类默认实现调 positionRider
 * 做 getMountedYOffset 基础定位）。子类 override 此方法实现载具特化定位：
 * - BoatEntity::updatePassengerPosition → updateAllPassengerPositions（朝向旋转+多乘客偏移）
 * - AbstractHorseEntity::updatePassengerPosition（扬蹄 prevRearingAmount 偏移，对齐 MC 1.16.5）
 * 阶段 E 前原 baseTick:850 调 updatePassengers，但 updatePassengers 内部直调非虚 positionRider
 * 绕过子类 override（致 boat/horse 的 override 成死代码、boat 乘客位置错误、horse 扬蹄偏移失效）。
 * 阶段 E 改 updatePassengers 调虚 updatePassengerPosition，子类 override 生效。
 *
 * ## 载具同步现状（阶段 E 清理后统一）
 * 阶段 E 前：baseTick:850（所有载具，滞后）+ MinecartEntity::tick:190（移动后，双重）+
 * AbstractHorseEntity::updateRiding:986（移动后，双重）+ BoatEntity::tick:236
 * updateAllPassengerPositions（移动后，独立实现）。阶段 E 删除前 4 处，统一由本 system 在
 * PostEntityTick 调 updatePassengers 同步。PigEntity/StriderEntity 等无末尾同步的载具原本完全
 * 靠 baseTick:850（滞后），迁移后由 rideTick 同步（无滞后，行为变好）。
 *
 * ## 首批不建 PassengerComponent
 * m_passengers（std::vector<EntityInstanceId>）仍 OOP 成员（Entity.hpp），乘客数少遍历成本低，
 * 经 EntityOwnerComponent 反查 OOP 句柄调 hasPassengers()/updatePassengers()。阶段 H 并行化时
 * 若需避免 OOP 句柄竞争再组件化。
 *
 * ## 签名与依赖推导
 * 参数为 entt 原生 basic_view（经 mc::ecs::EntityView 别名绑定 EntityId）。organizer 从
 * EntityOwnerComponent&（非 const，rw）推导反查句柄依赖。view 只列 EntityOwnerComponent（本
 * system 不读写任何组件，仅经 OOP 句柄调虚函数同步乘客位置——乘客位置写入走 passenger.setPosition
 * 经 StateVectorComponent，但那是乘客实体的组件非载具的，不在本 view 范围）。
 */
void rideTick(entt::basic_registry<EntityId>& registry, mc::ecs::EntityView<entt::get_t<EntityOwnerComponent>> view);

} // namespace mc::ecs::sys
