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

#include "LargeDripstoneFeature.hpp"

#include "common/util/Direction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/Column.hpp"
#include "common/world/gen/feature/DripstoneUtils.hpp"

#include <cmath>
#include <limits>

namespace mc::world::gen::feature::cave {

// ============================================================================
// WindOffsetter — 风偏移器
// ============================================================================

class WindOffsetter {
public:
    WindOffsetter(i32 originY, math::IRandom& random, const valueprovider::FloatProvider& windSpeed)
        : m_originY(originY)
    {
        const f32 magnitude = windSpeed.sample(random);
        const f32 angle = random.nextFloat(0.0f, math::PI);
        m_windX = std::cos(angle) * magnitude;
        m_windZ = std::sin(angle) * magnitude;
    }

    static std::unique_ptr<WindOffsetter> noWind()
    {
        // 直接构造（私有默认构造函数仅类内可见，std::make_unique 无法访问）。
        return std::unique_ptr<WindOffsetter>(new WindOffsetter());
    }

    [[nodiscard]] bool hasWind() const noexcept { return m_windX.has_value(); }

    [[nodiscard]] BlockPos offset(const BlockPos& pos) const
    {
        if (!m_windX.has_value()) {
            return pos;
        }
        const i32 dy = m_originY - pos.y;
        const double dx = *m_windX * dy;
        const double dz = *m_windZ * dy;
        return BlockPos(pos.x + static_cast<i32>(std::floor(dx)), pos.y, pos.z + static_cast<i32>(std::floor(dz)));
    }

private:
    WindOffsetter()
        : m_originY(0)
    {}

    i32 m_originY;
    // nullopt 表示无风（noWind）。MC 用 Vec3 windSpeed=null 表示无风。
    std::optional<double> m_windX;
    std::optional<double> m_windZ;
};

// ============================================================================
// LargeDripstone — 单根钟乳石/石笋
// ============================================================================

class LargeDripstone {
public:
    LargeDripstone(const BlockPos& root, bool pointingUp, i32 radius, double bluntness, double scale)
        : m_root(root)
        , m_pointingUp(pointingUp)
        , m_radius(radius)
        , m_bluntness(bluntness)
        , m_scale(scale)
    {}

    [[nodiscard]] bool isSuitableForWind(const LargeDripstoneConfig& config) const
    {
        return m_radius >= config.minRadiusForWind && m_bluntness >= static_cast<double>(config.minBluntnessForWind);
    }

    [[nodiscard]] bool moveBackUntilBaseIsInsideStoneAndShrinkRadiusIfNecessary(
        IWorld& world, const WindOffsetter& offsetter)
    {
        while (m_radius > 1) {
            BlockPosMutable cursor(m_root);
            const i32 limit = std::min(10, getHeight());

            for (i32 j = 0; j < limit; ++j) {
                const BlockState* state = world.getBlockState(cursor);
                if (state != nullptr && state->is(VanillaBlocks::LAVA)) {
                    return false;
                }
                if (DripstoneUtils::isCircleMostlyEmbeddedInStone(world, offsetter.offset(cursor), m_radius)) {
                    m_root = cursor;
                    return true;
                }
                cursor.move(m_pointingUp ? Direction::Down : Direction::Up);
            }

            m_radius /= 2;
        }
        return false;
    }

    void placeBlocks(IWorld& world, math::Random& random, const WindOffsetter& offsetter)
    {
        for (i32 i = -m_radius; i <= m_radius; ++i) {
            for (i32 j = -m_radius; j <= m_radius; ++j) {
                const f32 dist = static_cast<f32>(std::sqrt(static_cast<double>(i * i + j * j)));
                if (!(dist > static_cast<f32>(m_radius))) {
                    i32 k = getHeightAtRadius(dist);
                    if (k > 0) {
                        if (random.nextFloat() < 0.2F) {
                            k = static_cast<i32>(static_cast<f32>(k) * random.nextFloat(0.8F, 1.0F));
                        }

                        BlockPosMutable cursor(BlockPos(m_root.x + i, m_root.y, m_root.z + j));
                        bool placedAny = false;
                        const i32 heightCap =
                            m_pointingUp ? world.getHeight(cursor.x, cursor.z) : std::numeric_limits<i32>::max();

                        for (i32 i1 = 0; i1 < k && cursor.y < heightCap; ++i1) {
                            const BlockPos pos = offsetter.offset(cursor);
                            if (DripstoneUtils::isEmptyOrWaterOrLava(world, pos)) {
                                placedAny = true;
                                world.setBlockState(
                                    pos.x, pos.y, pos.z, &VanillaBlocks::DRIPSTONE_BLOCK->defaultState());
                            } else {
                                const BlockState* state = world.getBlockState(pos);
                                if (placedAny && state != nullptr &&
                                    BlockTags::BASE_STONE_OVERWORLD().contains(*state)) {
                                    break;
                                }
                            }
                            cursor.move(m_pointingUp ? Direction::Up : Direction::Down);
                        }
                    }
                }
            }
        }
    }

private:
    [[nodiscard]] i32 getHeight() const { return getHeightAtRadius(0.0F); }

