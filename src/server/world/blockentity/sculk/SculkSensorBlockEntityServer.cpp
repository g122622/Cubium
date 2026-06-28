/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

/**
 * @file SculkSensorBlockEntityServer.cpp
 * @brief 幽匿感测体方块实体 - 服务端 VibrationSystem::User 实现
 *
 * 实现 SculkSensorBlockEntity 对应的 VibrationSystem::User 子类，
 * 用于将振动系统与方块实体连接。该类在服务端目录中，
 * 因为 VibrationSystem::User::canReceiveVibration 和 onReceiveVibration
 * 需要 ServerWorld 参数。
 */

#include "common/entity/core/Entity.hpp"
#include "common/world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc::blockentity {

/**
 * @brief 幽匿感测体的 VibrationSystem::User 实现
 *
 * 配置检测半径为 8 格，处理振动接收回调。
 * 通过指针引用 SculkSensorBlockEntity 来更新振动频率数据。
 */
class SculkSensorVibrationUser : public gameevent::VibrationSystem::User {
public:
    explicit SculkSensorVibrationUser(SculkSensorBlockEntity& entity)
        : m_entity(entity)
        , m_positionSource(entity.getPos())
    {}

    [[nodiscard]] i32 getListenerRadius() const override { return LISTENER_RADIUS; }

    [[nodiscard]] gameevent::PositionSource& getPositionSource() override { return m_positionSource; }
    [[nodiscard]] const gameevent::PositionSource& getPositionSource() const override { return m_positionSource; }

    [[nodiscard]] bool canReceiveVibration(server::ServerWorld& world,
        const BlockPos& pos,
        const gameevent::GameEvent& event,
        const gameevent::GameEvent::Context& context) const override;

    void onReceiveVibration(server::ServerWorld& world,
        const BlockPos& pos,
        const gameevent::GameEvent& event,
        const Entity* sourceEntity,
        f32 distance) override;

    [[nodiscard]] bool requiresAdjacentChunksToBeTicking() const override { return true; }
    [[nodiscard]] bool canTriggerAvoidVibration() const override { return true; }

    void onDataChanged() override { m_entity.setChanged(); }

private:
    SculkSensorBlockEntity& m_entity;
    gameevent::BlockPositionSource m_positionSource;

    /// 幽匿感测体的检测半径（格）
    static constexpr i32 LISTENER_RADIUS = 8;
};

bool SculkSensorVibrationUser::canReceiveVibration(server::ServerWorld& world,
    const BlockPos& pos,
    const gameevent::GameEvent& event,
    const gameevent::GameEvent::Context& context) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 基本验证：事件频率不能为0
    i32 frequency = gameevent::VibrationSystem::getGameEventFrequency(event);
    if (frequency == 0) {
        return false;
    }

    // 潜行实体产生的事件可被忽略
    if (gameevent::VibrationSystem::isIgnoredBySneaking(event)) {
        if (context.sourceEntity() != nullptr && context.sourceEntity()->isSteppingCarefully()) {
            return false;
        }
    }

    return true;
}

void SculkSensorVibrationUser::onReceiveVibration(server::ServerWorld& world,
    const BlockPos& pos,
    const gameevent::GameEvent& event,
    const Entity* sourceEntity,
    f32 distance)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(sourceEntity);
    MC_UNUSED(distance);

    // 更新最后振动频率
    i32 frequency = gameevent::VibrationSystem::getGameEventFrequency(event);
    m_entity.setLastVibrationFrequency(frequency);

    // 标记方块实体已修改（需要保存）
    m_entity.setChanged();

    // TODO: 触发幽匿感测体方块状态变化（ACTIVE_PHASE）和红石信号更新
    // 当前仅更新频率数据，方块状态变化由 SculkSensorBlock 的 tick 逻辑处理
}

} // namespace mc::blockentity
