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

#include "../../../command/ICommandSource.hpp"
#include "../../../core/Types.hpp"
#include "../../block/BlockPos.hpp"
#include "RaiderType.hpp"

#include <optional>
#include <unordered_set>
#include <vector>

namespace mc {
class IWorld;
class Entity;

namespace world::village {
class Village;
}

namespace test {
class RaidTestAccessor; // 测试访问器，声明为 friend 以访问 private 成员
}

namespace world::village::raid {

/**
 * @brief 单个波次的运行时数据。
 *
 * 该结构为后续更精细的波次调度保留扩展点，目前主要用于承载生成和清场状态。
 */
struct RaidWave {
    i32 waveNumber = 0;
    std::vector<EntityInstanceId> raiders;
    i32 spawnCount = 0;
    i32 totalToSpawn = 0;
    bool spawned = false;
    bool defeated = false;
};

/**
 * @brief 袭击参与者信息。
 *
 * 记录参与袭击的玩家及其贡献，用于在袭击胜利时给予奖励。
 */
struct RaidParticipant {
    Uuid uuid;                 ///< 玩家 UUID
    EntityInstanceId entityId; ///< 玩家实体 ID（可能失效）
    i32 contribution = 0;      ///< 贡献值（击杀袭击者数量）

    RaidParticipant() = default;
    RaidParticipant(Uuid playerUuid, EntityInstanceId id)
        : uuid(playerUuid)
        , entityId(id)
        , contribution(0)
    {}
};

/**
 * @brief 单次村庄袭击事件。
 *
 * 负责管理波次推进、袭击者生成、胜负判定与 Boss 栏基础状态。
 * 当前实现保持接口稳定，后续可继续向更贴近 Java 版逻辑的波次配置演进。
 *
 * @note TODO(bossbar-network-sync): 当前 Raid 仅在内部计算并缓存 Boss 栏进度，
 *       尚未接入 ServerBossInfo / BossInfoPacket 网络同步层，因此 m_cachedProgress
 *       不会真正推送到客户端。原因：Raid 位于 src/common，而 ServerBossInfo 位于
 *       src/server，且客户端 BossInfoPacket 处理路径尚未实现。待客户端 BossInfoPacket
 *       处理器落地、且 BossInfo 系统能在 Raid 构造期注入后，再在 _updateBossBar()
 *       中追加 ServerBossInfo::setProgress() 调用，并由 RaidManager 持有
 *       ServerBossInfo 引用以同步 addPlayer/removePlayer。
 */
class Raid {
public:
    /**
     * @brief 构造袭击事件。
     *
     * @param id 袭击唯一标识。
     * @param village 关联村庄，可为空。
     *
     * @note 当 `village == nullptr` 时，袭击会处于不可用状态，部分行为会提前退出。
     */
    Raid(RaidId id, village::Village* village);

    ~Raid() = default;
    Raid(const Raid&) = delete;
    Raid& operator=(const Raid&) = delete;
    Raid(Raid&&) = default;
    Raid& operator=(Raid&&) = default;

    /**
     * @brief 获取袭击 ID。
     */
    [[nodiscard]] RaidId id() const { return m_id; }

    /**
     * @brief 获取关联村庄。
     *
     * @return 关联村庄指针，可能为空。
     */
    [[nodiscard]] village::Village* village() const { return m_village; }

    /**
     * @brief 获取当前波次。
     *
     * @return 当前波次编号，0 表示尚未开始。
     */
    [[nodiscard]] i32 wave() const { return m_wave; }

    /**
     * @brief 根据难度计算基础波次数。
     *
     * @param difficulty 世界难度。
     * @return 对应难度下的基础波次数。
     *
     * @note 该值不包含不祥之兆附加波次。
     */
    [[nodiscard]] i32 maxWaves(Difficulty difficulty) const;

    /**
     * @brief 获取袭击状态。
     */
    [[nodiscard]] RaidStatus status() const { return m_status; }

    /**
     * @brief 获取袭击中心。
     */
    [[nodiscard]] BlockPos center() const { return m_center; }

    /**
     * @brief 设置袭击中心。
     *
     * @param pos 新的中心方块坐标。
     */
    void setCenter(BlockPos pos) { m_center = pos; }

    /**
     * @brief 获取袭击已运行的 tick 数。
     */
    [[nodiscard]] i64 ticksActive() const { return m_ticksActive; }

    /**
     * @brief 获取开始时间戳。
     *
     * @note 当前字段主要为后续序列化和统计预留，默认值为 0。
     */
    [[nodiscard]] i64 startTime() const { return m_startTime; }

    /**
     * @brief 获取当前存活袭击者列表。
     *
     * @return 袭击者实体 ID 列表引用。
     *
     * @warning 该列表只表示当前 Raid 追踪到的实体，不保证实体仍然存在于世界中。
     */
    [[nodiscard]] const std::vector<EntityInstanceId>& raiders() const { return m_raiders; }

    /**
     * @brief 获取仍被追踪为存活的袭击者数量。
     */
    [[nodiscard]] i32 getAliveRaidersCount() const;

