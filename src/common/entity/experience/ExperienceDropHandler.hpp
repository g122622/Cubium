#pragma once

#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/math/random/Random.hpp"
#include "ExperienceConstants.hpp"
#include "ExperienceUtils.hpp"
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class IWorld;
class Player;
class Entity;
class ExperienceOrbEntity;

namespace entity {

/**
 * @brief 经验掉落处理器
 *
 * 负责在世界中生成经验球实体。
 * 提供统一的经验掉落接口。
 *
 * 参考 MC 1.16.5 ExperienceOrbEntity.spawn()
 */
class ExperienceDropHandler {
public:
    /**
     * @brief 在指定位置生成经验球
     *
     * 将经验值分割成多个经验球，并设置随机速度散射。
     *
     * @param world 世界指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param totalXp 总经验值
     * @param rng 随机数生成器（可选，用于随机化速度）
     * @return 生成的经验球数量
     */
    static i32 spawnExperienceOrbs(IWorld* world, f64 x, f64 y, f64 z, i32 totalXp, math::Random* rng = nullptr);

    /**
     * @brief 在指定位置生成经验球（向量位置）
     *
     * @param world 世界指针
     * @param pos 位置向量
     * @param totalXp 总经验值
     * @param rng 随机数生成器（可选）
     * @return 生成的经验球数量
     */
    static i32 spawnExperienceOrbs(IWorld* world, const Vector3& pos, i32 totalXp, math::Random* rng = nullptr)
    {
        return spawnExperienceOrbs(world, pos.x, pos.y, pos.z, totalXp, rng);
    }

    /**
     * @brief 在实体位置生成经验球
     *
     * @param entity 实体指针
     * @param totalXp 总经验值
     * @param rng 随机数生成器（可选）
     * @return 生成的经验球数量
     */
    static i32 spawnExperienceOrbs(Entity* entity, i32 totalXp, math::Random* rng = nullptr);

    /**
     * @brief 生成玩家死亡掉落的经验
     *
     * 计算玩家死亡时掉落的经验值并在位置生成经验球。
     * 掉落量 = min(level * 7, 100)
     *
     * @param player 玩家指针
     * @return 生成的经验球数量
     */
    static i32 spawnPlayerDeathXp(Player* player);

    /**
     * @brief 生成随机矿石经验
     *
     * @param world 世界指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param oreType 矿石类型 (0=煤矿, 1=钻石, 2=绿宝石, 3=青金石, 4=下界石英, 5=下界金, 6=红石, 7=刷怪笼)
     * @param rng 随机数生成器
     * @return 生成的经验球数量
     */
    static i32 spawnOreExperience(IWorld* world, f64 x, f64 y, f64 z, i32 oreType, math::Random& rng);

    /**
     * @brief 生成钓鱼经验
     *
     * @param world 世界指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param rng 随机数生成器
     * @return 生成的经验球数量
     */
    static i32 spawnFishingExperience(IWorld* world, f64 x, f64 y, f64 z, math::Random& rng);

    /**
     * @brief 生成被动动物死亡经验
     *
     * 动物死亡时掉落 1-3 点经验。
     *
     * @param world 世界指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param rng 随机数生成器
     * @return 生成的经验球数量
     */
    static i32 spawnPassiveMobExperience(IWorld* world, f64 x, f64 y, f64 z, math::Random& rng);

    /**
     * @brief 生成怪物死亡经验
     *
     * 普通怪物掉落固定经验值。
     *
     * @param world 世界指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param baseXp 基础经验值
     * @param rng 随机数生成器（可选）
     * @return 生成的经验球数量
     */
    static i32 spawnHostileMobExperience(IWorld* world, f64 x, f64 y, f64 z, i32 baseXp, math::Random* rng = nullptr);

private:
    /**
     * @brief 创建单个经验球实体
     *
     * @param world 世界指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param xpValue 经验值
     * @param vx X方向速度
     * @param vy Y方向速度
     * @param vz Z方向速度
     * @return 创建的经验球实体指针
     */
    static ExperienceOrbEntity* createExperienceOrb(
        IWorld* world, f64 x, f64 y, f64 z, i32 xpValue, f32 vx, f32 vy, f32 vz);
};

} // namespace entity
} // namespace mc
