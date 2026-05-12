#pragma once

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/player/SleepResult.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/network/packet/ExperiencePackets.hpp"
#include "server/stats/StatisticsManager.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc {

namespace server {
class ServerWorld;
class IServer;
class PlayerAdvancements;
}

/**
 * @brief 服务端玩家实体。
 *
 * 扩展 Player 类，添加服务端特有的网络同步与在线状态管理能力。
 */
class ServerPlayer : public Player {
public:
    /**
     * @brief 构造服务端玩家。
     * @param id 实体ID。
     * @param name 玩家名称。
     */
    ServerPlayer(EntityId id, const std::string& name);
    ~ServerPlayer() override = default;

    // ========== 网络相关 ==========

    /**
     * @brief 发送聊天消息给玩家。
     * @param message 聊天内容。
     */
    void sendChatMessage(const std::string& message);

    /**
     * @brief 发送系统消息给玩家。
     * @param message 系统消息内容。
     */
    void sendSystemMessage(const std::string& message);

    /**
     * @brief 发送状态消息给玩家（重写 Player 基类）。
     *
     * 通过网络发送消息到客户端。如果 actionBar 为 true，
     * 消息会显示在物品栏上方的 Action Bar 区域。
     *
     * @param message 消息内容（翻译键或格式化文本）
     * @param actionBar 是否显示在 Action Bar 区域
     */
    void sendStatusMessage(const std::string& message, bool actionBar = false) override;

    /**
     * @brief 检查玩家是否能接收消息（重写 Player 基类）。
     * @return 如果有有效网络连接返回 true
     */
    [[nodiscard]] bool canReceiveMessages() const override { return hasConnection(); }

    /**
     * @brief 同步经验状态到客户端。
     * @note 仅在连接可用时发送网络包。
     */
    void syncExperience();

    // ========== 重写经验方法 ==========

    /**
     * @brief 添加经验并同步到客户端。
     * @param amount 增加的经验值。
     */
    void addExperience(i32 amount) override;

    /**
     * @brief 设置经验等级并同步到客户端。
     * @param level 目标等级。
     */
    void setExperienceLevel(i32 level) override;

    /**
     * @brief 添加经验等级并同步到客户端。
     * @param levels 要添加的等级数（可以为负数）。
     */
    void addExperienceLevels(i32 levels) override;

    /**
     * @brief 消耗经验值并同步到客户端。
     * @param amount 要消耗的经验值。
     * @return 是否成功消耗。
     */
    [[nodiscard]] bool consumeExperience(i32 amount) override;

    /**
     * @brief 消耗经验等级并同步到客户端。
     * @param levels 要消耗的等级数。
     * @return 是否成功消耗。
     */
    [[nodiscard]] bool consumeExperienceLevels(i32 levels) override;

    /**
     * @brief 设置完整经验状态并同步到客户端。
     * @param level 等级
     * @param progress 进度 (0.0-1.0)
     * @param totalExperience 总经验值
     */
    void setExperience(i32 level, f32 progress, i32 totalExperience) override;

    /**
     * @brief 绑定网络连接。
     * @param connection 玩家连接（可为 nullptr）。
     */
    void setConnection(network::ConnectionPtr connection) { m_connection = std::move(connection); }

    /**
     * @brief 获取网络连接。
     * @return 网络连接共享指针的常量引用。
     * @note 返回值可能为 nullptr，调用方需结合 hasConnection() 使用。
     */
    [[nodiscard]] const network::ConnectionPtr& connection() const { return m_connection; }

    /**
     * @brief 检查网络连接是否可用。
     * @return true 表示连接存在且仍处于连接状态。
     */
    [[nodiscard]] bool hasConnection() const { return m_connection && m_connection->isConnected(); }

    // ========== 世界相关 ==========

    /**
     * @brief 设置所在世界。
     * @param world 世界指针。
     */
    void setWorld(server::ServerWorld* world) { m_world = world; }

    /**
     * @brief 获取所在世界。
     * @return 当前所在世界指针。
     */
    [[nodiscard]] server::ServerWorld* getWorld() const { return m_world; }

    /**
     * @brief 设置服务器引用。
     * @param server 服务器接口指针。
     */
    void setServer(server::IServer* server) { m_server = server; }

    /**
     * @brief 获取服务器引用。
     * @return 服务器接口指针。
     */
    [[nodiscard]] server::IServer* getServer() const { return m_server; }

    // ========== 类型转换 ==========

    /**
     * @brief 转换为 ServerPlayer 指针（重写 Player 基类）
     * @return 返回 this 指针
     */
    [[nodiscard]] ServerPlayer* asServerPlayer() override { return this; }
    [[nodiscard]] const ServerPlayer* asServerPlayer() const override { return this; }

    // ========== 成就系统 ==========

