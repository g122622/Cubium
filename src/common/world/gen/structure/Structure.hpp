#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../biome/Biome.hpp"
#include "StructureBoundingBox.hpp"
#include <string>
#include <vector>
#include <memory>

namespace mc {

// 前向声明
class IWorld;
class IWorldWriter;
class ChunkPrimer;
class IChunkGenerator;

namespace world::gen {
    class StructureBoundingBox;
}

namespace world::gen::structure {

/**
 * @brief 结构类型枚举
 */
enum class StructureType : u8 {
    Temple,         ///< 神殿/神庙结构（沙漠神殿、丛林神庙等）
    Monument,       ///< 海洋纪念碑
    Stronghold,     ///< 要塞
    Village,        ///< 村庄
    Mineshaft,      ///< 废弃矿井
    RuinedPortal,   ///< 废弃传送门
    BuriedTreasure, ///< 埋藏宝藏
    Shipwreck,      ///< 沉船
    OceanRuin,      ///< 海洋废墟
    WoodlandMansion,///< 林地府邸
    Bastion,        ///< 堡垒遗迹
    Fortress,       ///< 下界要塞
    EndCity         ///< 末地城
};

/**
 * @brief 结构间距设置
 */
struct StructureSeparationSettings {
    i32 spacing;      ///< 平均间距（区块）
    i32 separation;   ///< 最小间距（区块）
    i32 salt;         ///< 随机种子盐

    constexpr StructureSeparationSettings(i32 s = 1, i32 sep = 0, i32 st = 0)
        : spacing(s), separation(sep), salt(st) {}
};

/**
 * @brief 结构片段基类
 */
class StructurePiece {
public:
    StructurePiece(i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ);
    virtual ~StructurePiece() = default;

    [[nodiscard]] i32 type() const { return m_type; }
    [[nodiscard]] i32 minX() const { return m_minX; }
    [[nodiscard]] i32 minY() const { return m_minY; }
    [[nodiscard]] i32 minZ() const { return m_minZ; }
    [[nodiscard]] i32 maxX() const { return m_maxX; }
    [[nodiscard]] i32 maxY() const { return m_maxY; }
    [[nodiscard]] i32 maxZ() const { return m_maxZ; }

    [[nodiscard]] bool intersectsChunk(i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 在区块中生成片段
     * @param world 世界写入器
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param chunkBounds 区块边界框
     */
    virtual void generate(IWorldWriter& world, math::Random& rng,
                          i32 chunkX, i32 chunkZ,
                          const StructureBoundingBox& chunkBounds) = 0;

protected:
    i32 m_type;
    i32 m_minX, m_minY, m_minZ;
    i32 m_maxX, m_maxY, m_maxZ;
};

/**
 * @brief 结构实例
 */
class StructureStart {
public:
    StructureStart(i32 chunkX, i32 chunkZ);
    ~StructureStart() = default;

    void addPiece(std::unique_ptr<StructurePiece> piece);
    [[nodiscard]] const std::vector<std::unique_ptr<StructurePiece>>& pieces() const { return m_pieces; }
    [[nodiscard]] size_t pieceCount() const { return m_pieces.size(); }
    [[nodiscard]] bool isValid() const { return !m_pieces.empty(); }

    [[nodiscard]] i32 chunkX() const { return m_chunkX; }
    [[nodiscard]] i32 chunkZ() const { return m_chunkZ; }

private:
    std::vector<std::unique_ptr<StructurePiece>> m_pieces;
    i32 m_chunkX;
    i32 m_chunkZ;
};

/**
 * @brief 结构基类
 *
 * 所有世界结构的基类。
 */
class Structure {
public:
    virtual ~Structure() = default;

    [[nodiscard]] virtual const String& name() const = 0;
    [[nodiscard]] virtual StructureSeparationSettings separationSettings() const = 0;
    [[nodiscard]] virtual const std::vector<BiomeId>& validBiomes() const = 0;

    [[nodiscard]] StructureType structureType() const { return m_type; }
    [[nodiscard]] bool isValidBiome(BiomeId biomeId) const;

    /**
     * @brief 检查是否可以在指定位置生成结构
     * @param world 世界引用
     * @param generator 区块生成器
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 是否可以生成
     */
    [[nodiscard]] virtual bool canGenerate(
        IWorld& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ);

    /**
     * @brief 生成结构
     * @param world 世界写入器
     * @param generator 区块生成器
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 生成的结构实例，如果无法生成则返回 nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<StructureStart> generate(
        IWorldWriter& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) const;

    /**
     * @brief 在区块中放置结构片段
     * @param world 世界写入器
     * @param chunk 区块
     * @param start 结构起点
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     */
    virtual void placeInChunk(
        IWorldWriter& world,
        ChunkPrimer& chunk,
        StructureStart& start,
        i32 chunkX,
        i32 chunkZ) const;

    [[nodiscard]] static bool findStructureStart(
        i64 seed, i32 chunkX, i32 chunkZ,
        const StructureSeparationSettings& settings,
        i32& outStartX, i32& outStartZ);

protected:
    explicit Structure(StructureType type) : m_type(type) {}

    [[nodiscard]] static math::Random createRandom(i64 seed, i32 chunkX, i32 chunkZ, i32 salt);

    StructureType m_type;
};

// 片段类型常量
namespace StructurePieceTypes {
    constexpr i32 RUINED_PORTAL = 50;
    constexpr i32 BURIED_TREASURE = 53;
    // 废弃矿井片段
    constexpr i32 MINESHAFT_ROOM = 60;
    constexpr i32 MINESHAFT_CORRIDOR = 61;
    constexpr i32 MINESHAFT_CROSS = 62;
    constexpr i32 MINESHAFT_STAIRS = 63;
}

} // namespace mc::world::gen::structure
} // namespace mc
