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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEventListener.hpp"
#include "common/world/gameevent/PositionSource.hpp"

#include <algorithm>
#include <optional>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {

namespace nbt {
namespace tags {
struct compound_tag;
} // namespace tags
using CompoundTag = tags::compound_tag;
} // namespace nbt

class Entity; // 前向声明

namespace server {
class ServerWorld; // 前向声明
} // namespace server

namespace gameevent {

/**
 * @brief 振动信息
 *
 * 记录一次振动的详细信息，包括事件类型、距离、来源位置和源实体。
 * 由 VibrationSelector 管理候选振动，被 VibrationSystem.Ticker 消费。
 *
 */
struct VibrationInfo {
    /** 触发的游戏事件 */
    const GameEvent* gameEvent = nullptr;

    /** 振动源到监听器的距离 */
    f32 distance = 0.0f;

    /** 振动源位置 */
    Vector3d pos;

    /** 源实体UUID（可为空） */
    u64 sourceEntityId = 0;

    /** 是否有源实体 */
    bool hasSourceEntity = false;

    VibrationInfo() = default;

    VibrationInfo(const GameEvent& event, f32 dist, const Vector3d& position, const Entity* entity);

    // ========================================================================
    // 序列化
    // ========================================================================

    /**
     * @brief 保存到 NBT 复合标签
     *
     * NBT 结构（对齐 MC 原版 VibrationInfo.CODEC）：
     * - "game_event": string  - 事件 ID（如 "minecraft:step"）
     * - "distance": float     - 振动传播距离
     * - "pos": list<double>   - 振动源位置 [x, y, z]
     * - "source": long        - 源实体 ID（可选）
     * - "projectile_owner": long - 弹射物拥有者实体 ID（可选）
     *
     * @param tag 输出 NBT 复合标签
     */
    void saveToNBT(nbt::CompoundTag& tag) const;

    /**
     * @brief 从 NBT 复合标签加载
     *
     * 如果 "game_event" 缺失或无法识别，gameEvent 将为 nullptr。
     *
     * @param tag 输入 NBT 复合标签
     * @return 是否成功加载（至少包含有效的 game_event）
     */
    [[nodiscard]] bool loadFromNBT(const nbt::CompoundTag& tag);

    /**
     * @brief 保存到 JSON
     * @param data 输出 JSON 对象
     */
    void saveToJson(nlohmann::json& data) const;

    /**
     * @brief 从 JSON 加载
     * @param data 输入 JSON 对象
     * @return 是否成功加载
     */
    [[nodiscard]] bool loadFromJson(const nlohmann::json& data);
};

/**
 * @brief 振动选择器
 *
 * 当同一 tick 内有多个振动到达时，选择最优先的振动。
 * 选择规则：同一 tick 内，距离最近的振动优先；距离相同时，
 * 频率更高的振动优先。
 *
 */
class VibrationSelector {
public:
    VibrationSelector() = default;

    /**
     * @brief 添加候选振动
     * @param info 振动信息
     * @param gameTick 当前游戏 tick
     */
    void addCandidate(VibrationInfo info, u64 gameTick);

    /**
     * @brief 获取当前 tick 应该处理的候选振动
     *
     * 只有在添加 tick < 当前 tick 时才返回候选（确保振动至少延迟1 tick）。
     *
     * @param currentTick 当前游戏 tick
     * @return 候选振动，如果没有则返回空
     */
    [[nodiscard]] std::optional<VibrationInfo> chosenCandidate(u64 currentTick) const;

    /**
     * @brief 重置选择器，开始新一轮选择
     */
    void startOver();

    // ========================================================================
    // 序列化
    // ========================================================================

    /**
     * @brief 保存到 NBT 复合标签
     *
     * NBT 结构（对齐 MC 原版 VibrationSelector.CODEC）：
     * - "event": compound (可选) - 候选振动信息，结构同 VibrationInfo
     * - "tick": long - 候选振动被添加时的游戏 tick，无候选时为 -1
     *
     * @param tag 输出 NBT 复合标签
     */
    void saveToNBT(nbt::CompoundTag& tag) const;

