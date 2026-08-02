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

#include "ServerDimension.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace mc {

// 前向声明
namespace server {
class MinecraftServer;
}
namespace world::gen {
class RandomState;
namespace settings {
struct WorldPresetGenerator;
} // namespace settings
} // namespace world::gen
namespace world::biome {
class IBiomeSource;
} // namespace world::biome

/**
 * @brief 服务端维度管理器
 *
 * 管理 ServerDimension 实例，处理玩家维度切换，
 * 协调多维度 tick 更新。
 */
class ServerDimensionManager : public DimensionManager {
public:
    /**
     * @brief 维度切换回调类型
     *
     * @param playerId 玩家ID
     * @param fromDimension 源维度
     * @param toDimension 目标维度
     * @param position 目标位置
     */
    using DimensionChangeCallback = std::function<void(PlayerId, DimensionId, DimensionId, const Vector3d&)>;

    /**
     * @brief 构造函数
     *
     * @param server 服务器实例
     */
    explicit ServerDimensionManager(server::MinecraftServer* server);

    /**
     * @brief 析构函数
     */
    ~ServerDimensionManager() override;

    // 禁止拷贝
    ServerDimensionManager(const ServerDimensionManager&) = delete;
    ServerDimensionManager& operator=(const ServerDimensionManager&) = delete;

    // ========== 初始化 ==========

    /**
     * @brief 初始化维度管理器
     *
     * 创建所有原版维度，初始化区块管理器等。
     *
     * @param seed 世界种子
     * @param viewDistance 视野距离
     * @param simulationDistance 模拟距离（控制实体激活范围）
     * @param overworldType 主世界类型（旧 WorldType 路径，仅 WorldPresetRegistry 未加载时兜底）
     * @param worldPresetId 世界预设资源位置（如 "minecraft:normal"），数据驱动装配查 WorldPresetRegistry
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(u64 seed,
        i32 viewDistance,
        i32 simulationDistance,
        WorldType overworldType,
        resource::ResourceLocation worldPresetId);

    /**
     * @brief 关闭维度管理器
     *
     * 清理所有维度。
     */
    void shutdown();

    // ========== 维度访问 ==========

    /**
     * @brief 获取服务端维度
     *
     * @param id 维度ID
     * @return 服务端维度指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] ServerDimension* getDimension(DimensionId id);
    [[nodiscard]] const ServerDimension* getDimension(DimensionId id) const;

    /**
     * @brief 获取主世界维度
     */
    [[nodiscard]] ServerDimension* getOverworld();
    [[nodiscard]] const ServerDimension* getOverworld() const;

    /**
     * @brief 获取下界维度
     */
    [[nodiscard]] ServerDimension* getNether();
    [[nodiscard]] const ServerDimension* getNether() const;

    /**
     * @brief 获取末地维度
     */
    [[nodiscard]] ServerDimension* getTheEnd();
    [[nodiscard]] const ServerDimension* getTheEnd() const;

    // ========== 玩家维度管理 ==========

    /**
     * @brief 玩家加入维度
     *
     * @param playerId 玩家ID
     * @param dimId 维度ID
     */
    void playerJoinDimension(PlayerId playerId, DimensionId dimId);

    /**
     * @brief 玩家离开维度
     *
     * @param playerId 玩家ID
     */
    void playerLeaveDimension(PlayerId playerId);

    /**
     * @brief 获取玩家当前所在维度
     *
     * @param playerId 玩家ID
     * @return 维度ID，如果玩家不在任何维度则返回 -1
     */
    [[nodiscard]] DimensionId getPlayerDimension(PlayerId playerId) const;

    /**
     * @brief 获取玩家所在维度的实例
     *
     * @param playerId 玩家ID
     * @return 维度实例，如果玩家不在任何维度则返回 nullptr
     */
    [[nodiscard]] ServerDimension* getPlayerDimensionWorld(PlayerId playerId);

    // ========== 维度切换 ==========