    /**
     * @brief 将实体加入袭击追踪列表。
     *
     * @param raider 袭击者实体 ID。
     *
     * @note 重复加入会被忽略，以保持列表去重。
     */
    void addRaider(EntityInstanceId raider);

    /**
     * @brief 从袭击追踪列表移除实体。
     *
     * @param raider 袭击者实体 ID。
     */
    void removeRaider(EntityInstanceId raider);

    /**
     * @brief 处理袭击者死亡事件。
     *
     * @param raider 死亡的袭击者实体 ID。
     * @param world 所属世界。
     *
     * @note 当前仅更新 Raid 内部状态，战利品和村庄声望尚未接入。
     */
    void onRaiderDeath(EntityInstanceId raider, IWorld& world);

    /**
     * @brief 启动下一波。
     *
     * @param world 所属世界。
     *
     * @warning 调用前应确保当前波已结束，否则会直接推进波次编号。
     */
    void startNextWave(IWorld& world);

    /**
     * @brief 生成当前波袭击者。
     *
     * @param world 所属世界。
     *
     * @note 若当前波次尚未初始化，会自动将其修正为第 1 波。
     */
    void spawnRaiders(IWorld& world);

    /**
     * @brief 判断当前波是否已被清空。
     */
    [[nodiscard]] bool isWaveDefeated() const;

    /**
     * @brief 判断是否还有后续波次。
     *
     * @note 该判断基于记录的难度和不祥之兆等级。
     */
    [[nodiscard]] bool hasMoreWaves() const;

    /**
     * @brief 执行一次袭击 tick。
     *
     * @param world 所属世界。
     *
     * @warning 该方法假设由单线程世界主循环调用，不做并发同步。
     */
    void tick(IWorld& world);

    /**
     * @brief 判断袭击是否仍然有效。
     *
     * @return 若关联村庄仍存在则返回 true。
     */
    [[nodiscard]] bool isValid() const;

    /**
     * @brief 强制停止袭击。
     */
    void stop();

    /**
     * @brief 将袭击标记为胜利。
     *
     * @note 胜利方为玩家阵营。
     */
    void setVictory();

    /**
     * @brief 将袭击标记为失败。
     *
     * @note 失败表示袭击者阵营获胜或 Raid 运行失败。
     */
    void setLoss();

    /**
     * @brief 获取 Boss 栏标题。
     *
     * @return 当前状态对应的文本。
     */
    [[nodiscard]] std::string getBossBarTitle() const;

    /**
     * @brief 获取 Boss 栏进度。
     *
     * @return 0.0 到 1.0 的进度值。
     *
     * @note 返回的是 _updateBossBar() 在最近一次 tick 中缓存的进度值，
     *       避免外部高频调用导致重复遍历袭击者列表。进度遵循 Java 版 1.21.11
     *       的三段式语义：战斗中按存活血量比例、波间冷却按 300 tick 倒计时比例、
     *       胜利/失败/停止后归零。
     *
     * @note TODO(bossbar-network-sync): 该值目前仅作为公共 API 暴露给潜在的
     *       调用方（命令系统、调试接口、未来的 BossInfo 网络包等），尚未被任何
     *       下游模块消费。完整的 Boss 栏网络同步链路见 _updateBossBar() 注释。
     */
    [[nodiscard]] f32 getBossBarProgress() const;

    /**
     * @brief 获取不祥之兆等级。
     */
    [[nodiscard]] i32 badOmenLevel() const { return m_badOmenLevel; }

    /**
     * @brief 设置不祥之兆等级。
     *
     * @param level 不祥之兆等级。
     *
     * @warning 调用方需保证等级大于等于 1。
     */
    void setBadOmenLevel(i32 level) { m_badOmenLevel = level; }

    // ========== 英雄追踪 ==========

    /**
     * @brief 添加英雄（参与袭击的玩家）。
     *
     * 在玩家击败袭击者时调用，记录玩家对袭击的贡献。
     * 袭击胜利时会给予这些玩家"村庄英雄"效果。
     *
     * @param playerUuid 玩家 UUID。
     * @param entityId 玩家实体 ID。
     */
    void addHero(Uuid playerUuid, EntityInstanceId entityId);

    /**
     * @brief 检查玩家是否为英雄。
     *
     * @param playerUuid 玩家 UUID。
     * @return 是否为英雄。
     */
    [[nodiscard]] bool isHero(Uuid playerUuid) const;

    /**
     * @brief 获取所有英雄 UUID 列表。
     *
     * @return 英雄 UUID 集合。
     */
    [[nodiscard]] const std::unordered_set<Uuid, UuidHash>& heroes() const { return m_heroes; }

    /**
     * @brief 增加玩家贡献值。
     *
     * @param playerUuid 玩家 UUID。
     * @param amount 增加的贡献值数量。
     */
    void addContribution(Uuid playerUuid, i32 amount = 1);

