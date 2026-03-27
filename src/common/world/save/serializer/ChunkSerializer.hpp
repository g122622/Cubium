#pragma once

#include "../data/LevelData.hpp"
#include "../../core/Types.hpp"
#include "../../core/Result.hpp"
#include "../../util/nbt/Nbt.hpp"
#include "../../world/chunk/ChunkData.hpp"
#include "../../world/time/GameTime.hpp"
#include "../../world/weather/WeatherState.hpp"
#include <memory>

namespace mc::world::save::serializer {

/**
 * @brief 区块序列化/反序列化器
 *
 * 负责将 ChunkData 与 NBT 格式互转。
 * 参考 MC 1.16.5 ChunkSerializer.java
 *
 * ## NBT 结构
 *
 * ```
 * CompoundTag (root)
 * ├── DataVersion: Int
 * └── Level: CompoundTag
 *     ├── xPos: Int
 *     ├── zPos: Int
 *     ├── LastUpdate: Long
 *     ├── InhabitedTime: Long
 *     ├── Status: String
 *     ├── Sections: ListTag
 *     ├── Biomes: IntArrayTag
 *     ├── Heightmaps: CompoundTag
 *     ├── Entities: ListTag
 *     ├── TileEntities: ListTag
 *     ├── TileTicks: ListTag
 *     ├── LiquidTicks: ListTag
 *     └── Structures: CompoundTag
 * ```
 *
 * ## 使用示例
 * ```cpp
 * // 序列化区块
 * auto nbt = ChunkSerializer::serialize(*chunkData, gameTime);
 *
 * // 反序列化区块
 * auto result = ChunkSerializer::deserialize(*nbt);
 * if (result.success()) {
 *     auto chunk = std::move(result.value());
 * }
 * ```
 */
class ChunkSerializer {
public:
    /// 当前支持的数据版本（MC 1.16.5 = 2586）
    static constexpr i32 DATA_VERSION = 2586;

    /**
     * @brief 序列化区块到 NBT
     *
     * @param chunk 区块数据
     * @param gameTime 当前游戏时间（ticks）
     * @return NBT 复合标签
     */
    [[nodiscard]] static std::unique_ptr<nbt::CompoundTag>
    serialize(const ChunkData& chunk, i64 gameTime);

    /**
     * @brief 从 NBT 反序列化区块
     *
     * @param nbt NBT 数据
     * @return 成功返回区块数据，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<ChunkData>>
    deserialize(const nbt::CompoundTag& nbt);

    /**
     * @brief 序列化区块段
     *
     * @param section 区块段
     * @param sectionY 段的 Y 坐标
     * @return NBT 复合标签
     */
    [[nodiscard]] static std::unique_ptr<nbt::CompoundTag>
    serializeSection(const ChunkSection& section, i32 sectionY);

    /**
     * @brief 反序列化区块段
     *
     * @param nbt NBT 数据
     * @return 成功返回区块段，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<ChunkSection>>
    deserializeSection(const nbt::CompoundTag& nbt);

private:
    // ========== 序列化辅助方法 ==========

    /**
     * @brief 序列化方块段数据
     *
     * 使用调色板压缩存储方块状态。
     */
    static void serializeSections(nbt::CompoundTag& level, const ChunkData& chunk);

    /**
     * @brief 反序列化方块段数据
     */
    static Result<void> deserializeSections(const nbt::CompoundTag& level, ChunkData& chunk);

    /**
     * @brief 序列化生物群系数据
     */
    static void serializeBiomes(nbt::CompoundTag& level, const ChunkData& chunk);

    /**
     * @brief 反序列化生物群系数据
     */
    static Result<void> deserializeBiomes(const nbt::CompoundTag& level, ChunkData& chunk);

    /**
     * @brief 序列化高度图
     */
    static void serializeHeightmaps(nbt::CompoundTag& level, const ChunkData& chunk);

    /**
     * @brief 反序列化高度图
     */
    static Result<void> deserializeHeightmaps(const nbt::CompoundTag& level, ChunkData& chunk);

    /**
     * @brief 序列化光照数据
     */
    static void serializeLight(nbt::CompoundTag& section, const ChunkSection& chunkSection);

