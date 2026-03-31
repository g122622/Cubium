#pragma once

#include "RaiderType.hpp"
#include "../../block/BlockPos.hpp"
#include "../../../core/Types.hpp"
#include <vector>
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class Entity;
class LivingEntity;

namespace world {
namespace village {
class Village;
}

namespace village {
namespace raid {

/**
 * @brief 袭击波次数据
 */
struct RaidWave {
    i32 waveNumber;                     ///< 波次编号
    std::vector<EntityId> raiders;      ///< 该波的掠夺者
    i32 spawnCount;                     ///< 已生成数量
    i32 totalToSpawn;                   ///< 总共需要生成数量
    bool spawned = false;               ///< 是否已生成
    bool defeated = false;              ///< 是否已被击败
};

/**
 * @brief 袭击事件
 *
 * 管理单个村庄的袭击事件，包括波次、掠夺者、Boss栏等。
 *
 * 参考 MC 1.16.5 Raid
 */
class Raid {
public:
    /**
     * @brief 构造函数
     * @param id 袭击ID
     * @param village 关联的村庄（可为空）
     */
    Raid(RaidId id, village::Village* village);

    ~Raid() = default;

    // 禁止拷贝
    Raid(const Raid&) = delete;
    Raid& operator=(const Raid&) = delete;

    // 允许移动
    Raid(Raid&&) = default;
    Raid& operator=(Raid&&) = default;

    // ========== 基本信息 ==========

    /**
     * @brief 获取袭击ID
     */
    [[nodiscard]] RaidId id() const { return m_id; }

    /**
     * @brief 获取关联的村庄
     */
    [[nodiscard]] village::Village* village() const { return m_village; }

    /**
     * @brief 获取当前波次
     */
    [[nodiscard]] i32 wave() const { return m_wave; }

    /**
     * @brief 获取总波次数
     * @param difficulty 难度（0=简单，1=普通，2=困难）
     * @return 总波次数
     */
    [[nodiscard]] i32 maxWaves(i32 difficulty) const;

    /**
     * @brief 获取袭击状态
     */
    [[nodiscard]] RaidStatus status() const { return m_status; }

    /**
     * @brief 获取袭击中心位置
     */
    [[nodiscard]] BlockPos center() const { return m_center; }

    /**
     * @brief 设置袭击中心
     */
    void setCenter(BlockPos pos) { m_center = pos; }

    // ========== 时间 ==========

    /**
     * @brief 获取已持续的tick数
     */
    [[nodiscard]] i64 ticksActive() const { return m_ticksActive; }

    /**
     * @brief 获取开始时间
     */
    [[nodiscard]] i64 startTime() const { return m_startTime; }

    // ========== 掠夺者管理 ==========

    /**
     * @brief 获取所有存活的掠夺者
     */
    [[nodiscard]] const std::vector<EntityId>& raiders() const { return m_raiders; }

    /**
     * @brief 获取存活的掠夺者数量
     */
    [[nodiscard]] i32 getAliveRaidersCount() const;

    /**
     * @brief 添加掠夺者
     */
    void addRaider(EntityId raider);

    /**
     * @brief 移除掠夺者（死亡时调用）
     */
    void removeRaider(EntityId raider);

    /**
     * @brief 掠夺者死亡处理
     * @param raider 死亡的掠夺者
     * @param world 世界引用
     */
    void onRaiderDeath(EntityId raider, IWorld& world);

    // ========== 波次管理 ==========

    /**
     * @brief 开始下一波
     */
    void startNextWave(IWorld& world);

    /**
     * @brief 生成掠夺者
     */
    void spawnRaiders(IWorld& world);

    /**
     * @brief 检查当前波次是否完成
     */
    [[nodiscard]] bool isWaveDefeated() const;

    /**
     * @brief 检查是否还有剩余波次
     */
    [[nodiscard]] bool hasMoreWaves() const;

    // ========== 状态更新 ==========

    /**
     * @brief 每tick更新
     */
    void tick(IWorld& world);

    /**
     * @brief 检查袭击是否有效（村庄仍存在等）
     */
    [[nodiscard]] bool isValid() const;

    /**
     * @brief 停止袭击
     */
    void stop();

    /**
     * @brief 标记为胜利（玩家方胜利）
     */
    void setVictory();

    /**
     * @brief 标记为失败（掠夺者胜利）
     */
    void setLoss();

    // ========== Boss栏 ==========

    /**
     * @brief 获取Boss栏标题
     */
    [[nodiscard]] String getBossBarTitle() const;

    /**
     * @brief 获取Boss栏进度（0-1）
     */
    [[nodiscard]] f32 getBossBarProgress() const;

    // ========== 不祥之兆 ==========

    /**
     * @brief 获取不祥之兆等级
     */
    [[nodiscard]] i32 badOmenLevel() const { return m_badOmenLevel; }

    /**
     * @brief 设置不祥之兆等级
     */
    void setBadOmenLevel(i32 level) { m_badOmenLevel = level; }

private:
    /**
     * @brief 更新Boss栏
     */
    void updateBossBar();

    /**
     * @brief 生成单波掠夺者
     */
    void spawnWave(IWorld& world, i32 waveNum);

    /**
     * @brief 计算每波的掠夺者数量
     */
    [[nodiscard]] i32 calculateRaidersForWave(i32 waveNum, i32 difficulty) const;

    /**
     * @brief 选择掠夺者类型
     */
    [[nodiscard]] RaiderType selectRaiderType(i32 waveNum, i32 index, i32 total) const;

    /**
     * @brief 生成单个掠夺者
     */
    EntityId spawnRaider(IWorld& world, RaiderType type, BlockPos pos);

    /**
     * @brief 查找生成位置
     */
    [[nodiscard]] std::optional<BlockPos> findSpawnPosition(IWorld& world) const;

private:
    RaidId m_id;                           ///< 袭击ID
    village::Village* m_village;            ///< 关联的村庄
    BlockPos m_center;                     ///< 袭击中心
    i32 m_wave = 0;                        ///< 当前波次
    RaidStatus m_status = RaidStatus::Ongoing;  ///< 袭击状态
    i64 m_startTime = 0;                   ///< 开始时间
    i64 m_ticksActive = 0;                 ///< 已持续时间
    i64 m_lastWaveTime = 0;                ///< 上一波时间

    std::vector<EntityId> m_raiders;       ///< 所有掠夺者
    std::vector<RaidWave> m_waves;         ///< 波次数据

    i32 m_badOmenLevel = 1;                ///< 不祥之兆等级
    i32 m_groupsSpawned = 0;               ///< 已生成的组数
    i32 m_postRaidTicks = 0;               ///< 袭击后tick数
    i32 m_celebrateTicks = 0;              ///< 庆祝tick数
};

} // namespace raid
} // namespace village
} // namespace world
} // namespace mc