    /**
     * @brief 从 NBT 复合标签加载
     * @param tag 输入 NBT 复合标签
     * @return 是否成功加载
     */
    [[nodiscard]] bool loadFromNBT(const nbt::CompoundTag& tag);

    /**
     * @brief 保存到 JSON
     * @param data 输出 JSON 对象
     */
    void saveToJson(nlohmann::json& data) const;

    /**
     * @brief 从 JSON 加载
     * @param data 输入 JSON 对象
     * @return 是否成功加载
     */
    [[nodiscard]] bool loadFromJson(const nlohmann::json& data);

private:
    /**
     * @brief 判断新振动是否应替换当前候选
     * @param info 新振动
     * @param gameTick 当前 tick
     */
    [[nodiscard]] bool shouldReplaceVibration(const VibrationInfo& info, u64 gameTick) const;

    std::optional<std::pair<VibrationInfo, u64>> m_currentCandidate;
};

/**
 * @brief 振动系统
 *
 * 幽匿感测体、幽匿尖啸体、监守者、悦灵等振动接收器的核心系统。
 * 包含三部分：
 * - Data: 振动状态数据（当前振动、选择器等）
 * - User: 振动接收者的配置接口（半径、可接收事件、回调等）
 * - Listener: 实现 GameEventListener 接口的监听器
 * - Ticker: 每 tick 更新振动传播状态
 *
 */
class VibrationSystem {
public:
    // ========================================================================
    // 振动频率映射
    // ========================================================================

    /**
     * @brief 获取游戏事件对应的振动频率
     *
     * 频率为 0 表示该事件不产生振动信号。
     * 频率 1-15 对应 RESONATE_1 到 RESONATE_15。
     *
     * @param event 游戏事件
     * @return 振动频率 (0-15)
     */
    [[nodiscard]] static i32 getGameEventFrequency(const GameEvent& event);

    /**
     * @brief 检查游戏事件是否可被潜行状态忽略
     *
     * 当源实体正在潜行（isSteppingCarefully）时，
     * 属于此类别的事件不会触发振动信号。
     * 参考: net.minecraft.tags.GameEventTags.IGNORE_VIBRATIONS_SNEAKING
     *
     * 包含的事件：HIT_GROUND, PROJECTILE_SHOOT, STEP, SWIM,
     *            ITEM_INTERACT_START, ITEM_INTERACT_FINISH
     *
     * @param event 游戏事件
     * @return 如果事件可被潜行忽略返回true
     */
    [[nodiscard]] static bool isIgnoredBySneaking(const GameEvent& event);

    /**
     * @brief 根据频率获取共鸣事件
     * @param frequency 振动频率 (1-15)
     * @return 对应的共鸣事件，频率无效时返回 nullptr
     */
    [[nodiscard]] static const GameEvent* getResonanceEventByFrequency(i32 frequency);

    /**
     * @brief 根据振动距离计算红石信号强度
     * @param distance 振动传播距离
     * @param radius 监听器半径
     * @return 红石信号强度 (1-15)
     */
    [[nodiscard]] static i32 getRedstoneStrengthForDistance(f32 distance, i32 radius);

    static constexpr i32 NO_VIBRATION_FREQUENCY = 0;

    // ========================================================================
    // Data: 振动状态数据
    // ========================================================================

    /**
     * @brief 振动系统数据
     *
     * 存储振动系统的运行时状态，包括当前正在传播的振动、
     * 选择器（候选振动管理）和传播时间。
     */
    class Data {
    public:
        Data() = default;

