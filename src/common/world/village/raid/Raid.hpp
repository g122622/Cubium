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

namespace world::village::raid {

/**
 * @brief 单个波次的运行时数据。
 *
 * 该结构为后续更精细的波次调度保留扩展点，目前主要用于承载生成和清场状态。
 */
struct RaidWave {
    i32 waveNumber = 0;
    std::vector<EntityId> raiders;
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
    Uuid uuid;            ///< 玩家 UUID
    EntityId entityId;    ///< 玩家实体 ID（可能失效）
    i32 contribution = 0; ///< 贡献值（击杀袭击者数量）

    RaidParticipant() = default;
    RaidParticipant(Uuid playerUuid, EntityId id)
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
    [[nodiscard]] const std::vector<EntityId>& raiders() const { return m_raiders; }

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
    void addRaider(EntityId raider);

    /**
     * @brief 从袭击追踪列表移除实体。
     *
     * @param raider 袭击者实体 ID。
     */
    void removeRaider(EntityId raider);

    /**
     * @brief 处理袭击者死亡事件。
     *
     * @param raider 死亡的袭击者实体 ID。
     * @param world 所属世界。
     *
     * @note 当前仅更新 Raid 内部状态，战利品和村庄声望尚未接入。
     */
    void onRaiderDeath(EntityId raider, IWorld& world);

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
     * @note 当前进度为简化实现，并非 Java 版的最终精确行为。
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
    void addHero(Uuid playerUuid, EntityId entityId);

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
    /**
     * @brief 更新 Boss 栏内部状态。
     *
     * @note 当前仅保留扩展点，未来接入同步系统后在此集中更新。
     */
    void updateBossBar();

    /**
     * @brief 生成指定波次。
     *
     * @param world 所属世界。
     * @param waveNum 目标波次编号。
     *
     * @note 这是 `spawnRaiders()` 的显式波次入口，便于未来做预生成或重放测试。
     */
    void spawnWave(IWorld& world, i32 waveNum);

    /**
     * @brief 计算指定波次的袭击者数量。
     *
     * @param waveNum 波次编号。
     * @param difficulty 世界难度。
     * @return 该波次应生成的袭击者数量。
     */
    [[nodiscard]] i32 calculateRaidersForWave(i32 waveNum, Difficulty difficulty) const;

    /**
     * @brief 为本波中的单个索引选择袭击者类型。
     *
     * @param waveNum 波次编号。
     * @param index 当前生成索引。
     * @param total 本波总生成数量。
     * @return 选择出的袭击者类型。
     */
    [[nodiscard]] RaiderType selectRaiderType(i32 waveNum, i32 index, i32 total) const;

    /**
     * @brief 生成单个袭击者实体。
     *
     * @param world 所属世界。
     * @param type 袭击者类型。
     * @param pos 生成方块坐标。
     * @return 新实体 ID，失败时返回 0。
     */
    EntityId spawnRaider(IWorld& world, RaiderType type, BlockPos pos);

    /**
     * @brief 查找本次波次的生成位置。
     *
     * @param world 所属世界。
     * @return 可用生成位置；若无法生成则返回空值。
     */
    [[nodiscard]] std::optional<BlockPos> findSpawnPosition(IWorld& world) const;

private:
    RaidId m_id;
    village::Village* m_village = nullptr;
    BlockPos m_center;
    i32 m_wave = 0;
    RaidStatus m_status = RaidStatus::Ongoing;
    i64 m_startTime = 0;
    i64 m_ticksActive = 0;
    i64 m_lastWaveTime = 0;
    std::vector<EntityId> m_raiders;
    std::vector<RaidWave> m_waves;
    Difficulty m_difficulty = Difficulty::Normal;
    i32 m_badOmenLevel = 1;
    i32 m_groupsSpawned = 0;
    i32 m_postRaidTicks = 0;
    i32 m_celebrateTicks = 0;

    // 英雄追踪（MC 1.16.5: heroes 字段）
    std::unordered_set<Uuid, UuidHash> m_heroes; ///< 参与袭击的玩家 UUID
    std::vector<RaidParticipant> m_participants; ///< 参与者详细信息
};

} // namespace world::village::raid
} // namespace mc
