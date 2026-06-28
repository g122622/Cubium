/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software
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
 * @file SculkShriekerBlockEntityServer.cpp
 * @brief 幽匿尖啸体方块实体 - 服务端 VibrationSystem::User 实现
 *
 * 实现 SculkShriekerBlockEntity 对应的 VibrationSystem::User 子类，
 * 用于将振动系统与方块实体连接。该类在服务端目录中，
 * 因为 VibrationSystem::User::canReceiveVibration 和 onReceiveVibration
 * 需要 ServerWorld 参数。
 *
 * TODO: 集成步骤（需在 ServerWorld 中完成）：
 * 1. 在 ServerWorld::setBlockEntity() 中检测 SculkShriekerBlockEntity，
 *    创建 SculkShriekerVibrationUser 和 VibrationSystem::Listener，
 *    并注册到区块的 GameEventListenerRegistry
 * 2. 在 ServerWorld::tickBlockEntities() 中对 SculkShriekerBlockEntity
 *    调用 VibrationSystem::Ticker::tick()
 * 3. 在 ServerWorld::removeBlockEntity() 中注销 Listener
 */

#include "common/entity/core/Entity.hpp"
#include "common/world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc::blockentity {

/**
 * @brief 幽匿尖啸体的 VibrationSystem::User 实现
 *
 * 配置检测半径为 8 格，仅响应 SHRIEK 事件。
 * 通过指针引用 SculkShriekerBlockEntity 来递增警告等级。
 */
class SculkShriekerVibrationUser : public gameevent::VibrationSystem::User {
public:
    explicit SculkShriekerVibrationUser(SculkShriekerBlockEntity& entity)
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
    [[nodiscard]] bool canTriggerAvoidVibration() const override { return false; }

    void onDataChanged() override { m_entity.setChanged(); }

private:
    SculkShriekerBlockEntity& m_entity;
    gameevent::BlockPositionSource m_positionSource;

    /// 幽匿尖啸体的检测半径（格）
    static constexpr i32 LISTENER_RADIUS = 8;
};

bool SculkShriekerVibrationUser::canReceiveVibration(server::ServerWorld& world,
    const BlockPos& pos,
    const gameevent::GameEvent& event,
    const gameevent::GameEvent::Context& context) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 幽匿尖啸体只响应 SHRIEK 事件（来自其他尖啸体）
    // 对齐 MC 原版：SculkShriekerBlockEntity.VibrationUser.canReceiveVibration()
    if (&event != &gameevent::GameEvents::SHRIEK) {
        return false;
    }

    // 潜行实体产生的事件可被忽略
    if (context.sourceEntity() != nullptr && context.sourceEntity()->isSteppingCarefully()) {
        return false;
    }

    return true;
}

void SculkShriekerVibrationUser::onReceiveVibration(server::ServerWorld& world,
    const BlockPos& pos,
    const gameevent::GameEvent& event,
    const Entity* sourceEntity,
    f32 distance)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(event);
    MC_UNUSED(sourceEntity);
    MC_UNUSED(distance);

    // 递增警告等级
    m_entity.incrementWarningLevel();

    // 标记方块实体已修改（需要保存）
    m_entity.setChanged();

    // TODO: 当警告等级达到阈值时，触发监守者召唤逻辑
    // 当前仅递增警告等级，召唤逻辑待 WardenEntity 实现后集成
}

} // namespace mc::blockentity