    /**
     * @brief 将玩家传送到另一个维度
     *
     * @param playerId 玩家ID
     * @param targetDim 目标维度ID
     * @param position 目标位置（如果为空，则使用维度的出生点）
     * @return 是否成功
     */
    [[nodiscard]] bool transferPlayerToDimension(
        PlayerId playerId, DimensionId targetDim, const std::optional<Vector3d>& position = std::nullopt);

    /**
     * @brief 设置维度切换回调
     *
     * 当玩家成功切换维度后调用。
     */
    void setDimensionChangeCallback(DimensionChangeCallback callback)
    {
        // 不允许多次注册
        if (m_dimensionChangeCallback) {
            throw std::invalid_argument("Dimension change callback already set");
        }
        m_dimensionChangeCallback = std::move(callback);
    }

    // ========== 更新 ==========

    /**
     * @brief 更新所有维度
     *
     * 遍历所有维度并调用 tick()。
     */
    void tick();

    // ========== 配置 ==========

    /**
     * @brief 获取视野距离
     */
    [[nodiscard]] i32 viewDistance() const { return m_viewDistance; }

    /**
     * @brief 获取模拟距离（控制实体激活范围）
     */
    [[nodiscard]] i32 simulationDistance() const { return m_simulationDistance; }

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] u64 seed() const { return m_seed; }

protected:
    server::MinecraftServer* m_server;

private:
    // 玩家 -> 维度映射
    std::unordered_map<PlayerId, DimensionId> m_playerDimensions;

    // 维度 -> 玩家集合映射
    std::unordered_map<DimensionId, std::unordered_set<PlayerId>> m_dimensionPlayers;

    // 配置
    u64 m_seed = 0;
    i32 m_viewDistance = 10;
    i32 m_simulationDistance = 10;
    WorldType m_overworldType = WorldType::Default;
    resource::ResourceLocation m_worldPresetId{"minecraft", "normal"};

    // 回调
    DimensionChangeCallback m_dimensionChangeCallback;

    // 初始化标志
    bool m_initialized = false;

    /**
     * @brief 创建服务端维度实例
     *
     * @param id 维度ID
     * @param seed 世界种子
     * @return 维度实例
     */
    [[nodiscard]] std::unique_ptr<ServerDimension> _createServerDimension(DimensionId id, u64 seed);

    /**
     * @brief 按 world_preset 的 biome_source 配置创建生物群系源
     *
     * @param gen world_preset 生成器配置（含 biomeSourceType + multiNoisePreset/fixedBiome）
     * @param rs 共享的 RandomState（multi_noise/the_end 取噪声采样）
     * @param seed 世界种子（fixed 生物群系源构造用）
     * @return 生物群系源
     */
    [[nodiscard]] std::unique_ptr<world::biome::IBiomeSource> _createBiomeSource(
        const world::gen::settings::WorldPresetGenerator& gen, const world::gen::RandomState& rs, u64 seed);

    /**
     * @brief 旧 WorldType 装配兜底（WorldPresetRegistry 未加载时）
     *
     * @param id 维度ID
     * @param seed 世界种子
     * @return 区块生成器
     */
    [[nodiscard]] std::unique_ptr<IChunkGenerator> _createLegacyGenerator(DimensionId id, u64 seed);

    [[nodiscard]] std::unique_ptr<server::ServerWorld> _createServerWorld(
        DimensionId id, u64 seed, std::unique_ptr<IChunkGenerator> generator) const;

    /**
     * @brief 发送维度切换数据包
     *
     * @param playerId 玩家ID
     * @param newDim 新维度ID
     * @param pos 目标位置
     */
    void _sendDimensionChangePacket(PlayerId playerId, DimensionId newDim, const Vector3d& pos);

    /**
     * @brief 卸载玩家当前维度的区块
     *
     * @param playerId 玩家ID
     */
    void _unloadPlayerChunks(PlayerId playerId);

    /**
     * @brief 加载新维度的区块给玩家
     *
     * @param playerId 玩家ID
     * @param dim 目标维度
     */
    void _loadPlayerChunks(PlayerId playerId, ServerDimension* dim);
};

} // namespace mc