        /**
         * @brief 从存档数据构造振动系统状态
         *
         * 对齐 MC 原版 VibrationSystem.Data CODEC 反序列化行为：
         * 从 NBT 加载时 reloadVibrationParticle 固定为 true，以便在区块重新加载后
         * 重发正在传播的振动粒子效果。新创建的 Data 使用默认构造函数，
         * reloadVibrationParticle 为 false。
         *
         * @param currentVibration 当前正在传播的振动（可为 nullopt）
         * @param selectionStrategy 振动选择器
         * @param travelTimeInTicks 传播剩余时间
         * @param reloadVibrationParticle 是否需要重发振动粒子（从存档加载时为 true）
         *
         * 当 SculkSensorBlockEntity、SculkShriekerBlockEntity、WardenEntity、AllayEntity
         * 从存档数据加载 VibrationSystem.Data 时，应使用此构造函数（或调用 setReloadVibrationParticle(true)），
         * 以便在区块重新加载后重发正在传播的振动粒子效果。参考 MC 原版：
         * - SculkSensorBlockEntity.load() → read("listener", VibrationSystem.Data.CODEC)
         * - SculkShriekerBlockEntity.load() → read("listener", VibrationSystem.Data.CODEC)
         * - Warden.load() → read("listener", VibrationSystem.Data.CODEC)
         * - Allay.load() → read("listener", VibrationSystem.Data.CODEC)
         * MC CODEC 反序列化时硬编码 reloadVibrationParticle = true。
         */
        Data(std::optional<VibrationInfo> currentVibration,
            VibrationSelector selectionStrategy,
            i32 travelTimeInTicks,
            bool reloadVibrationParticle)
            : m_currentVibration(std::move(currentVibration))
            , m_selectionStrategy(std::move(selectionStrategy))
            , m_travelTimeInTicks(travelTimeInTicks)
            , m_reloadVibrationParticle(reloadVibrationParticle)
        {}

        /**
         * @brief 获取当前正在传播的振动
         */
        [[nodiscard]] const VibrationInfo* currentVibration() const
        {
            return m_currentVibration.has_value() ? &m_currentVibration.value() : nullptr;
        }

        /**
         * @brief 设置当前正在传播的振动
         */
        void setCurrentVibration(VibrationInfo info) { m_currentVibration = std::move(info); }

        /**
         * @brief 清除当前振动
         */
        void clearCurrentVibration() { m_currentVibration = std::nullopt; }

        /**
         * @brief 获取振动选择器
         */
        [[nodiscard]] VibrationSelector& selectionStrategy() { return m_selectionStrategy; }
        [[nodiscard]] const VibrationSelector& selectionStrategy() const { return m_selectionStrategy; }

        /**
         * @brief 获取/设置传播剩余时间（tick）
         */
        [[nodiscard]] i32 travelTimeInTicks() const { return m_travelTimeInTicks; }
        void setTravelTimeInTicks(i32 ticks) { m_travelTimeInTicks = ticks; }
        void decrementTravelTime() { m_travelTimeInTicks = std::max(0, m_travelTimeInTicks - 1); }

        /**
         * @brief 获取/设置是否需要重新加载振动粒子
         */
        [[nodiscard]] bool shouldReloadVibrationParticle() const { return m_reloadVibrationParticle; }
        void setReloadVibrationParticle(bool value) { m_reloadVibrationParticle = value; }

        // ========================================================================
        // 序列化
        // ========================================================================

        /**
         * @brief 保存到 NBT 复合标签
         *
         * NBT 结构（对齐 MC 原版 VibrationSystem.Data.CODEC，键名为 "listener"）：
         * - "event": compound (可选) - 当前正在传播的振动信息
         * - "selector": compound     - 振动选择器
         * - "event_delay": int       - 传播剩余时间（tick），默认 0
         *
         * 注意：reloadVibrationParticle 不序列化到 NBT，反序列化时始终设为 true。
         *
         * @param tag 输出 NBT 复合标签
         */
        void saveToNBT(nbt::CompoundTag& tag) const;

        /**
         * @brief 从 NBT 复合标签加载
         *
         * 加载后自动设置 reloadVibrationParticle = true，
         * 以便在区块重新加载后重发振动粒子效果。
         * 对齐 MC 原版 VibrationSystem.Data.CODEC 反序列化行为。
         *
         * @param tag 输入 NBT 复合标签
         * @return 是否成功加载
         */
        [[nodiscard]] bool loadFromNBT(const nbt::CompoundTag& tag);

        /**
         * @brief 保存到 JSON
         * @param data 输出 JSON 对象
         */
        void saveToJson(nlohmann::json& data) const;

