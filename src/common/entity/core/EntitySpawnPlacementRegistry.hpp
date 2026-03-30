#pragma once

#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../world/chunk/IChunk.hpp"  // 包含 HeightmapType
#include <functional>
#include <unordered_map>

namespace mc {

// 前向声明
class BlockState;
class Block;

namespace entity {
    class EntityType;
    enum class EntityClassification : u8;
}

namespace world::spawn {

/**
 * @brief 生成原因枚举
 *
 * 定义实体生成的各种原因，用于决定生成规则。
 *
 * 参考 MC 1.16.5 SpawnReason
 */
enum class SpawnReason : u8 {
    Natural,            // 自然生成
    ChunkGeneration,    // 区块生成时放置
    SpawnEgg,           // 生成蛋
    Spawner,            // 刷怪笼
    Structure,          // 结构生成（如村民）
    Breeding,           // 繁殖
    Jockey,             // 骑乘生成
    MobSummons,         // 召唤
    Conversion,         // 转化（如僵尸村民）
    Reinforcement,      // 增援（如僵尸召唤其他僵尸）
    Trigger,            // 触发器
    Bucket,             // 水桶释放
    Dispenser,          // 发射器
    Event               // 事件（如袭击）
};

/**
 * @brief 实体生成放置类型
 *
 * 定义实体生成时需要的环境条件类型。
 *
 * 参考 MC 1.16.5 EntitySpawnPlacementRegistry.PlacementType
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
     * @brief 检查位置是否在世界范围内
     */
    [[nodiscard]] virtual bool isInWorldBounds(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取高度图值
     */
    [[nodiscard]] virtual i32 getHeight(HeightmapType type, i32 x, i32 z) const = 0;

    /**
     * @brief 获取指定位置的生物群系ID
     */
    [[nodiscard]] virtual BiomeId getBiome(i32 x, i32 y, i32 z) const = 0;
};

/**
 * @brief 实体生成放置注册表
 *
 * 管理每种实体类型的生成规则和放置条件。
 * 在区块生成和自然生成时用于验证生成位置。
 *
 * 参考 MC 1.16.5 EntitySpawnPlacementRegistry
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
     * @param entityType 实体类型（用于类型检查）
     * @return 是否可以生成
     */
    using PlacementPredicate = std::function<bool(
        const ISpawnWorldReader& world,
        const Vector3i& pos,
        const String& entityTypeId
    )>;

    /**
     * @brief 放置条目
     *
     * 存储单个实体类型的生成规则。
     */
    struct PlacementEntry {
        /// 放置类型
        PlacementType placementType = PlacementType::OnGround;

        /// 高度图类型（用于获取生成高度）
        HeightmapType heightmapType = HeightmapType::MotionBlockingNoLeaves;

        /// 额外的放置条件检查函数
        PlacementPredicate predicate;

        PlacementEntry() = default;

        PlacementEntry(PlacementType type, HeightmapType heightmap, PlacementPredicate pred = nullptr)
            : placementType(type)
            , heightmapType(heightmap)
            , predicate(std::move(pred))
        {}
    };

    /**
     * @brief 注册实体放置规则
     *
     * @param entityTypeId 实体类型ID
     * @param placementType 放置类型
     * @param heightmapType 高度图类型
     * @param predicate 额外的放置条件检查函数（可选）
     */
    static void registerPlacement(
        const String& entityTypeId,
        PlacementType placementType,
        HeightmapType heightmapType,
        PlacementPredicate predicate = nullptr
    );

    /**
     * @brief 获取实体放置类型
     *
     * @param entityTypeId 实体类型ID
     * @return 放置类型，如果未注册返回 NoRestrictions
     */
    [[nodiscard]] static PlacementType getPlacementType(const String& entityTypeId);

    /**
     * @brief 获取实体高度图类型
     *
     * @param entityTypeId 实体类型ID
     * @return 高度图类型，如果未注册返回 MotionBlockingNoLeaves
     */
    [[nodiscard]] static HeightmapType getHeightmapType(const String& entityTypeId);

    /**
     * @brief 获取实体放置条目
     *
     * @param entityTypeId 实体类型ID
     * @return 放置条目指针，如果未注册返回 nullptr
     */
    [[nodiscard]] static const PlacementEntry* getPlacementEntry(const String& entityTypeId);

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
     *
     * 参考 MC 1.16.5 canCreatureTypeSpawnAtLocation
     */
    [[nodiscard]] static bool canSpawnAtLocation(
        PlacementType placementType,
        const ISpawnWorldReader& world,
        const Vector3i& pos,
        const String& entityTypeId
    );

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
     *
     * 参考 MC 1.16.5 EntitySpawnPlacementRegistry.canSpawnEntity
     */
    [[nodiscard]] static bool canSpawnEntity(
        const String& entityTypeId,
        ISpawnWorldReader& world,
        SpawnReason reason,
        const Vector3i& pos,
        math::Random& random
    );

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
        const ISpawnWorldReader& world,
        const Vector3i& pos,
        const String& entityTypeId
    );

    /**
     * @brief 检查水中生成条件
     *
     * 检查实体是否可以在水中生成。
     * 需要当前位置和下方位置都有水。
     *
     * @param world 世界读取器
     * @param pos 生成位置
     * @param entityTypeId 实体类型ID
     * @return 是否可以生成
     */
    [[nodiscard]] static bool checkInWaterSpawn(
        const ISpawnWorldReader& world,
        const Vector3i& pos,
        const String& entityTypeId
    );

    /**
     * @brief 检查岩浆中生成条件
     *
     * @param world 世界读取器
     * @param pos 生成位置
     * @param entityTypeId 实体类型ID
     * @return 是否可以生成
     */
    [[nodiscard]] static bool checkInLavaSpawn(
        const ISpawnWorldReader& world,
        const Vector3i& pos,
        const String& entityTypeId
    );

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
    static std::unordered_map<String, PlacementEntry> s_registry;

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
    [[nodiscard]] static bool isValidSpawnBlock(
        const ISpawnWorldReader& world,
        const Vector3i& pos,
        const String& entityTypeId
    );

    /**
     * @brief 检查方块是否阻止生成
     *
     * 某些方块（如红石、障碍物）会阻止生成
     *
     * @param state 方块状态
     * @return 是否阻止生成
     */
    [[nodiscard]] static bool blockPreventsSpawn(const BlockState* state);
};

} // namespace world::spawn
} // namespace mc
