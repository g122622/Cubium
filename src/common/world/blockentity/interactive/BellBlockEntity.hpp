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

#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class LivingEntity;

/// 方向枚举前向声明（mc::Direction 是 enum class）
enum class Direction : u8;

namespace blockentity {

/**
 * @brief 钟方块实体
 *
 * 钟方块实体维护钟的摇晃动画与共振机制：
 * - 玩家右键/投射物击中钟时，调用 onHit(direction) 触发摇晃动画
 * - 摇晃过程中若检测到附近 32 格内有灾厄村民（RAIDERS 标签），进入共振状态
 * - 共振持续 40 tick 后，对附近 48 格内的灾厄村民施加发光效果（60 tick）
 * - 同时给附近 32 格内的村民写入 HEARD_BELL_TIME 记忆，触发"躲藏"行为
 *
 * 关键常量（与 MC 1.21.11 BellBlockEntity.java 对齐）：
 * - DURATION = 50：摇晃动画持续 tick
 * - GLOW_DURATION = 60：发光效果持续 tick
 * - MIN_TICKS_BETWEEN_SEARCHES = 60：两次实体搜索最小间隔
 * - MAX_RESONATION_TICKS = 40：共振持续 tick
 * - TICKS_BEFORE_RESONATION = 5：摇晃开始后多久开始检测共振
 * - SEARCH_RADIUS = 48：实体搜索半径
 * - HEAR_BELL_RADIUS = 32：听到钟声的半径
 * - HIGHLIGHT_RAIDERS_RADIUS = 48：发光效果施加半径
 *
 * 参考: net.minecraft.world.level.block.entity.BellBlockEntity
 */
class BellBlockEntity : public BlockEntity {
public:
    // ========== 动画常量 ==========

    /// 摇晃动画持续 tick（与 MC 1.21.11 DURATION 对齐）
    static constexpr i32 DURATION = 50;

    /// 发光效果持续 tick（与 MC 1.21.11 GLOW_DURATION 对齐）
    static constexpr i32 GLOW_DURATION = 60;

    /// 两次实体搜索最小间隔 tick（与 MC 1.21.11 MIN_TICKS_BETWEEN_SEARCHES 对齐）
    static constexpr i32 MIN_TICKS_BETWEEN_SEARCHES = 60;

    /// 共振持续 tick（与 MC 1.21.11 MAX_RESONATION_TICKS 对齐）
    static constexpr i32 MAX_RESONATION_TICKS = 40;

    /// 摇晃开始后多久开始检测共振（与 MC 1.21.11 TICKS_BEFORE_RESONATION 对齐）
    static constexpr i32 TICKS_BEFORE_RESONATION = 5;

    /// 实体搜索半径（与 MC 1.21.11 SEARCH_RADIUS 对齐）
    static constexpr f32 SEARCH_RADIUS = 48.0f;

    /// 听到钟声的半径（与 MC 1.21.11 HEAR_BELL_RADIUS 对齐）
    static constexpr f32 HEAR_BELL_RADIUS = 32.0f;

    /// 发光效果施加半径（与 MC 1.21.11 HIGHLIGHT_RAIDERS_RADIUS 对齐）
    static constexpr f32 HIGHLIGHT_RAIDERS_RADIUS = 48.0f;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit BellBlockEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~BellBlockEntity() noexcept override;

    // ========== 钟敲击接口 ==========

    /**
     * @brief 敲击钟时调用
     *
     * 设置敲击方向，启动摇晃动画，并通过 blockEvent 将动画同步到客户端。
     * 若钟已经在摇晃，则重置 ticks 计数。
     *
     * 参考: net.minecraft.world.level.block.entity.BellBlockEntity#onHit
     *
     * @param world 世界引用
     * @param direction 敲击方向（玩家点击面或投射物击中面）
     */
    void onHit(IWorld& world, Direction direction);

    // ========== 状态查询 ==========

    /**
     * @brief 是否正在摇晃
     */
    [[nodiscard]] bool isShaking() const noexcept { return m_shaking; }

    /**
     * @brief 获取摇晃已持续的 tick 数
     */
    [[nodiscard]] i32 ticks() const noexcept { return m_ticks; }

    /**
     * @brief 获取敲击方向
     */
    [[nodiscard]] Direction clickDirection() const noexcept { return m_clickDirection; }

    /**
     * @brief 是否正在共振
     */
    [[nodiscard]] bool isResonating() const noexcept { return m_resonating; }

    /**
     * @brief 获取共振已持续的 tick 数
     */
    [[nodiscard]] i32 resonationTicks() const noexcept { return m_resonationTicks; }

    // ========== Tick 更新 ==========