        /**
         * @brief 从 JSON 加载
         *
         * 加载后自动设置 reloadVibrationParticle = true，
         * 以便在区块重新加载后重发振动粒子效果。
         *
         * @param data 输入 JSON 对象
         * @return 是否成功加载
         */
        [[nodiscard]] bool loadFromJson(const nlohmann::json& data);

    private:
        std::optional<VibrationInfo> m_currentVibration;
        VibrationSelector m_selectionStrategy;
        i32 m_travelTimeInTicks = 0;
        bool m_reloadVibrationParticle = false;
    };

    // ========================================================================
    // User: 振动接收者配置接口
    // ========================================================================

    /**
     * @brief 振动接收者配置接口
     *
     * 由具体的振动接收者（幽匿感测体、监守者等）实现，
     * 定义检测半径、可接收的事件类型和振动到达回调等。
     */
    class User {
    public:
        virtual ~User() = default;

        /**
         * @brief 获取检测半径
         */
        [[nodiscard]] virtual i32 getListenerRadius() const = 0;

        /**
         * @brief 获取位置源
         */
        [[nodiscard]] virtual PositionSource& getPositionSource() = 0;
        [[nodiscard]] virtual const PositionSource& getPositionSource() const = 0;

        /**
         * @brief 检查是否可以接收此振动
         *
         * 由 VibrationSystem.Listener.handleGameEvent() 调用，
         * 在基本的频率/标签检查之后进行更细致的过滤。
         *
         * @param world 服务端世界
         * @param pos 振动源位置
         * @param event 游戏事件
         * @param context 事件上下文
         * @return 如果可以接收返回 true
         */
        [[nodiscard]] virtual bool canReceiveVibration(server::ServerWorld& world,
            const BlockPos& pos,
            const GameEvent& event,
            const GameEvent::Context& context) const = 0;

        /**
         * @brief 振动到达回调
         *
         * 当振动传播到监听器时调用。此方法实现具体的响应逻辑，
         * 例如激活幽匿感测体、增加尖啸体的警告等级等。
         *
         * @param world 服务端世界
         * @param pos 振动源位置
         * @param event 到达的游戏事件
         * @param sourceEntity 触发事件的实体（可为 nullptr）
         * @param distance 振动传播距离
         */
        virtual void onReceiveVibration(server::ServerWorld& world,
            const BlockPos& pos,
            const GameEvent& event,
            const Entity* sourceEntity,
            f32 distance) = 0;

        /**
         * @brief 计算振动传播时间（tick）
         *
         * 默认实现为 floor(distance)，即每格1 tick。
         *
         * @param distance 振动传播距离
         * @return 传播时间（tick）
         */
        [[nodiscard]] virtual i32 calculateTravelTimeInTicks(f32 distance) const;

        /**
         * @brief 检查振动事件是否有效
         *
         * 基本检查：事件频率是否为0、源实体是否在潜行、
         * 方块是否阻尼振动等。
         *
         * @param event 游戏事件
         * @param context 事件上下文
         * @return 如果振动有效返回 true
         */
        [[nodiscard]] virtual bool isValidVibration(const GameEvent& event, const GameEvent::Context& context) const;

        /**
         * @brief 是否需要相邻区块正在 tick
         *
         * 幽匿感测体需要此条件为 true，以确保振动到达时
         * 附近的区块已加载且在 tick。
         */
        [[nodiscard]] virtual bool requiresAdjacentChunksToBeTicking() const { return false; }

        /**
         * @brief 是否可以触发规避振动成就
         */
        [[nodiscard]] virtual bool canTriggerAvoidVibration() const { return false; }

        /**
         * @brief 是否为幽匿尖啸体（而非感测体）
         *
         * 用于 SculkVibrationManager::tickAll() 中区分感测体和尖啸体，
         * 以便仅对尖啸体检查 SHRIEKING 结束标志。
         * 默认返回 false（感测体），尖啸体覆写为 true。
         */
        [[nodiscard]] virtual bool isSculkShrieker() const { return false; }

