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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/player/SleepResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/crafting/RecipeBook.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "server/network/ServerNetwork.hpp"
#include "server/stats/StatisticsManager.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc {

namespace server {
class ServerWorld;
class IServer;
class PlayerAdvancements;
} // namespace server

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
    ServerPlayer(EntityInstanceId id, const std::string& name, ecs::EntityRegistry& registry);
    ServerPlayer(ServerPlayer&& other) noexcept = default;
    ServerPlayer& operator=(ServerPlayer&& other) noexcept = default;
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
     * @brief 打开告示牌编辑器（重写 Player 基类）。
     *
     * 向客户端发送 OpenSignEditorPacket，通知其打开告示牌编辑界面。
     *
     * @param pos 告示牌方块位置
     * @param isFrontSide 是否编辑正面
     */
    void openSignEditor(const BlockPos& pos, bool isFrontSide) override;

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
     * @brief 向此玩家客户端发送速度同步包
     *
     * 重写 Player 基类版本，通过网络包将此实体的当前速度
     * 发送给玩家客户端。用于 causeExtraKnockback() 中对 ServerPlayer
     * 目标立即发送速度包，避免 EntityTracker::tick() 重复发送导致
     * 击退速度重复应用。
     *
     * @return true 如果成功发送了速度包，false 如果连接不可用
     */
    [[nodiscard]] bool sendVelocityPacket() override;

    /**
     * @brief 摄像机目标变更通知（重写 Player 基类）
     *
     * 当 setCameraEntityId() 导致摄像机目标实际变化时调用。
     * ServerPlayer 重写以发送 SetCameraPacket 给客户端，
     * 并在旁观新实体时将玩家传送到目标位置。
     *
     * @param oldCameraId 变更前的摄像机目标实体ID
     * @param newCameraId 变更后的摄像机目标实体ID
     */
    void onCameraEntityChanged(
        std::optional<EntityInstanceId> oldCameraId, std::optional<EntityInstanceId> newCameraId) override;

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
     * @param connection 玩家连接（非拥有，可为 nullptr）。
     */
    void setConnection(mc::server::net::ServerClientConnection* connection) { m_connection = connection; }

    /**
     * @brief 获取网络连接。
     * @return 网络连接指针（非拥有，可能为 nullptr）。
     * @note 调用方需结合 hasConnection() 使用。
     */
    [[nodiscard]] mc::server::net::ServerClientConnection* connection() const { return m_connection; }

    /**
     * @brief 检查网络连接是否可用。
     * @return true 表示连接存在且仍处于连接状态。
     */
    [[nodiscard]] bool hasConnection() const { return m_connection != nullptr && m_connection->isConnected(); }

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

    // ========== 队伍系统 ==========

    /**
     * @brief 获取玩家所属队伍（重写 Entity 基类）
     *
     * 通过服务器的记分板系统获取玩家所在队伍。
     *
     * @return 队伍指针，如果玩家不在任何队伍返回 nullptr
     */
    [[nodiscard]] scoreboard::Team* getTeam() override;
    [[nodiscard]] const scoreboard::Team* getTeam() const override;

    // ========== PvP 系统 ==========

    /**
     * @brief 判断本玩家是否可以对目标玩家造成伤害（重写 Player 基类）
     *
     * 在基类队伍友伤检查之上，额外检查 PvP 游戏规则：
     * - 如果 PvP 被禁用（pvp 游戏规则为 false），返回 false
     * - 否则委托给基类检查队伍友伤规则
     *
     * @param target 目标玩家
     * @return 如果可以造成伤害返回 true
     */
    [[nodiscard]] bool canHarmPlayer(const Player& target) const override;

    /**
     * @brief 服务端受伤处理（重写 Player 基类）
     *
     * 在基类创造模式无敌检查之上，额外检查 PvP 保护：
     * - 如果伤害来源是玩家且 canHarmPlayer 返回 false，拒绝伤害
     * - 如果伤害来源是弹射物且其发射者是玩家且 canHarmPlayer 返回 false，拒绝伤害
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 记录受伤方向并广播受伤动画包给追踪者（含受害者自己）
     *
     * ServerPlayer.indicateDamage：设置 hurtDir 后发送
     * ir::play::HurtAnimation（entityId + hurtDir）。
     * 客户端据此驱动 damageTilt（bobHurt）屏幕倾斜。
     */
    void indicateDamage(f64 d0, f64 d1) override;

    /**
     * @brief 玩家死亡处理（重写 Player 基类）
     *
     * 对齐 MC Java 1.21.11 ServerPlayer.die（ServerPlayer.java:879-939）。
     * 玩家死亡需额外处理：
     * - SHOW_DEATH_MESSAGES 游戏规则检查，发送死亡消息
     * - FORGIVE_DEAD_PLAYERS 游戏规则检查，通知中立生物
     * - 死亡统计递增（DEATHS）+ 重置计时统计（TIME_SINCE_DEATH/TIME_SINCE_REST）
     * - 清除火焰 + 重置冰冻
     * - 记录最后死亡位置
     *
     * @param cause 死亡原因
     */
    void die(DamageSource& cause) override;

    /**
     * @brief 移除肩部实体（重写 Player 基类）
     *
     * 对齐 MC Java 1.21.11 ServerPlayer.removeEntitiesOnShoulder（ServerPlayer.java:808-814）：
     * 若 timeEntitySatOnShoulder + 20 < gameTime，则将左右肩实体生成回世界并清空 NBT。
     * 玩家死亡时由 die() 调用，确保肩部鹦鹉在玩家死亡后回到世界。
     */
    void removeEntitiesOnShoulder() override;

    /**
     * @brief 通知附近中立生物玩家已死亡（对齐 vanilla ServerPlayer.tellNeutralMobsThatIDied）
     *
     * 对齐 MC Java 1.21.11 ServerPlayer.tellNeutralMobsThatIDied（ServerPlayer.java:820-826）：
     * 遍历 32×10×32 范围内非旁观者 Mob，对实现 IAngerable 的中立生物调用 playerDied 等价逻辑，
     * 让其原谅本玩家（清除愤怒状态与攻击目标）。
     *
     * 注意：原版 NeutralMob.playerDied 检查 getPersistentAngerTarget().matches(player) 后
     * 调 stopBeingAngry()。Cubium 的 IAngerable 无持久愤怒目标概念，也无 stopBeingAngry，
     * 此处对范围内所有愤怒中立生物统一清除愤怒——这是与原版的已知偏差。
     * TODO: 对齐原版 NeutralMob.playerDied 语义（需 IAngerable 增加 getPersistentAngerTarget/
     *       stopBeingAngry 等方法，或将 NeutralMob 作为独立接口实现）。
     */
    void tellNeutralMobsThatIDied();

    // ========== 肩部实体访问器（对齐 vanilla ServerPlayer.getShoulderEntityLeft/Right） ==========
    /**
     * @brief 获取左肩实体 NBT
     * @return 左肩实体 NBT 的 const 引用
     */
    [[nodiscard]] const nbt::tags::compound_tag& getShoulderEntityLeft() const noexcept { return m_shoulderEntityLeft; }
    /**
     * @brief 获取右肩实体 NBT
     * @return 右肩实体 NBT 的 const 引用
     */
    [[nodiscard]] const nbt::tags::compound_tag& getShoulderEntityRight() const noexcept
    {
        return m_shoulderEntityRight;
    }
    /**
     * @brief 设置左肩实体 NBT
     * @param tag 肩部实体 NBT
     */
    void setShoulderEntityLeft(nbt::tags::compound_tag tag) { m_shoulderEntityLeft = std::move(tag); }
    /**
     * @brief 设置右肩实体 NBT
     * @param tag 肩部实体 NBT
     */
    void setShoulderEntityRight(nbt::tags::compound_tag tag) { m_shoulderEntityRight = std::move(tag); }

    /**
     * @brief 获取配方书
     * @return 配方书引用
     */
    [[nodiscard]] crafting::ServerRecipeBook& getRecipeBook() { return m_recipeBook; }
    [[nodiscard]] const crafting::ServerRecipeBook& getRecipeBook() const { return m_recipeBook; }

    /**
     * @brief 将肩部实体生成回世界（对齐 vanilla ServerPlayer.respawnEntityOnShoulder）
     *
     * 对齐 MC Java 1.21.11 ServerPlayer.respawnEntityOnShoulder（ServerPlayer.java:816-832）：
     * 从 CompoundTag 反序列化实体，设置位置为玩家上方 0.7 格，生成到世界。
     * 若是可驯服实体（TameableEntity），设置主人为本玩家。
     *
     * @param shoulderNbt 肩部实体 NBT，为空时无操作
     */
    void respawnEntityOnShoulder(const nbt::tags::compound_tag& shoulderNbt);

    /**
     * @brief 授予击杀记分（重写 LivingEntity 基类）
     *
     * 对齐 MC Java 1.21.11 ServerPlayer.awardKillScore（ServerPlayer.java:950-967）：
     * 递增 KILL_COUNT_ALL；若被杀者是 Player 则递增 KILL_COUNT_PLAYERS。
     * 基类 LivingEntity::awardKillScore 为空实现（对齐 Entity.awardKillScore）。
     *
     * @param killedEntity 被杀实体
     * @param source 致死伤害来源
     */
    void awardKillScore(Entity& killedEntity, const DamageSource& source) override;

    /**
     * @brief 攻击实体（重写 Player 基类）
     *
     * 旁观者模式下，攻击实体等同于设置旁观目标（调用 setCamera）。
     * 非旁观者模式下，委托给 Player::attack() 执行正常攻击逻辑。
     *
     * @param target 目标实体
     */
    void attack(Entity& target) override;

    // ========== 类型转换 ==========

    /**
     * @brief 转换为 ServerPlayer 指针（重写 Player 基类）
     * @return 返回 this 指针
     */
    [[nodiscard]] ServerPlayer* asServerPlayer() override { return this; }
    [[nodiscard]] const ServerPlayer* asServerPlayer() const override { return this; }

    [[nodiscard]] scoreboard::Scoreboard* getScoreboard() override;
    [[nodiscard]] const scoreboard::Scoreboard* getScoreboard() const override;

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
     * @brief 增加物品使用统计（重写 Player 基类）
     * @param itemId 物品资源位置
     * @param count 使用次数
     */
    void awardUsedStat(const ResourceLocation& itemId, i32 count) override;

    /**
     * @brief 增加物品合成统计（重写 Player 基类）
     * @param itemId 物品资源位置
     * @param count 合成数量
     */
    void awardCraftedStat(const ResourceLocation& itemId, i32 count) override;

    /**
     * @brief 增加自定义统计（重写 Player 基类）
     * @param statId 自定义统计的资源位置（使用 mc::stats 命名空间中的常量）
     * @param count 增量值
     */
    void awardCustomStat(const ResourceLocation& statId, i32 count) override;

    /**
     * @brief 装备损坏回调（重写）
     *
     * 在基类实现（广播破损动画 + 播放音效）基础上，
     * 额外更新玩家的物品损坏统计（minecraft.broken:{item_id}）。
     * 对应 MC 原版 ServerPlayer.onEquippedItemBroken()。
     *
     * @param item 损坏的物品类型
     * @param slot 损坏物品所在的装备槽位
     */
    void onEquippedItemBroken(const Item& item, EquipmentSlot slot) override;

    /**
     * @brief 物品合成完成时调用（重写 Player 基类）
     * @param stack 合成的物品堆
     * @param amount 合成数量
     */
    void onItemCrafted(ItemStack& stack, i32 amount) override;

    /**
     * @brief 解锁配方（重写 Player 基类）
     *
     * 触发 RecipeUnlockedTrigger 成就，并更新配方书。
     *
     * @param recipeId 配方资源位置
     */
    void unlockRecipe(const ResourceLocation& recipeId) override;

    /**
     * @brief 批量解锁配方
     *
     * 解锁配方、标记为新配方、触发成就。
     *
     * @param recipes 配方ID列表
     * @return 成功解锁的配方数量
     */
    size_t unlockRecipes(const std::vector<ResourceLocation>& recipes);

    /**
     * @brief 锁定配方
     *
     * 从配方书中移除配方。
     *
     * @param recipes 配方ID列表
     * @return 成功锁定的配方数量
     */
    size_t lockRecipes(const std::vector<ResourceLocation>& recipes);

    /**
     * @brief 设置物品栏变更回调
     *
     * 在玩家加入服务器后调用，设置物品栏变更时的成就触发回调。
     */
    void setupInventoryCallback();

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
     * @brief 尝试开始睡眠（重写基类虚方法）
     *
     * 调用 trySleep() 进行完整验证。
     *
     * @param bedPos 床头位置
     * @return 睡眠结果
     */
    entity::SleepResult tryStartSleeping(const BlockPos& bedPos) override { return trySleep(bedPos); }

    /**
     * @brief 停止睡眠
     *
     * @param resetTimer 是否重置睡眠计时器（true=立即重置为0，false=设置为100继续渐变）
     *
     * 全员睡眠判定由 MinecraftServer::tick 每帧跨维度聚合轮询，此方法不再触发世界睡眠标志重算。
     */
    void stopSleepInBed(bool resetTimer);

    /**
     * @brief 唤醒玩家（完全唤醒）
     *
     * 相当于 stopSleepInBed(true)
     */
    void wakeUp();

    // ========== 旁观者跟踪系统 ==========

    /**
     * @brief 每 tick 更新（重写 Player 基类）
     *
     * 在 Player::tick() 基础上增加旁观者位置同步逻辑。
     */
    void tick() override;

    /**
     * @brief 累积方块变更 ACK 序列号
     *
     * 对齐 Java ServerGamePacketListenerImpl.ackBlockChangesUpTo(int)（:1418-1424）：
     * 取 max 累积，而非直接赋值。use_item_on / use_item / PlayerAction(Start/Abort/
     * StopDestroy) 收包后调用此方法记录 sequence，由 tick() 末统一发送一个
     * ClientboundBlockChangedAckPacket(maxSequence)。
     *
     * @param sequence 客户端发来的方块预测序列号（<0 视为无效，忽略）
     */
    void recordBlockChangeAck(i32 sequence)
    {
        if (sequence >= 0) {
            m_ackBlockChangesUpTo = std::max(m_ackBlockChangesUpTo, sequence);
        }
    }

    /**
     * @brief 设置旁观目标实体
     *
     * 设置玩家的摄像机跟踪目标。当目标非空时，玩家的视角将跟随目标实体，
     * 玩家位置每 tick 同步到目标实体位置。
     * 传入 nullptr 表示恢复正常视角（摄像机跟踪自身）。
     *
     * @param target 目标实体指针，nullptr 表示恢复自身视角
     * @return true 如果设置成功
     */
    bool setCamera(Entity* target);

    /**
     * @brief 重置旁观目标为自身
     *
     * 停止旁观任何实体，恢复到自身视角。
     * 会在游戏模式切换离开旁观者模式时自动调用。
     */
    void resetCamera();

    /**
     * @brief 每 tick 更新旁观者位置
     *
     * 如果玩家正在旁观某个实体，将玩家位置同步到目标实体位置。
     * 如果目标实体已死亡或移除，自动停止旁观。
     * 如果玩家按住潜行键，自动停止旁观。
     *
     * 应在 ServerPlayer::tick() 或 Player::tick() 中调用。
     */
    void tickSpectator();

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
     *
     * @return true 如果传送成功
     */
    bool onPortalTriggered() override;

    /**
     * @brief 当玩家进入方块碰撞箱时调用
     *
     * 触发 EnterBlockTrigger 成就。
     *
     * @param blockState 方块状态
     */
    void onInsideBlock(const BlockState& blockState) override;

    /**
     * @brief 传送到另一个维度
     *
     * @param targetDim 目标维度ID
     * @return true 如果传送成功
     */
    [[nodiscard]] bool changeDimension(DimensionId targetDim) override;

    /**
     * @brief 跨维度传送到指定维度的指定坐标（带朝向）。
     *
     * 对齐 vanilla `Entity.teleportTo(ServerLevel, x, y, z, ...)`，目标坐标由调用方指定
     * （/tp 命令语义，区别于传送门 changeDimension 的 Teleporter 计算坐标）。
     * ServerPlayer override 复用 changeDimension 的迁移逻辑（迁移 EntityManager + setDimension
     * /setPosition/setWorld + transferPlayerToDimension），但用传入坐标而非 Teleporter。
     *
     * @param targetDim 目标维度ID
     * @param pos 目标坐标（世界绝对坐标）
     * @param rot 目标朝向（yaw/pitch）
     * @return true 如果传送成功
     */
    [[nodiscard]] bool teleportToDimension(DimensionId targetDim, const Vector3d& pos, const Vector2f& rot) override;

    // ========== 反飞行阈值校验（跨 tick 状态） ==========
    // 对齐 Java ServerGamePacketListenerImpl.handleMovePlayer 的 moved-too-quickly /
    // moved-wrongly 双闸。ServerPlayHandler 为无状态门面，跨 tick 基线须落在玩家实体上。

    /**
     * @brief 记录本 tick 已收到的移动包数。
     *
     * 每 tick 末由 ServerPlayer::tick 复位为 knownMovePacketCount（对齐 Java
     * ServerGamePacketListenerImpl.tick 的 receivedMovePacketCount=knownMovePacketCount）。
     */
    void incrementReceivedMovePacketCount() { ++m_receivedMovePacketCount; }
    void syncMovePacketCounters() { m_knownMovePacketCount = m_receivedMovePacketCount; }
    [[nodiscard]] i32 receivedMovePacketCount() const { return m_receivedMovePacketCount; }
    [[nodiscard]] i32 knownMovePacketCount() const { return m_knownMovePacketCount; }

    /**
     * @brief 反飞行基线位置（玩家上一合法坐标）。
     * firstGood 为本 tick 起始基线，lastGood 为最近一次通过校验的坐标。
     * 超限时回弹至 lastGood。首次进入 Play 阶段由 resetAntiFlightBaseline 初始化。
     */
    [[nodiscard]] f64 firstGoodX() const { return m_firstGoodX; }
    [[nodiscard]] f64 firstGoodY() const { return m_firstGoodY; }
    [[nodiscard]] f64 firstGoodZ() const { return m_firstGoodZ; }
    [[nodiscard]] f64 lastGoodX() const { return m_lastGoodX; }
    [[nodiscard]] f64 lastGoodY() const { return m_lastGoodY; }
    [[nodiscard]] f64 lastGoodZ() const { return m_lastGoodZ; }

    /**
     * @brief 初始化反飞行基线为当前坐标（登录/传送后调用）。
     */
    void resetAntiFlightBaseline(f64 x, f64 y, f64 z)
    {
        m_firstGoodX = x;
        m_firstGoodY = y;
        m_firstGoodZ = z;
        m_lastGoodX = x;
        m_lastGoodY = y;
        m_lastGoodZ = z;
        m_receivedMovePacketCount = 0;
        m_knownMovePacketCount = 0;
    }

    /**
     * @brief 推进反飞行基线（移动包通过校验后调用）。
     * firstGood 取本 tick 首包基线语义：每 tick 内多包共用同一 firstGood，
     * 故仅滚动 lastGood；firstGood 由 tick 末滚动。
     */
    void advanceLastGood(f64 x, f64 y, f64 z)
    {
        m_lastGoodX = x;
        m_lastGoodY = y;
        m_lastGoodZ = z;
    }

    /// 反飞行基线是否已初始化（首次进入 Play 阶段置位）。
    [[nodiscard]] bool hasAntiFlightBaselineInited() const { return m_antiFlightBaselineInited; }
    void markAntiFlightBaselineInited() { m_antiFlightBaselineInited = true; }

    // ========== 载具反飞行基线（跨 tick 状态） ==========
    // 对齐 Java ServerGamePacketListenerImpl.handleMoveVehicle 的 vehicleFirstGood/
    // vehicleLastGood。骑乘载具时按载具坐标做 moved-too-quickly/wrongly 校验。

    [[nodiscard]] f64 vehicleFirstGoodX() const { return m_vehicleFirstGoodX; }
    [[nodiscard]] f64 vehicleFirstGoodY() const { return m_vehicleFirstGoodY; }
    [[nodiscard]] f64 vehicleFirstGoodZ() const { return m_vehicleFirstGoodZ; }
    [[nodiscard]] f64 vehicleLastGoodX() const { return m_vehicleLastGoodX; }
    [[nodiscard]] f64 vehicleLastGoodY() const { return m_vehicleLastGoodY; }
    [[nodiscard]] f64 vehicleLastGoodZ() const { return m_vehicleLastGoodZ; }

    void resetVehicleAntiFlightBaseline(f64 x, f64 y, f64 z)
    {
        m_vehicleFirstGoodX = x;
        m_vehicleFirstGoodY = y;
        m_vehicleFirstGoodZ = z;
        m_vehicleLastGoodX = x;
        m_vehicleLastGoodY = y;
        m_vehicleLastGoodZ = z;
    }
    void advanceVehicleLastGood(f64 x, f64 y, f64 z)
    {
        m_vehicleLastGoodX = x;
        m_vehicleLastGoodY = y;
        m_vehicleLastGoodZ = z;
    }
    void rollVehicleFirstGoodToLastGood()
    {
        m_vehicleFirstGoodX = m_vehicleLastGoodX;
        m_vehicleFirstGoodY = m_vehicleLastGoodY;
        m_vehicleFirstGoodZ = m_vehicleLastGoodZ;
    }
    [[nodiscard]] bool hasVehicleAntiFlightInited() const { return m_vehicleAntiFlightInited; }
    void markVehicleAntiFlightInited() { m_vehicleAntiFlightInited = true; }
    void clearVehicleAntiFlightInited() { m_vehicleAntiFlightInited = false; }

    /// 上次反飞行校验的载具实体ID，用于检测载具切换并重置基线。
    [[nodiscard]] EntityInstanceId lastVehicleId() const { return m_lastVehicleId; }
    void setLastVehicleId(EntityInstanceId id) { m_lastVehicleId = id; }

    /**
     * @brief 滚动 firstGood 到 lastGood（tick 末调用）。
     */
    void rollFirstGoodToLastGood()
    {
        m_firstGoodX = m_lastGoodX;
        m_firstGoodY = m_lastGoodY;
        m_firstGoodZ = m_lastGoodZ;
    }

