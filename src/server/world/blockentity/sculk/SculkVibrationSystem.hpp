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

/**
 * @file SculkVibrationSystem.hpp
 * @brief 幽匿方块实体的振动系统附件
 *
 * 为 SculkSensorBlockEntity 和 SculkShriekerBlockEntity 提供
 * VibrationSystem 的服务端集成。由于这些方块实体位于 mc_common
 * （不能依赖 mc_server），而 VibrationSystem::Listener 的
 * handleGameEvent() 实现需要 ServerWorld，因此 VibrationSystem
 * 子类在 mc_server 中定义，并通过本文件中的管理器与方块实体关联。
 *
 * SculkVibrationSystem 是 VibrationSystem 的具体子类，持有对
 * 方块实体的 Data 和 User 的引用，同时拥有 Listener 实例。
 * SculkVibrationManager 负责管理所有幽匿方块实体的振动系统附件，
 * 并在 ServerWorld 的生命周期钩子中驱动注册、注销和 tick。
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "common/world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEventListener.hpp"
#include "common/world/gameevent/GameEventListenerRegistry.hpp"
#include "common/world/gameevent/PositionSource.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

#include <memory>
#include <optional>
#include <unordered_map>

namespace mc::server {

class ServerWorld;
class ServerChunkManager;

// ============================================================================
// 幽匿感测体 VibrationSystem::User 实现
// ============================================================================

/**
 * @brief 幽匿感测体的 VibrationSystem::User 实现
 *
 * 配置检测半径为 8 格，处理振动接收回调。
 */
class SculkSensorVibrationUser : public gameevent::VibrationSystem::User {
public:
    explicit SculkSensorVibrationUser(blockentity::SculkSensorBlockEntity& entity)
        : m_entity(entity)
        , m_positionSource(entity.getPos())
    {}

    [[nodiscard]] i32 getListenerRadius() const override { return LISTENER_RADIUS; }

    [[nodiscard]] gameevent::PositionSource& getPositionSource() override { return m_positionSource; }
    [[nodiscard]] const gameevent::PositionSource& getPositionSource() const override { return m_positionSource; }

    [[nodiscard]] bool canReceiveVibration(ServerWorld& world,
        const BlockPos& pos,
        const gameevent::GameEvent& event,
        const gameevent::GameEvent::Context& context) const override;

    void onReceiveVibration(ServerWorld& world,
        const BlockPos& pos,
        const gameevent::GameEvent& event,
        const Entity* sourceEntity,
        f32 distance) override;

    [[nodiscard]] bool requiresAdjacentChunksToBeTicking() const override { return true; }
    [[nodiscard]] bool canTriggerAvoidVibration() const override { return true; }

    /**
     * @brief 标识此 User 为感测体（而非尖啸体）
     *
     * 用于 SculkVibrationManager::tickAll() 中区分感测体和尖啸体，
     * 以便仅对尖啸体检查 SHRIEKING 结束标志。
     */
    [[nodiscard]] bool isSculkShrieker() const override { return false; }

    void onDataChanged() override { m_entity.setChanged(); }

private:
    blockentity::SculkSensorBlockEntity& m_entity;
    gameevent::BlockPositionSource m_positionSource;

    /// 幽匿感测体的检测半径（格）
    static constexpr i32 LISTENER_RADIUS = 8;
};

// ============================================================================
// 幽匿尖啸体 VibrationSystem::User 实现
// ============================================================================

/**
 * @brief 幽匿尖啸体的 VibrationSystem::User 实现
 *
 * 配置检测半径为 8 格，仅响应 SHRIEK 事件。
 */
class SculkShriekerVibrationUser : public gameevent::VibrationSystem::User {
public:
    explicit SculkShriekerVibrationUser(blockentity::SculkShriekerBlockEntity& entity)
        : m_entity(entity)
        , m_positionSource(entity.getPos())
    {}

    [[nodiscard]] i32 getListenerRadius() const override { return LISTENER_RADIUS; }

    [[nodiscard]] gameevent::PositionSource& getPositionSource() override { return m_positionSource; }
    [[nodiscard]] const gameevent::PositionSource& getPositionSource() const override { return m_positionSource; }

    [[nodiscard]] bool canReceiveVibration(ServerWorld& world,
        const BlockPos& pos,
        const gameevent::GameEvent& event,
        const gameevent::GameEvent::Context& context) const override;

    void onReceiveVibration(ServerWorld& world,
        const BlockPos& pos,
        const gameevent::GameEvent& event,
        const Entity* sourceEntity,
        f32 distance) override;

    [[nodiscard]] bool requiresAdjacentChunksToBeTicking() const override { return true; }
    [[nodiscard]] bool canTriggerAvoidVibration() const override { return false; }