    /**
     * @brief 每 tick 更新钟的状态
     *
     * 服务端：处理摇晃计时、共振触发、共振到期后对灾厄村民施加发光效果、
     *        对村民写入 HEARD_BELL_TIME 记忆。
     * 客户端：处理摇晃计时与共振触发的 BELL_RESONATE 音效、共振到期后发射粒子。
     *
     * 由于本实现将服务端与客户端逻辑合并（项目尚无 BlockEntityTicker 分离机制），
     * tick() 同时承担 serverTick 与 clientTick 的职责。
     *
     * 参考: net.minecraft.world.level.block.entity.BellBlockEntity#tick
     *       net.minecraft.world.level.block.entity.BellBlockEntity#serverTick
     *       net.minecraft.world.level.block.entity.BellBlockEntity#clientTick
     *
     * @param world 所在世界
     */
    void tick(IWorld& world) override;

    /**
     * @brief 钟方块实体只在摇晃或共振时需要 tick
     *
     * @return 如果正在摇晃或共振返回 true
     */
    [[nodiscard]] bool needsTick() const noexcept override;

    // ========== 客户端方块事件 ==========

    /**
     * @brief 处理客户端方块事件
     *
     * 当服务端调用 IWorld::blockEvent(pos, block, 1, direction.get3DDataValue()) 时，
     * 事件在服务端执行后广播到客户端，客户端收到 BlockEventPacket 后调用此方法。
     *
     * 事件 ID = 1：启动摇晃动画
     * - 重新搜索附近实体（updateEntities）
     * - 重置共振 tick
     * - 设置敲击方向
     * - 重置 ticks 计数
     * - 启动摇晃
     *
     * 参考: net.minecraft.world.level.block.entity.BellBlockEntity#triggerEvent
     *
     * @param id 事件 ID（1=敲击动画）
     * @param type 事件数据（Direction 的 3D 数据值）
     * @return 如果事件被成功处理返回 true
     */
    [[nodiscard]] bool triggerEvent(i32 id, i32 type) override;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    // ========== 内部实现 ==========

    /**
     * @brief 重新搜索附近实体
     *
     * 距离上次搜索超过 MIN_TICKS_BETWEEN_SEARCHES tick 时重新搜索。
     * 搜索范围为以钟为中心、半径 SEARCH_RADIUS 的 AABB。
     * 同时（仅服务端）给范围内的存活实体写入 HEARD_BELL_TIME 记忆。
     *
     * 参考: net.minecraft.world.level.block.entity.BellBlockEntity#updateEntities
     *
     * @param world 世界引用
     */
    void _updateEntities(IWorld& world);

    /**
     * @brief 检查附近是否有灾厄村民
     *
     * 在已缓存的附近实体中查找满足以下条件的实体：
     * - 存活
     * - 未被移除
     * - 距离钟中心不超过 HEAR_BELL_RADIUS
     * - 实体类型属于 RAIDERS 标签
     *
     * 参考: net.minecraft.world.level.block.entity.BellBlockEntity#areRaidersNearby
     *
     * @return 如果存在符合条件的灾厄村民返回 true
     */
    [[nodiscard]] bool _areRaidersNearby() const;

    /**
     * @brief 共振到期后对灾厄村民施加发光效果
     *
     * 筛选 m_nearbyEntities 中满足条件的灾厄村民，施加 GLOW_DURATION tick 的发光效果。
     *
     * 参考: net.minecraft.world.level.block.entity.BellBlockEntity#makeRaidersGlow
     *
     * @param world 世界引用
     */
    void _makeRaidersGlow(IWorld& world);

    /**
     * @brief 共振到期后在客户端发射钟粒子
     *
     * 在客户端发射 ENTITY_EFFECT 粒子，颜色随灾厄村民数量变化。
     * 由于本实现将服务端与客户端逻辑合并，仅在世界为客户端时发射粒子。
     *
     * 参考: net.minecraft.world.level.block.entity.BellBlockEntity#showBellParticles
     *
     * @param world 世界引用
     */
    void _showBellParticles(IWorld& world);

    /**
     * @brief 判断实体是否为范围内（HIGHLIGHT_RAIDERS_RADIUS）的灾厄村民
     *
     * 参考: net.minecraft.world.level.block.entity.BellBlockEntity#isRaiderWithinRange
     *
     * @param entity 待判断的实体
     * @return 如果是范围内的灾厄村民返回 true
     */
    [[nodiscard]] bool _isRaiderWithinRange(const LivingEntity& entity) const;

    /**
     * @brief 对单个实体施加发光效果
     *
     * 参考: net.minecraft.world.level.block.entity.BellBlockEntity#glow
     *
     * @param entity 目标实体
     */
    static void _glow(LivingEntity& entity);

    // ========== 成员变量 ==========

    /// 上次敲响的游戏时间（用于实体搜索节流）
    i64 m_lastRingTimestamp = 0;

    /// 摇晃已持续的 tick 数
    i32 m_ticks = 0;

    /// 是否正在摇晃
    bool m_shaking = false;

    /// 敲击方向（默认 North，因为 Direction 枚举的 North=2 是非零值）
    Direction m_clickDirection;

    /// 附近实体缓存（搜索结果）
    std::vector<LivingEntity*> m_nearbyEntities;

    /// 是否正在共振
    bool m_resonating = false;

    /// 共振已持续的 tick 数
    i32 m_resonationTicks = 0;
};

} // namespace blockentity
} // namespace mc