    [[nodiscard]] i32 getHeightAtRadius(f32 radius) const
    {
        return static_cast<i32>(DripstoneUtils::getDripstoneHeight(radius, m_radius, m_scale, m_bluntness));
    }

    BlockPos m_root;
    bool m_pointingUp;
    i32 m_radius;
    double m_bluntness;
    double m_scale;
};

// ============================================================================
// LargeDripstoneConfig
// ============================================================================

LargeDripstoneConfig::LargeDripstoneConfig(i32 searchRange,
    std::unique_ptr<valueprovider::IntProvider> radius,
    std::unique_ptr<valueprovider::FloatProvider> hScale,
    f32 maxRatio,
    std::unique_ptr<valueprovider::FloatProvider> stalactiteBlunt,
    std::unique_ptr<valueprovider::FloatProvider> stalagmiteBlunt,
    std::unique_ptr<valueprovider::FloatProvider> wind,
    i32 minRadiusWind,
    f32 minBluntWind)
    : floorToCeilingSearchRange(searchRange)
    , columnRadius(std::move(radius))
    , heightScale(std::move(hScale))
    , maxColumnRadiusToCaveHeightRatio(maxRatio)
    , stalactiteBluntness(std::move(stalactiteBlunt))
    , stalagmiteBluntness(std::move(stalagmiteBlunt))
    , windSpeed(std::move(wind))
    , minRadiusForWind(minRadiusWind)
    , minBluntnessForWind(minBluntWind)
{}

// ============================================================================
// ConfiguredLargeDripstoneFeature
// ============================================================================

ConfiguredLargeDripstoneFeature::ConfiguredLargeDripstoneFeature(
    std::unique_ptr<LargeDripstoneConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredLargeDripstoneFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& pos) const
{
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// LargeDripstoneFeature::place
// ============================================================================

namespace {

LargeDripstone makeDripstone(const BlockPos& root,
    bool pointingUp,
    math::Random& random,
    i32 radius,
    const valueprovider::FloatProvider& blunt,
    const valueprovider::FloatProvider& scale)
{
    return LargeDripstone(root, pointingUp, radius, blunt.sample(random), scale.sample(random));
}

} // namespace

bool LargeDripstoneFeature::place(
    IWorld& world, math::Random& random, const BlockPos& pos, const LargeDripstoneConfig& config)
{
    if (!DripstoneUtils::isEmptyOrWater(world, pos)) {
        return false;
    }

    auto optional = Column::scan(world,
        pos,
        config.floorToCeilingSearchRange,
        static_cast<bool (*)(const BlockState*)>(DripstoneUtils::isEmptyOrWater),
        static_cast<bool (*)(const BlockState*)>(DripstoneUtils::isDripstoneBaseOrLava));
    if (!optional.has_value()) {
        return false;
    }

    auto& column = **optional;
    auto* range = dynamic_cast<Column::Range*>(&column);
    if (range == nullptr) {
        return false;
    }
    if (range->height() < 4) {
        return false;
    }

    const i32 maxRadius =
        static_cast<i32>(static_cast<double>(range->height()) * config.maxColumnRadiusToCaveHeightRatio);
    const i32 clampedMax =
        math::clamp(maxRadius, config.columnRadius->getMinValue(), config.columnRadius->getMaxValue());
    const i32 radius = random.nextInt(config.columnRadius->getMinValue(), clampedMax);

    LargeDripstone stalactite = makeDripstone(BlockPos(pos.x, range->ceiling() - 1, pos.z),
        false,
        random,
        radius,
        *config.stalactiteBluntness,
        *config.heightScale);
    LargeDripstone stalagmite = makeDripstone(BlockPos(pos.x, range->floor() + 1, pos.z),
        true,
        random,
        radius,
        *config.stalagmiteBluntness,
        *config.heightScale);

    std::unique_ptr<WindOffsetter> offsetter;
    if (stalactite.isSuitableForWind(config) && stalagmite.isSuitableForWind(config)) {
        offsetter = std::make_unique<WindOffsetter>(pos.y, random, *config.windSpeed);
    } else {
        offsetter = WindOffsetter::noWind();
    }

    const bool placeStalactite = stalactite.moveBackUntilBaseIsInsideStoneAndShrinkRadiusIfNecessary(world, *offsetter);
    const bool placeStalagmite = stalagmite.moveBackUntilBaseIsInsideStoneAndShrinkRadiusIfNecessary(world, *offsetter);

    if (placeStalactite) {
        stalactite.placeBlocks(world, random, *offsetter);
    }
    if (placeStalagmite) {
        stalagmite.placeBlocks(world, random, *offsetter);
    }

    return true;
}

} // namespace mc::world::gen::feature::cave
