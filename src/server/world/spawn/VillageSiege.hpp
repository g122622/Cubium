#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class Player;

namespace server {
class ServerWorld; // 前向声明
}

namespace world::village {
class VillageManager;
class Village;
} // namespace world::village

namespace server::spawn {

/**
 * @brief 村庄围攻（僵尸围村）系统
 *
 * 实现 Minecraft 1.16.5 的僵尸围村机制。
 * 在午夜时刻，如果满足条件，有10%概率触发僵尸围村事件。
 *
 * 触发条件：
 * - 夜晚（不是白天）
 * - 午夜时刻（天体角度 0.5）
 * - 有非旁观者玩家在村庄内
 * - 玩家所在生物群系不是蘑菇岛
 * - 10% 概率触发
 *
 * 围攻过程：
 * - 总共生成 20 个僵尸
 * - 每 2 tick 生成 1 个僵尸
 * - 僵尸在玩家周围 32 格圆周上生成
 * - 生成位置必须在村庄内且满足光照条件
 *
 * 参考 MC 1.16.5 VillageSiege
 */
class VillageSiege {
public:
    /**
     * @brief 围攻状态枚举
     */
    enum class State : u8 {
        /// 可以激活（等待午夜）
        CanActivate,
        /// 今晚发生围攻
        Tonight,
        /// 已完成/未激活
        Done
    };

    /**
     * @brief 构造函数
     */
    VillageSiege();

    /**
     * @brief 析构函数
     */
    ~VillageSiege() = default;

    // 禁止拷贝
    VillageSiege(const VillageSiege&) = delete;
    VillageSiege& operator=(const VillageSiege&) = delete;

    // 允许移动
    VillageSiege(VillageSiege&&) noexcept = default;
    VillageSiege& operator=(VillageSiege&&) noexcept = default;

    // ========== 主要接口 ==========

    /**
     * @brief 每tick更新
     *
     * 检查围攻条件，执行围攻逻辑。
     * 参考 MC 1.16.5 VillageSiege.func_230253_a_
     *
     * @param world 服务端世界
     * @param spawnHostiles 是否允许生成敌对生物
     * @return 生成的僵尸数量
     */
    i32 tick(server::ServerWorld& world, bool spawnHostiles);

    // ========== 状态查询 ==========

    /**
     * @brief 获取当前围攻状态
     */
    [[nodiscard]] State getState() const { return m_state; }

    /**
     * @brief 检查是否正在进行围攻
     */
    [[nodiscard]] bool isSiegeActive() const { return m_state == State::Tonight && m_siegeCount > 0; }

    /**
     * @brief 获取剩余僵尸数量
     */
    [[nodiscard]] i32 getRemainingZombies() const { return m_siegeCount; }

    /**
     * @brief 获取生成中心位置
     */
    [[nodiscard]] const BlockPos& getSpawnCenter() const { return m_spawnCenter; }

    // ========== 配置 ==========

    /**
     * @brief 围攻配置常量
     */
    struct Config {
        /// 触发围攻的概率（1/10 = 10%）
        static constexpr i32 TRIGGER_CHANCE = 10;

        /// 总共生成的僵尸数量
        static constexpr i32 TOTAL_ZOMBIES = 20;

        /// 每次生成之间的延迟（tick）
        static constexpr i32 SPAWN_DELAY = 2;

        /// 生成距离（玩家周围的圆周半径）
        static constexpr f32 SPAWN_DISTANCE = 32.0f;

        /// 查找生成位置的最大尝试次数
        static constexpr i32 MAX_SPAWN_ATTEMPTS = 10;

        /// 查找围攻设置位置的最大尝试次数
        static constexpr i32 MAX_SETUP_ATTEMPTS = 10;

        /// 生成位置随机偏移范围
        static constexpr i32 SPAWN_OFFSET_RANGE = 8;
    };

private:
    // ========== 状态字段 ==========

    /// 围攻状态
    State m_state = State::Done;

    /// 是否已设置围攻
    bool m_hasSetup = false;

    /// 剩余要生成的僵尸数量
    i32 m_siegeCount = 0;

    /// 下次生成的延迟（tick）
    i32 m_nextSpawnDelay = 0;

    /// 生成中心位置
    BlockPos m_spawnCenter;

    /// 随机数生成器
    math::Random m_random;

    // ========== 内部方法 ==========

    /**
     * @brief 尝试设置围攻
     *
     * 寻找符合条件的玩家并设置生成位置。
     *
     * @param world 服务端世界
     * @return 是否成功设置
     */
    bool trySetupSiege(server::ServerWorld& world);

    /**
     * @brief 生成一个僵尸
     *
     * @param world 服务端世界
     * @return 是否成功生成
     */
    bool spawnZombie(server::ServerWorld& world);

    /**
     * @brief 查找随机生成位置
     *
     * 在指定位置附近寻找有效的僵尸生成点。
     *
     * @param world 世界
     * @param searchCenter 搜索中心
     * @return 有效生成位置，如果找不到返回空
     */
    [[nodiscard]] std::optional<BlockPos> findRandomSpawnPos(IWorld& world, const BlockPos& searchCenter);

    /**
     * @brief 检查位置是否可以生成怪物
     *
     * @param world 世界
     * @param pos 位置
     * @return 是否可以生成
     */
    [[nodiscard]] bool canMonsterSpawnAt(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查是否为午夜时刻
     *
     * @param world 世界
     * @return 是否为午夜
     */
    [[nodiscard]] bool isMidnight(server::ServerWorld& world) const;

    /**
     * @brief 检查玩家是否在有效的村庄内
     *
     * @param world 世界
     * @param playerPos 玩家位置
     * @return 是否在有效村庄内
     */
    [[nodiscard]] bool isInValidVillage(server::ServerWorld& world, const BlockPos& playerPos);

    /**
     * @brief 检查位置是否在蘑菇岛生物群系
     *
     * 蘑菇岛是安全区域，不会发生僵尸围攻。
     * MC 1.16.5: getBiome(blockpos).getCategory() != Biome.Category.MUSHROOM
     *
     * @param world 世界
     * @param pos 位置
     * @return 是否为蘑菇岛生物群系
     */
    [[nodiscard]] bool isMushroomBiome(server::ServerWorld& world, const BlockPos& pos);
};

} // namespace server::spawn
} // namespace mc