        /**
         * @brief 数据变化回调
         *
         * 当振动系统数据发生变化时调用（如选择了新振动、振动到达等），
         * 用于标记方块实体需要保存。
         */
        virtual void onDataChanged() {}
    };

    // ========================================================================
    // Listener: 实现 GameEventListener 接口
    // ========================================================================

    /**
     * @brief 振动监听器
     *
     * 实现 GameEventListener 接口，作为 VibrationSystem 的监听器组件。
     * 当游戏事件到达时，验证事件有效性并添加到 VibrationSelector。
     *
     */
    class Listener final : public GameEventListener {
    public:
        explicit Listener(VibrationSystem& system)
            : m_system(system)
        {}

        [[nodiscard]] PositionSource& getListenerSource() override
        {
            return m_system.getVibrationUser().getPositionSource();
        }
        [[nodiscard]] const PositionSource& getListenerSource() const override
        {
            return m_system.getVibrationUser().getPositionSource();
        }

        [[nodiscard]] i32 getListenerRadius() const override { return m_system.getVibrationUser().getListenerRadius(); }

        bool handleGameEvent(server::ServerWorld& world,
            const GameEvent& event,
            const GameEvent::Context& context,
            const Vector3d& pos) override;

        [[nodiscard]] DeliveryMode getDeliveryMode() const override { return DeliveryMode::ByDistance; }

        /**
         * @brief 强制调度振动（跳过验证检查）
         */
        void forceScheduleVibration(
            server::ServerWorld& world, const GameEvent& event, const GameEvent::Context& context, const Vector3d& pos);

    private:
        /**
         * @brief 调度振动到选择器
         */
        void scheduleVibration(server::ServerWorld& world,
            const GameEvent& event,
            const GameEvent::Context& context,
            const Vector3d& pos,
            const Vector3d& listenerPos);

        VibrationSystem& m_system;
    };

    // ========================================================================
    // Ticker: 振动传播 tick 逻辑
    // ========================================================================

    /**
     * @brief 振动系统 tick 驱动器
     *
     * 每 tick 调用一次，驱动振动传播：
     * 1. 如果没有当前振动，尝试从选择器选择候选
     * 2. 递减传播时间
     * 3. 传播时间归零时，触发 onReceiveVibration 回调
     *
     */
    class Ticker {
    public:
        /**
         * @brief 每 tick 调用
         * @param world 服务端世界
         * @param data 振动数据
         * @param user 振动接收者配置
         */
        static void tick(server::ServerWorld& world, Data& data, User& user);

    private:
        /**
         * @brief 尝试从选择器中选择并调度振动
         */
        static void trySelectAndScheduleVibration(server::ServerWorld& world, Data& data, User& user);

        /**
         * @brief 区块重新加载后重发振动粒子
         *
         * 当区块重新加载且存在正在传播的振动时，根据当前传播进度
         * 计算粒子插值位置并重新发送粒子效果。
         */
        static void tryReloadVibrationParticle(server::ServerWorld& world, Data& data, User& user);

        /**
         * @brief 接收振动（传播完成回调）
         */
        [[nodiscard]] static bool receiveVibration(
            server::ServerWorld& world, Data& data, User& user, const VibrationInfo& info);
    };

    // ========================================================================
    // VibrationSystem 主接口
    // ========================================================================

    VibrationSystem() = default;
    virtual ~VibrationSystem() = default;

    /**
     * @brief 获取振动数据
     */
    [[nodiscard]] virtual Data& getVibrationData() = 0;
    [[nodiscard]] virtual const Data& getVibrationData() const = 0;

    /**
     * @brief 获取振动用户配置
     */
    [[nodiscard]] virtual User& getVibrationUser() = 0;
    [[nodiscard]] virtual const User& getVibrationUser() const = 0;

    /**
     * @brief 获取振动监听器
     */
    [[nodiscard]] Listener& getVibrationListener() { return m_listener; }
    [[nodiscard]] const Listener& getVibrationListener() const { return m_listener; }

private:
    Listener m_listener{*this};
};

} // namespace gameevent

} // namespace mc