    /**
     * @brief 获取玩家成就进度管理器
     * @return 成就进度管理器指针
     */
    [[nodiscard]] server::PlayerAdvancements* getAdvancements() { return m_advancements.get(); }
    [[nodiscard]] const server::PlayerAdvancements* getAdvancements() const { return m_advancements.get(); }

    /**
     * @brief 初始化成就系统
     */
    void initAdvancements();

    // ========== 统计系统 ==========

    /**
     * @brief 获取玩家统计管理器
     * @return 统计管理器引用
     */
    [[nodiscard]] server::stats::StatisticsManager& getStats() { return m_statistics; }
    [[nodiscard]] const server::stats::StatisticsManager& getStats() const { return m_statistics; }

    /**
     * @brief 增加物品合成统计（重写 Player 基类）
     * @param itemId 物品资源位置
     * @param count 合成数量
     */
    void awardCraftedStat(const ResourceLocation& itemId, i32 count) override;

    /**
     * @brief 物品合成完成时调用（重写 Player 基类）
     * @param stack 合成的物品堆
     * @param amount 合成数量
     */
    void onItemCrafted(ItemStack& stack, i32 amount) override;

    /**
     * @brief 解锁配方（重写 Player 基类）
     *
     * 触发 RecipeUnlockedTrigger 成就。
     *
     * @param recipeId 配方资源位置
     */
    void unlockRecipe(const ResourceLocation& recipeId) override;

    // ========== 连接状态 ==========

    /**
     * @brief 检查玩家是否在线。
     * @return true 表示在线。
     */
    [[nodiscard]] bool isOnline() const { return m_online; }

    /**
     * @brief 设置在线状态。
     * @param online 新的在线状态。
     */
    void setOnline(bool online) { m_online = online; }

    // ========== 睡眠系统 ==========

    /**
     * @brief 尝试在指定位置睡眠
     *
     * 执行完整的睡眠检查流程：
     * 1. 检查是否已经在睡眠
     * 2. 检查维度是否允许睡眠
     * 3. 检查距离床是否太远
     * 4. 检查床是否被阻挡
     * 5. 设置重生点
     * 6. 检查时间是否允许睡眠
     * 7. 非创造模式检查周围怪物
     *
     * @param bedPos 床头位置
     * @return 睡眠结果
     */
    entity::SleepResult trySleep(const BlockPos& bedPos);

    /**
     * @brief 停止睡眠
     *
     * @param resetTimer 是否重置睡眠计时器（true=立即重置为0，false=设置为100继续渐变）
     * @param updateSleepingFlag 是否更新世界睡眠标志
     */
    void stopSleepInBed(bool resetTimer, bool updateSleepingFlag);

    /**
     * @brief 唤醒玩家（完全唤醒）
     *
     * 相当于 stopSleepInBed(true, true)
     */
    void wakeUp();

    // ========== 重生系统 ==========

    /**
     * @brief 确定重生位置
     *
     * 按以下顺序确定：
     * 1. 玩家个人重生点（床/重生锚设置）
     * 2. 世界出生点
     * 3. 默认位置 (0, 64, 0)
     *
     * @return 重生位置（世界坐标）
     */
    [[nodiscard]] Vector3d determineRespawnPosition() const;

    /**
     * @brief 确定重生维度
     *
     * @return 重生维度ID
     */
    [[nodiscard]] DimensionId determineRespawnDimension() const;

    // ========== 维度传送 ==========

    /**
     * @brief 当传送门触发时调用
     *
     * 实现 ServerPlayer 的维度切换逻辑。
     * 参考 MC 1.16.5 ServerPlayerEntity.tickPortal()
     *
     * @return true 如果传送成功
     */
    bool onPortalTriggered() override;

    /**
     * @brief 传送到另一个维度
     *
     * @param targetDim 目标维度ID
     * @return true 如果传送成功
     */
    [[nodiscard]] bool changeDimension(DimensionId targetDim);

private:
    /**
     * @brief 发送睡眠包给客户端
     * @param bedPos 床位位置
     */
    void sendSleepPacket(const BlockPos& bedPos);

    /**
     * @brief 发送唤醒包给客户端
     */
    void sendWakeUpPacket();

    /**
     * @brief 发送完整封包到当前玩家连接。
     * @param packet 已包含协议头的完整封包字节流。
     * @return true 表示已成功投递到底层连接。
     * @note 当玩家连接不存在或已断开时返回 false，不抛出异常。
     */
    [[nodiscard]] bool sendFullPacket(const std::vector<u8>& packet) const;

private:
    network::ConnectionPtr m_connection;
    server::ServerWorld* m_world = nullptr;
    server::IServer* m_server = nullptr;
    std::shared_ptr<server::PlayerAdvancements> m_advancements;
    server::stats::StatisticsManager m_statistics;
    bool m_online = true;
};

} // namespace mc