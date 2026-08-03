/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so so, subject to the following conditions:
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
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>
#include <optional>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class Entity;

namespace world::chunk {
class ChunkData;
}

namespace blockentity {

/**
 * @brief 末地折跃门方块实体
 *
 * 末地折跃门是一种在末地维度中快速传送的方块。
 * 主要功能：
 * - 将实体从主岛传送到外岛（或返回）
 * - 传送冷却机制
 * - 自动寻找或生成出口传送门
 *
 * 传送机制（MC 1.16.5）：
 * - 实体进入折跃门后立即被传送
 * - 传送冷却 100 tick（触发后）
 * - 如果没有出口位置，会自动在约 1024 格外生成新传送门
 * - 每 2400 tick 自动触发冷却（用于外岛返回的传送门刷新）
 *
 * 参考: net.minecraft.tileentity.EndGatewayTileEntity
 */
class EndGatewayEntity : public BlockEntity {
public:
    /// 传送冷却时间（tick）
    static constexpr i32 TELEPORT_COOLDOWN = 100;

    /// 自动冷却周期（tick）- 每 2400 tick 自动触发冷却
    static constexpr i64 AUTO_COOLDOWN_INTERVAL = 2400L;

    /// 生成动画持续时间（tick）
    static constexpr i64 SPAWN_DURATION = 200L;

    /// 传送后冷却时间（tick）- 触发后设置的冷却
    static constexpr i32 TRIGGER_COOLDOWN = 40;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit EndGatewayEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~EndGatewayEntity() override = default;

    // ========== 方块实体接口 ==========

    /**
     * @brief 每 tick 更新
     * @param world 所在世界
     *
     * 检查方块内的实体并执行传送逻辑。
     */
    void tick(IWorld& world) override;

    /**
     * @brief 检查是否需要 tick
     * @return 末地折跃门需要持续 tick
     */
    [[nodiscard]] bool needsTick() const noexcept override { return true; }

    /**
     * @brief 从 JSON 加载数据
     * @param data JSON 数据
     * @return 是否成功
     */
    bool load(const nlohmann::json& data) override;

    /**
     * @brief 保存数据到 JSON
     * @param data 输出 JSON 数据
     */
    void save(nlohmann::json& data) const override;

    /**
     * @brief 创建副本
     * @return 副本的 unique_ptr
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 传送功能 ==========

    /**
     * @brief 传送实体
     * @param world 世界
     * @param entity 要传送的实体
     *
     * 执行传送逻辑，包括：
     * - 检查冷却状态
     * - 获取或生成出口位置
     * - 执行传送
     * - 触发冷却
     */
    void teleportEntity(IWorld& world, Entity& entity);

    /**
     * @brief 设置出口传送门位置
     * @param exitPos 出口位置
     * @param exactTeleport 是否精确传送
     */
    void setExitPortal(const BlockPos& exitPos, bool exactTeleport = false);

    /**
     * @brief 获取出口传送门位置
     * @return 出口位置（如果已设置）
     */
    [[nodiscard]] std::optional<BlockPos> getExitPortal() const noexcept { return m_exitPortal; }

    /**
     * @brief 检查是否精确传送
     * @return 如果精确传送返回 true
     */
    [[nodiscard]] bool isExactTeleport() const noexcept { return m_exactTeleport; }

    // ========== 状态查询 ==========

    /**
     * @brief 检查是否正在生成（新生成的折跃门）
     * @return 年龄小于 200 tick 返回 true
     */
    [[nodiscard]] bool isSpawning() const noexcept { return m_age < SPAWN_DURATION; }

    /**
     * @brief 检查是否在冷却中
     * @return 冷却时间大于 0 返回 true
     */
    [[nodiscard]] bool isCoolingDown() const noexcept { return m_teleportCooldown > 0; }

    /**
     * @brief 获取年龄
     * @return 年龄（tick）
     */
    [[nodiscard]] i64 getAge() const noexcept { return m_age; }

    /**
     * @brief 获取传送冷却
     * @return 剩余冷却时间（tick）
     */
    [[nodiscard]] i32 getTeleportCooldown() const noexcept { return m_teleportCooldown; }

    /**
     * @brief 获取生成进度（用于客户端动画）
     * @param partialTicks 部分 tick
     * @return 进度值 0.0-1.0
     */
    [[nodiscard]] f32 getSpawnPercent(f32 partialTicks = 0.0f) const;

    /**
     * @brief 获取冷却进度（用于客户端动画）
     * @param partialTicks 部分 tick
     * @return 进度值 0.0-1.0（1.0 = 冷却完成）
     */
    [[nodiscard]] f32 getCooldownPercent(f32 partialTicks = 0.0f) const;

    /**
     * @brief 触发冷却
     * @param world 世界
     *
     * 设置传送冷却并通知客户端。
     */
    void triggerCooldown(IWorld& world);

    // ========== 结构生成 ==========

    /**
     * @brief 在指定位置创建折跃门结构
     * @param world 世界
     * @param pos 基础位置（折跃门方块所在位置）
     *
     * 生成 3x5x3 的基岩十字框架结构，中心为末地折跃门方块。
     * 此方法不依赖实例状态，可独立调用。
     */
    static void createGatewayStructure(IWorld& world, const BlockPos& pos);

    // ========== 方块事件 ==========

    /**
     * @brief 处理方块事件
     * @param id 事件 ID
     * @param type 事件类型
     * @return 是否处理成功
     */
    [[nodiscard]] bool triggerEvent(i32 id, i32 type) override;

private:
    /**
     * @brief 寻找出口位置
     * @param world 世界
     * @return 出口方块位置
     *
     * 在出口传送门附近寻找安全的传送位置。
     */
    [[nodiscard]] BlockPos _findExitPosition(IWorld& world) const;

    /**
     * @brief 生成出口传送门（末地主岛折跃门专用）
     * @param world 服务端世界
     *
     * 在外岛生成新的折跃门结构。
     */
    void _generateExitPortal(IWorld& world);

    /**
     * @brief 查找最高方块
     * @param world 世界
     * @param center 中心位置
     * @param radius 搜索半径
     * @param allowBedrock 是否允许基岩
     * @return 最高方块位置
     */
    [[nodiscard]] static BlockPos _findHighestBlock(
        IWorld& world, const BlockPos& center, i32 radius, bool allowBedrock);

    /**
     * @brief 检查区块是否为空（所有区段均无方块）
     *
     * 对应 MC Java 的 TheEndGatewayBlockEntity.isChunkEmpty：
     * 判断区块中是否存在非空区段（getHighestFilledSectionIndex == -1 表示空区块）。
     *
     * @param chunk 区块数据，nullptr 视为空区块
     * @return 区块为空返回 true，否则 false
     */
    [[nodiscard]] static bool _isChunkEmpty(const world::chunk::ChunkData* chunk);

    /// 年龄（tick）- 用于生成动画
    i64 m_age = 0;

    /// 传送冷却（tick）- 传送后的冷却时间
    i32 m_teleportCooldown = 0;

    /// 出口传送门位置（可选）
    std::optional<BlockPos> m_exitPortal;

    /// 是否精确传送（传送到精确位置而非附近安全位置）
    bool m_exactTeleport = false;

    // 测试子类需要访问私有方法
    friend class TestEndGatewayEntity;
};

} // namespace blockentity
} // namespace mc