    /**
     * @brief 标识此 User 为尖啸体（而非感测体）
     *
     * 用于 SculkVibrationManager::tickAll() 中区分感测体和尖啸体，
     * 以便仅对尖啸体检查 SHRIEKING 结束标志。
     */
    [[nodiscard]] bool isSculkShrieker() const override { return true; }

    void onDataChanged() override { m_entity.setChanged(); }

private:
    blockentity::SculkShriekerBlockEntity& m_entity;
    gameevent::BlockPositionSource m_positionSource;

    /// 幽匿尖啸体的检测半径（格）
    static constexpr i32 LISTENER_RADIUS = 8;
};

// ============================================================================
// SculkVibrationSystem: VibrationSystem 的具体子类
// ============================================================================

/**
 * @brief 幽匿方块实体共享的 VibrationSystem 子类
 *
 * 持有 VibrationUser 和 VibrationSystem::Listener，通过引用
 * 访问方块实体的 VibrationSystem::Data。该类在 mc_server 中实例化，
 * 避免 mc_common 中的方块实体依赖 ServerWorld。
 */
class SculkVibrationSystem : public gameevent::VibrationSystem {
public:
    /**
     * @brief 为幽匿感测体构造振动系统
     */
    SculkVibrationSystem(blockentity::SculkSensorBlockEntity& entity)
        : m_sensorUser(entity)
        , m_data(entity.getVibrationData())
    {}

    /**
     * @brief 为幽匿尖啸体构造振动系统
     */
    SculkVibrationSystem(blockentity::SculkShriekerBlockEntity& entity)
        : m_shriekerUser(entity)
        , m_data(entity.getVibrationData())
    {}

    [[nodiscard]] Data& getVibrationData() override { return m_data; }
    [[nodiscard]] const Data& getVibrationData() const override { return m_data; }

    [[nodiscard]] User& getVibrationUser() override
    {
        if (m_sensorUser.has_value()) {
            return m_sensorUser.value();
        }
        return m_shriekerUser.value();
    }

    [[nodiscard]] const User& getVibrationUser() const override
    {
        if (m_sensorUser.has_value()) {
            return m_sensorUser.value();
        }
        return m_shriekerUser.value();
    }

private:
    /// 感测体 User（与尖啸体 User 互斥）
    std::optional<SculkSensorVibrationUser> m_sensorUser;
    /// 尖啸体 User（与感测体 User 互斥）
    std::optional<SculkShriekerVibrationUser> m_shriekerUser;
    /// 对方块实体 Data 的引用（共享，不拥有）
    Data& m_data;
};

// ============================================================================
// SculkVibrationManager: 幽匿振动系统管理器
// ============================================================================

/**
 * @brief 管理所有幽匿方块实体的振动系统附件
 *
 * 负责：
 * - 在方块实体添加到世界时，创建 VibrationSystem 并注册 Listener
 * - 在方块实体从世界移除时，注销 Listener 并销毁 VibrationSystem
 * - 每 tick 驱动所有 VibrationSystem 的 Ticker
 */
class SculkVibrationManager {
public:
    SculkVibrationManager() = default;

    /**
     * @brief 设置 ServerWorld 引用
     *
     * 必须在首次使用前调用。因为 SculkVibrationManager 是 ServerWorld
     * 的成员，在构造时 ServerWorld 尚未完成构造，因此需要延迟设置。
     */
    void setWorld(ServerWorld& world) { m_world = &world; }

    /**
     * @brief 为幽匿感测体注册振动系统
     *
     * 创建 VibrationSystem 附件并注册 Listener 到区块的 GameEventListenerRegistry。
     */
    void registerSculkSensor(blockentity::SculkSensorBlockEntity& entity);

    /**
     * @brief 为幽匿尖啸体注册振动系统
     *
     * 创建 VibrationSystem 附件并注册 Listener 到区块的 GameEventListenerRegistry。
     */
    void registerSculkShrieker(blockentity::SculkShriekerBlockEntity& entity);

    /**
     * @brief 注销幽匿方块实体的振动系统
     *
     * 从区块的 GameEventListenerRegistry 注销 Listener 并销毁 VibrationSystem 附件。
     */
    void unregisterSculkBlockEntity(const BlockPos& pos);

    /**
     * @brief 驱动所有已注册幽匿方块实体的振动 tick
     */
    void tickAll();

private:
    /**
     * @brief 在区块中注册 Listener 到 GameEventListenerRegistry
     */
    void registerListenerInChunk(const BlockPos& pos, gameevent::GameEventListener& listener);

    /**
     * @brief 从区块的 GameEventListenerRegistry 注销 Listener
     */
    void unregisterListenerFromChunk(const BlockPos& pos, gameevent::GameEventListener& listener);

    ServerWorld* m_world = nullptr;

    /// 位置到振动系统的映射
    std::unordered_map<BlockPos, std::unique_ptr<SculkVibrationSystem>> m_vibrationSystems;
};

} // namespace mc::server