    /**
     * @brief 反序列化光照数据
     */
    static Result<void> deserializeLight(const nbt::CompoundTag& section, ChunkSection& chunkSection);

    /**
     * @brief 序列化方块实体
     */
    static void serializeBlockEntities(nbt::CompoundTag& level, const ChunkData& chunk);

    /**
     * @brief 反序列化方块实体
     */
    static Result<void> deserializeBlockEntities(const nbt::CompoundTag& level, ChunkData& chunk);

    /**
     * @brief 序列化实体
     */
    static void serializeEntities(nbt::CompoundTag& level, const ChunkData& chunk);

    /**
     * @brief 反序列化实体
     */
    static Result<void> deserializeEntities(const nbt::CompoundTag& level, ChunkData& chunk);

    /**
     * @brief 序列化计划刻
     */
    static void serializeTicks(nbt::CompoundTag& level, const ChunkData& chunk);

    /**
     * @brief 反序列化计划刻
     */
    static Result<void> deserializeTicks(const nbt::CompoundTag& level, ChunkData& chunk);

    /**
     * @brief 将区块状态转换为字符串 ID
     *
     * @param status 区块状态
     * @return 状态字符串
     */
    [[nodiscard]] static const char* chunkStatusToString(ChunkLoadStatus status);

    /**
     * @brief 从字符串解析区块状态
     *
     * @param status 状态字符串
     * @return 区块状态
     */
    [[nodiscard]] static ChunkLoadStatus parseChunkStatus(const String& status);
};

/**
 * @brief 世界元数据序列化器
 *
 * 负责将 LevelData 与 level.dat 文件互转。
 */
class LevelDataSerializer {
public:
    /**
     * @brief 序列化 LevelData 到文件
     *
     * @param path 文件路径
     * @param levelData 世界元数据
     * @param playerNbt 可选的玩家数据（单人模式）
     * @return 成功返回 void，失败返回错误
     */
    static Result<void>
    save(const std::filesystem::path& path,
         const data::LevelData& levelData,
         const nbt::CompoundTag* playerNbt = nullptr);

    /**
     * @brief 从文件加载 LevelData
     *
     * @param path 文件路径
     * @return 成功返回 LevelData，失败返回错误
     */
    static Result<std::unique_ptr<data::LevelData>>
    load(const std::filesystem::path& path);

    /**
     * @brief 序列化到压缩的字节流
     *
     * @param levelData 世界元数据
     * @param playerNbt 可选的玩家数据
     * @return 压缩的 NBT 数据
     */
    [[nodiscard]] static Result<std::vector<u8>>
    serializeToBytes(const data::LevelData& levelData,
                     const nbt::CompoundTag* playerNbt = nullptr);

    /**
     * @brief 从字节流反序列化
     *
     * @param data 压缩的 NBT 数据
     * @param size 数据大小
     * @return 成功返回 LevelData，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<data::LevelData>>
    deserializeFromBytes(const u8* data, size_t size);
};

/**
 * @brief 玩家数据序列化器
 *
 * 负责将 PlayerData 与 playerdata/<uuid>.dat 文件互转。
 */
class PlayerDataSerializer {
public:
    /**
     * @brief 序列化 PlayerData 到文件
     *
     * @param path 文件路径
     * @param playerData 玩家数据
     * @return 成功返回 void，失败返回错误
     */
    static Result<void>
    save(const std::filesystem::path& path, const data::PlayerData& playerData);

    /**
     * @brief 从文件加载 PlayerData
     *
     * @param path 文件路径
     * @return 成功返回 PlayerData，失败返回错误
     */
    static Result<std::unique_ptr<data::PlayerData>>
    load(const std::filesystem::path& path);

    /**
     * @brief 序列化到压缩的字节流
     *
     * @param playerData 玩家数据
     * @return 压缩的 NBT 数据
     */
    [[nodiscard]] static Result<std::vector<u8>>
    serializeToBytes(const data::PlayerData& playerData);

    /**
     * @brief 从字节流反序列化
     *
     * @param data 压缩的 NBT 数据
     * @param size 数据大小
     * @return 成功返回 PlayerData，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<data::PlayerData>>
    deserializeFromBytes(const u8* data, size_t size);
};

} // namespace mc::world::save::serializer