    /**
     * @brief 获取玩家贡献值。
     *
     * @param playerUuid 玩家 UUID。
     * @return 贡献值，如果玩家不是英雄则返回 0。
     */
    [[nodiscard]] i32 getContribution(Uuid playerUuid) const;

private:
    /// 测试访问器，可访问私有方法 _updateBossBar / _getHealthOfLivingRaiders 等。
    friend class test::RaidTestAccessor;

    /**
     * @brief 更新 Boss 栏内部状态。
     *
     * @param world 所属世界，用于查询袭击者实体的实时血量。
     *
     * @note 该方法在每次 tick() 末尾调用，依据当前袭击阶段（战斗 / 波间冷却 /
     *       庆祝）按 Java 版 1.21.11 的语义计算进度并写入 m_cachedProgress。
     *
     * @note TODO(bossbar-network-sync): 当前仅写入本地缓存 m_cachedProgress，
     *       未调用 ServerBossInfo::setProgress() 推送到客户端。原因：Raid 位于
     *       src/common，无法直接依赖 src/server 的 ServerBossInfo；且客户端
     *       BossInfoPacket 处理路径尚未实现。完整的网络同步集成需要：
     *       1) 在 RaidManager 中为每个 Raid 关联一个 ServerBossInfo 实例；
     *       2) 在 _updateBossBar() 末尾调用 serverBossInfo->setProgress(m_cachedProgress)；
     *       3) 实现 RaidCallbacks::onBossBarProgressChanged 回调或类似机制，
     *          由 RaidManager 转发到 ServerBossInfo；
     *       4) 客户端实现 BossInfoPacket 处理器以渲染 Boss 栏 UI。
     */
    void _updateBossBar(IWorld& world);

    /**
     * @brief 计算当前所有存活袭击者的总血量。
     *
     * @param world 所属世界，用于查询实体。
     * @return 当前追踪的袭击者血量之和。
     *
     * @note 当袭击者已离开世界或已死亡时，其血量不计入；该函数对应 Java 版
     *       `Raid#getHealthOfLivingRaiders()`。
     */
    [[nodiscard]] f32 _getHealthOfLivingRaiders(IWorld& world) const;

    /**
     * @brief 生成指定波次。
     *
     * @param world 所属世界。
     * @param waveNum 目标波次编号。
     *
     * @note 这是 `spawnRaiders()` 的显式波次入口，便于未来做预生成或重放测试。
     */
    void _spawnWave(IWorld& world, i32 waveNum);

    /**
     * @brief 计算指定波次的袭击者数量。
     *
     * @param waveNum 波次编号。
     * @param difficulty 世界难度。
     * @return 该波次应生成的袭击者数量。
     */
    [[nodiscard]] i32 _calculateRaidersForWave(i32 waveNum, Difficulty difficulty) const;

    /**
     * @brief 为本波中的单个索引选择袭击者类型。
     *
     * @param waveNum 波次编号。
     * @param index 当前生成索引。
     * @param total 本波总生成数量。
     * @return 选择出的袭击者类型。
     */
    [[nodiscard]] RaiderType _selectRaiderType(i32 waveNum, i32 index, i32 total) const;

    /**
     * @brief 生成单个袭击者实体。
     *
     * @param world 所属世界。
     * @param type 袭击者类型。
     * @param pos 生成方块坐标。
     * @return 新实体 ID，失败时返回 0。
     */
    EntityInstanceId _spawnRaider(IWorld& world, RaiderType type, BlockPos pos);

    /**
     * @brief 查找本次波次的生成位置。
     *
     * @param world 所属世界。
     * @return 可用生成位置；若无法生成则返回空值。
     */
    [[nodiscard]] std::optional<BlockPos> _findSpawnPosition(IWorld& world) const;

private:
    RaidId m_id;
    village::Village* m_village = nullptr;
    BlockPos m_center;
    i32 m_wave = 0;
    RaidStatus m_status = RaidStatus::Ongoing;
    i64 m_startTime = 0;
    i64 m_ticksActive = 0;
    i64 m_lastWaveTime = 0;
    std::vector<EntityInstanceId> m_raiders;
    std::vector<RaidWave> m_waves;
    Difficulty m_difficulty = Difficulty::Normal;
    i32 m_badOmenLevel = 1;
    i32 m_groupsSpawned = 0;
    i32 m_postRaidTicks = 0;
    i32 m_celebrateTicks = 0;

    // Boss 栏进度追踪
    // 三个字段共同实现 Java 版 1.21.11 Raid 的 BossBar 行为：
    //  - m_totalHealth：当前波所有袭击者生成完成时的初始血量之和，作为分母
    //  - m_raidCooldownTicks：波间冷却倒计时（0 表示未在冷却中，300 表示刚启动）
    //  - m_cachedProgress：_updateBossBar() 计算后缓存给 getBossBarProgress() 返回的值
    f32 m_totalHealth = 0.0f;
    i32 m_raidCooldownTicks = 0;
    f32 m_cachedProgress = 0.0f;

    // 英雄追踪
    std::unordered_set<Uuid, UuidHash> m_heroes; ///< 参与袭击的玩家 UUID
    std::vector<RaidParticipant> m_participants; ///< 参与者详细信息
};

} // namespace world::village::raid
} // namespace mc
