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

#include "common/world/chunk/data/Heightmap.hpp"
#include "core/Types.hpp"
#include "util/math/Vector3.hpp"
#include "util/math/random/IRandom.hpp"
#include "world/chunk/data/IChunk.hpp"
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace mc {

// 前向声明
class BlockState;
class Block;

namespace entity {
class EntityType;
enum class EntityClassification : u8;
} // namespace entity

namespace world::spawn {

/**
 * @brief 生成原因枚举
 *
 * 定义实体生成的各种原因，用于决定生成规则和实体初始化行为。
 * 不同生成原因可能影响实体的初始状态和行为。
 */
enum class SpawnReason : u8 {
    /// 自然生成 - 常规的自然刷新生成
    Natural,

    /// 区块生成 - 区块首次生成时放置实体（如动物群）
    ChunkGeneration,

    /// 刷怪笼 - 由刷怪笼生成
    Spawner,

    /// 结构生成 - 由世界结构生成（如村民、掠夺者）
    Structure,

    /// 繁殖 - 通过繁殖产生（如动物幼崽、村民婴儿）
    Breeding,

    /// 召唤 - 被其他生物召唤（如恼鬼被唤魔者召唤、铁傀儡被村民召唤）
    MobSummons,

    /// 骑乘 - 作为骑乘者生成（如小僵尸骑鸡、蜘蛛骑士）
    Jockey,

    /// 事件 - 由游戏事件触发（如袭击、僵尸围城、流浪商人刷新）
    Event,

    /// 转化 - 由其他实体转化而来（如僵尸村民变村民、溺尸转化）
    Conversion,

    /// 增援 - 僵尸召唤增援
    Reinforcement,

    /// 触发 - 由特定条件触发（如骷髅陷阱马、三叉戟闪电）
    Trigger,

    /// 水桶 - 从水桶释放（如鱼、美西螈）
    Bucket,

    /// 刷怪蛋 - 使用刷怪蛋生成
    SpawnEgg,

    /// 命令 - 由 /summon 命令生成
    Command,

    /// 发射器 - 由发射器使用刷怪蛋或水桶生成
    Dispenser,

    /// 巡逻队 - 掠夺者巡逻队成员生成
    Patrol,
    /// 试炼刷怪笼：忽略光照要求
    TrialSpawner,
    /// 存档加载
    Load,
    /// 跨维度移动
    DimensionTravel
};

/**
 * @brief 判断生成原因是否为刷怪笼类型
 *
 * 刷怪笼生成应跳过某些特殊条件检查（如史莱姆区块判定、沼泽地表条件）。
 * 包含 Spawner 和 TrialSpawner 两种刷怪笼类型。
 *
 * @param reason 生成原因
 * @return 是否为刷怪笼生成
 */
[[nodiscard]] inline bool isSpawnerReason(SpawnReason reason)
{
    return reason == SpawnReason::Spawner || reason == SpawnReason::TrialSpawner;
}

/**
 * @brief 获取生成原因的名称字符串
 *
 * @param reason 生成原因
 * @return 名称字符串（如 "natural", "chunk_generation"）
 */
[[nodiscard]] inline const char* getSpawnReasonName(SpawnReason reason)
{
    switch (reason) {
        case SpawnReason::Natural:
            return "natural";
        case SpawnReason::ChunkGeneration:
            return "chunk_generation";
        case SpawnReason::Spawner:
            return "spawner";
        case SpawnReason::Structure:
            return "structure";
        case SpawnReason::Breeding:
            return "breeding";
        case SpawnReason::MobSummons:
            return "mob_summoned";
        case SpawnReason::Jockey:
            return "jockey";
        case SpawnReason::Event:
            return "event";
        case SpawnReason::Conversion:
            return "conversion";
        case SpawnReason::Reinforcement:
            return "reinforcement";
        case SpawnReason::Trigger:
            return "triggered";
        case SpawnReason::Bucket:
            return "bucket";
        case SpawnReason::SpawnEgg:
            return "spawn_item_use";
        case SpawnReason::Command:
            return "command";
        case SpawnReason::Dispenser:
            return "dispenser";
        case SpawnReason::Patrol:
            return "patrol";
        case SpawnReason::TrialSpawner:
            return "trial_spawner";
        case SpawnReason::Load:
            return "load";
        case SpawnReason::DimensionTravel:
            return "dimension_travel";
        default:
            return "unknown";
    }
}

/**
 * @brief 根据名称字符串获取生成原因
 *
 * @param name 名称字符串（如 "natural", "chunk_generation"）
 * @return 生成原因，如果名称无效返回空值
 */
[[nodiscard]] inline std::optional<SpawnReason> getSpawnReasonByName(const std::string& name)
{
    if (name == "natural") {
        return SpawnReason::Natural;
    }
    if (name == "chunk_generation") {
        return SpawnReason::ChunkGeneration;
    }
    if (name == "spawner") {
        return SpawnReason::Spawner;
    }
    if (name == "structure") {
        return SpawnReason::Structure;
    }
    if (name == "breeding") {
        return SpawnReason::Breeding;
    }
    if (name == "mob_summoned") {
        return SpawnReason::MobSummons;
    }
    if (name == "jockey") {
        return SpawnReason::Jockey;
    }
    if (name == "event") {
        return SpawnReason::Event;
    }
    if (name == "conversion") {
        return SpawnReason::Conversion;
    }
    if (name == "reinforcement") {
        return SpawnReason::Reinforcement;
    }
    if (name == "triggered") {
        return SpawnReason::Trigger;
    }
    if (name == "bucket") {
        return SpawnReason::Bucket;
    }
    if (name == "spawn_item_use") {
        return SpawnReason::SpawnEgg;
    }
    if (name == "command") {
        return SpawnReason::Command;
    }
    if (name == "dispenser") {
        return SpawnReason::Dispenser;
    }
    if (name == "patrol") {
        return SpawnReason::Patrol;
    }
    if (name == "trial_spawner") {
        return SpawnReason::TrialSpawner;
    }
    if (name == "load") {
        return SpawnReason::Load;
    }
    if (name == "dimension_travel") {
        return SpawnReason::DimensionTravel;
    }
    return std::nullopt;
}

/**
 * @brief 实体生成放置类型
 *
 * 定义实体生成时需要的环境条件类型。
 */
enum class PlacementType : u8 {
    /// 在地面上生成（需要固体方块支撑）
    OnGround,

    /// 在水中生成
    InWater,

    /// 在岩浆中生成
    InLava,

    /// 无限制
    NoRestrictions
};

/** @brief 一次生成谓词读取的位置环境，不包含任何隐式默认值。 */
struct SpawnEnvironment {
    i32 skyLight;
    i32 blockLight;
    i32 skyDarkening;
    bool thundering;
    i32 seaLevel;
    i32 monsterBlockLightLimit;
    i32 monsterLightMinimum;
    i32 monsterLightMaximum;
    f32 surfaceSlimeSpawnChance;
};

/**
 * @brief 简化的世界读取接口
 *
 * 用于生成检查的最小接口，支持 IWorld 和 WorldGenRegion。
 */
class ISpawnWorldReader {
public:
    virtual ~ISpawnWorldReader() = default;

    /**
     * @brief 获取方块状态
     */
    [[nodiscard]] virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 检查方块位置是否在可配置的水平世界边界内，不检查建筑高度
     */
    [[nodiscard]] virtual bool isWithinWorldBorder(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取高度图值
     */
    [[nodiscard]] virtual i32 getHeight(HeightmapType type, i32 x, i32 z) const = 0;

    /**
     * @brief 获取指定位置的生物群系ID
     */
    [[nodiscard]] virtual BiomeId getBiome(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取世界种子
     *
     * 用于确定性生成判断（如史莱姆区块判定）。
     */
    [[nodiscard]] virtual u64 seed() const = 0;

    /**
     * @brief 获取世界难度
     */
    [[nodiscard]] virtual Difficulty difficulty() const = 0;

    /** @brief 获取生成位置的光照、维度门槛与环境属性。 */
    [[nodiscard]] virtual SpawnEnvironment environment(const Vector3i& pos) const = 0;

    /** @brief 查询严格小于指定距离的非创造、非旁观玩家。 */
    [[nodiscard]] virtual bool hasNearbyPlayer(const Vector3d& center, f64 distance) const = 0;
};

/**
 * @brief 实体生成放置注册表
 *
 * 管理每种实体类型的生成规则和放置条件。
 * 在区块生成和自然生成时用于验证生成位置。
 *
 * 使用方式：
 * @code
 * // 初始化（游戏启动时调用一次）
 * EntitySpawnPlacementRegistry::initializeDefaults();
 *
 * // 检查是否可以在指定位置生成
 * bool canSpawn = EntitySpawnPlacementRegistry::canSpawnAtLocation(
 *     PlacementType::OnGround, world, pos, "minecraft:pig"
 * );
 * @endcode
 */
class EntitySpawnPlacementRegistry {
public:
    /**
     * @brief 放置谓词函数类型
     *
     * 检查指定位置是否可以生成特定实体类型。
     * @param world 世界读取器
     * @param pos 生成位置
     * @param entityTypeId 实体类型（用于类型检查）
     * @param random 随机数生成器（用于需要概率判断的生成条件）
     * @param reason 生成原因（用于区分刷怪笼、自然生成等不同来源）
     * @return 是否可以生成
     */
    using PlacementPredicate = std::function<bool(const ISpawnWorldReader& world,
        const Vector3i& pos,
        const std::string& entityTypeId,
        math::IRandom& random,
        SpawnReason reason)>;

    /**
     * @brief 放置条目
     *
     * 存储单个实体类型的生成规则。
     */
    struct PlacementEntry {
        PlacementType placementType;
        HeightmapType heightmapType;
        PlacementPredicate predicate;
    };

    /**
     * @brief 注册实体放置规则
     *
     * @param entityTypeId 实体类型ID
     * @param placementType 放置类型
     * @param heightmapType 高度图类型
     * @param predicate 额外的放置条件检查函数（可选）
     */
    static void registerPlacement(const std::string& entityTypeId,
        PlacementType placementType,
        HeightmapType heightmapType,
        PlacementPredicate predicate);

    /**
     * @brief 获取实体放置类型
     *
     * @param entityTypeId 实体类型ID
     * @return 放置类型，如果未注册返回 NoRestrictions
     */
    [[nodiscard]] static PlacementType getPlacementType(const std::string& entityTypeId);

    /**
     * @brief 获取实体高度图类型
     *
     * @param entityTypeId 实体类型ID
     * @return 高度图类型，如果未注册返回 MotionBlockingNoLeaves
     */
    [[nodiscard]] static HeightmapType getHeightmapType(const std::string& entityTypeId);

    /**
     * @brief 获取实体放置条目
     *
     * @param entityTypeId 实体类型ID
     * @return 放置条目指针，如果未注册返回 nullptr
     */
    [[nodiscard]] static const PlacementEntry* getPlacementEntry(const std::string& entityTypeId);

    /**
     * @brief 检查位置是否可以生成指定类型的实体
     *
     * 这是主要的放置检查函数，会根据放置类型执行相应的检查。
     *
     * @param placementType 放置类型
     * @param world 世界读取器
     * @param pos 生成位置
     * @param entityTypeId 实体类型ID
     * @return 是否可以生成
     */
    [[nodiscard]] static bool canSpawnAtLocation(PlacementType placementType,
        const ISpawnWorldReader& world,
        const Vector3i& pos,
        const std::string& entityTypeId);

    /**
     * @brief 检查实体是否可以在指定位置生成
     *
     * 包含放置类型检查和自定义谓词检查。
     *
     * @param entityTypeId 实体类型ID
     * @param world 世界读取器
     * @param reason 生成原因
     * @param pos 生成位置
     * @param random 随机数生成器
     * @return 是否可以生成
     */
    [[nodiscard]] static bool canSpawnEntity(const std::string& entityTypeId,
        const ISpawnWorldReader& world,
        SpawnReason reason,
        const Vector3i& pos,
        math::IRandom& random);

    /** @brief 独立运行注册的生成谓词，不检查放置位置。 */
    [[nodiscard]] static bool checkSpawnRules(const std::string& entityTypeId,
        const ISpawnWorldReader& world,
        SpawnReason reason,
        const Vector3i& pos,
        math::IRandom& random);

    /**
     * @brief 检查地面生成条件
     *
     * 检查实体是否可以在地面上生成。
     * 需要脚下有固体方块，且上方有足够空间。
     *
     * @param world 世界读取器
     * @param pos 生成位置
     * @param entityTypeId 实体类型ID
     * @return 是否可以生成
     */
    [[nodiscard]] static bool checkOnGroundSpawn(
        const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId);

    /**
     * @brief 检查水中生成条件
     *
     * 检查实体是否可以在水中生成。
     * 当前位置具有水流体，上方方块不导红石。
     *
     * @param world 世界读取器
     * @param pos 生成位置
     * @param entityTypeId 实体类型ID
     * @return 是否可以生成
     */
    [[nodiscard]] static bool checkInWaterSpawn(
        const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId);

    /**
     * @brief 检查岩浆中生成条件
     *
     * @param world 世界读取器
     * @param pos 生成位置
     * @param entityTypeId 实体类型ID
     * @return 是否可以生成
     */
    [[nodiscard]] static bool checkInLavaSpawn(
        const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId);

    /**
     * @brief 初始化默认的实体放置规则
     *
     * 注册所有原版实体的生成规则。
     */
    static void initializeDefaults();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized();

private:
    /// 放置规则注册表
    static std::unordered_map<std::string, PlacementEntry> s_registry;

    /// 是否已初始化
    static bool s_initialized;

    /**
     * @brief 检查方块状态是否允许生成
     *
     * @param world 世界读取器
     * @param pos 位置
     * @param entityTypeId 实体类型ID
     * @return 是否允许生成
     */
    [[nodiscard]] static bool _isValidSpawnBlock(
        const ISpawnWorldReader& world, const Vector3i& pos, const std::string& entityTypeId);
};

} // namespace world::spawn
} // namespace mc
