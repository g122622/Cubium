#pragma once

#include "../Structure.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include <vector>
#include <memory>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 要塞结构
 *
 * 要塞是生成在地下的大型结构，包含末地传送门。
 * 参考 MC 1.16.5: StrongholdStructure
 *
 * 特点：
 * - 生成于地下，Y 坐标通常在 20-40
 * - 包含多个房间：图书馆、监狱、传送门房间等
 * - 有复杂的走廊连接系统
 * - 每个世界最多 128 个要塞
 */
class StrongholdStructure : public Structure {
public:
    /**
     * @brief 要塞配置
     */
    struct Config {
        i32 distance = 32;          ///< 距离（环之间的距离）
        i32 spread = 3;             ///< 扩散角度
        i32 count = 128;            ///< 最大要塞数量
        i32 minY = 20;              ///< 最低 Y 坐标
        i32 maxY = 40;              ///< 最高 Y 坐标
    };

    StrongholdStructure();
    explicit StrongholdStructure(const Config& config);

    [[nodiscard]] const String& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) override;

    /**
     * @brief 生成要塞
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) const override;

    /**
     * @brief 计算要塞位置
     * @param index 要塞索引 (0-127)
     * @param worldSeed 世界种子
     * @return 要塞起始区块坐标
     */
    [[nodiscard]] static std::pair<i32, i32> calculateStrongholdPos(i32 index, i64 worldSeed);

    /**
     * @brief 计算要塞所在环
     * @param index 要塞索引
     * @return 环索引 (0-7)
     */
    [[nodiscard]] static i32 getRing(i32 index);

private:
    void initializeBiomes();
    void generateFallbackEntrance(IWorldWriter& world, math::Random& rng, const BlockPos& startPos) const;

    Config m_config;
    static constexpr StructureSeparationSettings m_settings{32, 8, 1429134543};
    static const String m_name;
    std::vector<BiomeId> m_validBiomes;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
