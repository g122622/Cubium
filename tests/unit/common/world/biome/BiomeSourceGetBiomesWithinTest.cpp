#include <gtest/gtest.h>

#include "common/world/biome/BiomeSource.hpp"

#include <set>
#include <tuple>
#include <vector>

namespace {

using mc::BiomeId;
using mc::i32;
using mc::world::biome::IBiomeSource;

/**
 * @brief 记录被查询过的 quart 坐标的桩群系源
 *
 * 用于验证 `IBiomeSource::getBiomesWithin` 的采样步长：它应当按 **4 格（一个 quart）**
 * 遍历方框内的每个 quart，而不是按区块（16 格）粗略跳点。
 */
class RecordingBiomeSource final : public IBiomeSource {
public:
    explicit RecordingBiomeSource(BiomeId biome)
        : IBiomeSource(0)
        , m_biome(biome)
    {}

    [[nodiscard]] BiomeId getNoiseBiome(i32 quartX, i32 /*quartY*/, i32 quartZ) const override
    {
        m_queried.emplace(quartX, quartZ);
        return m_biome;
    }

    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override
    {
        static const std::vector<BiomeId> kBiomes{m_biome};
        return kBiomes;
    }

    [[nodiscard]] const std::set<std::pair<i32, i32>>& queried() const noexcept
    {
        return m_queried;
    }

private:
    BiomeId m_biome;
    // mutable：getNoiseBiome 是 const 接口
    mutable std::set<std::pair<i32, i32>> m_queried;
};

} // namespace

// 半径 29 的方框把 x 从 -29 覆盖到 +29，除以 4 后是 quart -8..7，共 16 个 quart —— 每条轴。
// 若按 16 格步长近似，只会有 -1/0/1 三个采样点，这里断言实际覆盖了整段 quart 区间。
TEST(BiomeSourceGetBiomesWithinTest, SamplesEveryQuartInTheBox)
{
    RecordingBiomeSource source(1);

    const std::unordered_set<BiomeId> biomes = source.getBiomesWithin(0, 64, 0, 29);

    EXPECT_EQ(biomes.size(), 1u) << "a fixed source must yield exactly one biome";

    const auto& queried = source.queried();
    ASSERT_FALSE(queried.empty());

    // 收集实际访问到的 quart 范围
    i32 minQx = 0;
    i32 maxQx = 0;
    i32 minQz = 0;
    i32 maxQz = 0;
    bool first = true;
    for (const auto& [qx, qz] : queried) {
        if (first) {
            minQx = maxQx = qx;
            minQz = maxQz = qz;
            first = false;
        } else {
            minQx = std::min(minQx, qx);
            maxQx = std::max(maxQx, qx);
            minQz = std::min(minQz, qz);
            maxQz = std::max(maxQz, qz);
        }
    }

    // floorDiv(-29, 4) = -8，floorDiv(29, 4) = 7
    EXPECT_EQ(minQx, -8) << "the sampling box must start at quart -8";
    EXPECT_EQ(maxQx, 7) << "the sampling box must end at quart 7";
    EXPECT_EQ(minQz, -8);
    EXPECT_EQ(maxQz, 7);

    // 覆盖必须是每个 quart 都被访问（16x16 网格，y 只取一层）
    EXPECT_EQ(queried.size(), static_cast<size_t>(16 * 16))
        << "every quart in the box must be sampled, not just a 16-block stride";
}

// 半径为 0 时只应访问中心所在的单个 quart。
TEST(BiomeSourceGetBiomesWithinTest, ZeroRadiusSamplesSingleQuart)
{
    RecordingBiomeSource source(1);
    source.getBiomesWithin(10, 64, 10, 0);

    const auto& queried = source.queried();
    ASSERT_EQ(queried.size(), 1u);
    // floorDiv(10, 4) = 2
    EXPECT_EQ(queried.begin()->first, 2);
    EXPECT_EQ(queried.begin()->second, 2);
}