private:
    /**
     * @brief 发送睡眠包给客户端
     * @param bedPos 床位位置
     */
    void _sendSleepPacket(const BlockPos& bedPos);

    /**
     * @brief 发送唤醒包给客户端
     */
    void _sendWakeUpPacket();

    /**
     * @brief 发送 IR 包到当前玩家连接。
     * @param packet IR 包（按值移动）。
     * @return true 表示已成功投递到底层连接。
     * @note 当玩家连接不存在或已断开时返回 false，不抛出异常。
     */
    [[nodiscard]] bool _sendIrPacket(mc::network::ir::IrPacket packet) const;

    /**
     * @brief 发送 SetCameraPacket 给客户端
     * @param cameraEntityId 摄像机实体的 ID（玩家自身 ID 表示恢复正常视角）
     */
    void _sendSetCameraPacket(u32 cameraEntityId);

    /**
     * @brief 执行跨维度迁移核心逻辑（changeDimension 与 teleportToDimension 共用）。
     *
     * 已知目标维度与目标坐标后，统一完成：迁移 EntityManager（源世界 removeEntity → 目标世界
     * spawnEntity）+ setDimension/setPosition/setWorld + transferPlayerToDimension（更新
     * m_playerDimensions + 发维度切换包 + 区块卸载/加载）。坐标/朝向由调用方传入：
     * - changeDimension 传 Teleporter 计算的坐标（末地固定 (100,49,0)、下界/主世界搜索传送门）；
     * - teleportToDimension 传 /tp 命令显式坐标。
     *
     * @param targetDim 目标维度ID
     * @param targetPos 目标坐标（已按 Teleporter 或命令指定）
     * @return true 如果迁移成功
     */
    [[nodiscard]] bool _performDimensionTransfer(DimensionId targetDim, const Vector3d& targetPos);

private:
    mc::server::net::ServerClientConnection* m_connection = nullptr;
    server::ServerWorld* m_world = nullptr;
    server::IServer* m_server = nullptr;
    std::shared_ptr<server::PlayerAdvancements> m_advancements;
    server::stats::StatisticsManager m_statistics;
    crafting::ServerRecipeBook m_recipeBook; ///< 配方书
    bool m_online = true;

    // ========== 肩部实体存储（对齐 vanilla ServerPlayer 肩部鹦鹉） ==========
    // vanilla ServerPlayer.java:272-273, 2234-2250：
    //   private long timeEntitySatOnShoulder;
    //   private CompoundTag shoulderEntityLeft;
    //   private CompoundTag shoulderEntityRight;
    // 玩家死亡或潜行时，removeEntitiesOnShoulder() 将肩部实体生成回世界并清空 NBT。
    // 注意：Cubium 当前无驯服/鹦鹉落肩玩法，这两个字段恒为空 compound，
    //       removeEntitiesOnShoulder() 在空 NBT 时为无操作，不影响死亡链路正确性。
    /// 肩部实体最后一次落肩的游戏时间（对齐 timeEntitySatOnShoulder）。
    /// removeEntitiesOnShoulder 仅在 timeEntitySatOnShoulder + 20 < gameTime 时执行，
    /// 避免玩家刚让鹦鹉落肩就被立即生成回世界。
    i64 m_timeEntitySatOnShoulder = -1;
    /// 左肩实体 NBT（对齐 shoulderEntityLeft）。
    nbt::tags::compound_tag m_shoulderEntityLeft;
    /// 右肩实体 NBT（对齐 shoulderEntityRight）。
    nbt::tags::compound_tag m_shoulderEntityRight;

    // 反飞行阈值校验跨 tick 基线（对齐 Java ServerGamePacketListenerImpl）
    f64 m_firstGoodX = 0.0;
    f64 m_firstGoodY = 0.0;
    f64 m_firstGoodZ = 0.0;
    f64 m_lastGoodX = 0.0;
    f64 m_lastGoodY = 0.0;
    f64 m_lastGoodZ = 0.0;
    i32 m_receivedMovePacketCount = 0;
    i32 m_knownMovePacketCount = 0;
    bool m_antiFlightBaselineInited = false;

    // 载具反飞行基线（对齐 Java vehicleFirstGood/vehicleLastGood）
    f64 m_vehicleFirstGoodX = 0.0;
    f64 m_vehicleFirstGoodY = 0.0;
    f64 m_vehicleFirstGoodZ = 0.0;
    f64 m_vehicleLastGoodX = 0.0;
    f64 m_vehicleLastGoodY = 0.0;
    f64 m_vehicleLastGoodZ = 0.0;
    bool m_vehicleAntiFlightInited = false;
    EntityInstanceId m_lastVehicleId = INVALID_ENTITY_ID;

    // 方块变更 ACK 累积序列号（对齐 Java ServerGamePacketListenerImpl.ackBlockChangesUpTo，
    // 字段定义于该类 :234，取 max 累积、每 tick 末批量发送一个 ClientboundBlockChangedAckPacket）。
    // -1 表示本 tick 无待发 ACK。
    i32 m_ackBlockChangesUpTo = -1;
};

} // namespace mc