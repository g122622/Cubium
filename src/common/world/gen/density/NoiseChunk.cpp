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
 */

#include "common/world/gen/density/NoiseChunk.hpp"
#include "common/core/Constants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/gen/aquifer/Aquifer.hpp"
#include <algorithm>
#include <limits>

namespace mc::world::gen::density {

namespace {

[[nodiscard]] i64 packXZ(i32 x, i32 z)
{
    return static_cast<i64>((static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z));
}

class DensityFunctionReference final : public DensityFunction {
public:
    explicit DensityFunctionReference(const DensityFunction& target)
        : m_target(target)
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        return m_target.compute(blockX, blockY, blockZ);
    }

    [[nodiscard]] f64 minValue() const override { return m_target.minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_target.maxValue(); }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        // DensityFunctionReference 只是持有对目标函数的引用，
        // 不能 clone 目标，应该让 visitor 决定如何处理。
        // 由于它只是转发调用，实际上 visitor 无法替换引用目标。
        // 返回一个新的引用包装器指向同一个目标。
        return visitor.apply(std::make_unique<DensityFunctionReference>(m_target));
    }

private:
    const DensityFunction& m_target;
};

} // namespace

// ============================================================================
// NoiseInterpolator 实现
// ============================================================================

NoiseInterpolator::NoiseInterpolator(std::unique_ptr<DensityFunction> filler, i32 cellCountZ, i32 cellCountY)
    : m_filler(std::move(filler))
    , m_cellCountZ(cellCountZ)
    , m_cellCountY(cellCountY)
{
    const i32 zPoints = cellCountZ + 1;
    const i32 yPoints = cellCountY + 1;

    m_slice0.resize(static_cast<size_t>(zPoints));
    m_slice1.resize(static_cast<size_t>(zPoints));
    for (i32 z = 0; z < zPoints; ++z) {
        m_slice0[static_cast<size_t>(z)].resize(static_cast<size_t>(yPoints), 0.0);
        m_slice1[static_cast<size_t>(z)].resize(static_cast<size_t>(yPoints), 0.0);
    }
}

f64 NoiseInterpolator::compute(i32, i32, i32) const
{
    // 在 NoiseChunk 上下文中，插值结果通过 updateForZ 获取
    // 直接 compute 返回最近一次的插值结果
    return m_value;
}

void NoiseInterpolator::fillSlice(
    bool isSlice0, i32 cellX, i32 firstCellZ, i32 firstCellY, i32 cellWidthXZ, i32 cellHeightY)
{
    const i32 zPoints = m_cellCountZ + 1;
    const i32 yPoints = m_cellCountY + 1;
    const i32 blockX = cellX * cellWidthXZ;

    auto& targetSlice = isSlice0 ? m_slice0 : m_slice1;

    for (i32 z = 0; z < zPoints; ++z) {
        const i32 blockZ = (firstCellZ + z) * cellWidthXZ;
        for (i32 y = 0; y < yPoints; ++y) {
            const i32 blockY = (firstCellY + y) * cellHeightY;
            targetSlice[static_cast<size_t>(z)][static_cast<size_t>(y)] = m_filler->compute(blockX, blockY, blockZ);
        }
    }
}

void NoiseInterpolator::selectCellYZ(i32 cellY, i32 cellZ)
{
    m_noise000 = m_slice0[static_cast<size_t>(cellZ)][static_cast<size_t>(cellY)];
    m_noise010 = m_slice0[static_cast<size_t>(cellZ)][static_cast<size_t>(cellY + 1)];
    m_noise001 = m_slice0[static_cast<size_t>(cellZ + 1)][static_cast<size_t>(cellY)];
    m_noise011 = m_slice0[static_cast<size_t>(cellZ + 1)][static_cast<size_t>(cellY + 1)];

    m_noise100 = m_slice1[static_cast<size_t>(cellZ)][static_cast<size_t>(cellY)];
    m_noise110 = m_slice1[static_cast<size_t>(cellZ)][static_cast<size_t>(cellY + 1)];
    m_noise101 = m_slice1[static_cast<size_t>(cellZ + 1)][static_cast<size_t>(cellY)];
    m_noise111 = m_slice1[static_cast<size_t>(cellZ + 1)][static_cast<size_t>(cellY + 1)];
}

void NoiseInterpolator::updateForY(f64 delta)
{
    m_valueXZ00 = math::lerp(m_noise000, m_noise010, delta);
    m_valueXZ10 = math::lerp(m_noise100, m_noise110, delta);
    m_valueXZ01 = math::lerp(m_noise001, m_noise011, delta);
    m_valueXZ11 = math::lerp(m_noise101, m_noise111, delta);
}

void NoiseInterpolator::updateForX(f64 delta)
{
    m_valueZ0 = math::lerp(m_valueXZ00, m_valueXZ10, delta);
    m_valueZ1 = math::lerp(m_valueXZ01, m_valueXZ11, delta);
}

f64 NoiseInterpolator::updateForZ(f64 delta)
{
    m_value = math::lerp(m_valueZ0, m_valueZ1, delta);
    return m_value;
}

f64 NoiseInterpolator::interpolate(f64 deltaX, f64 deltaY, f64 deltaZ) const
{
    const f64 xz00 = math::lerp(m_noise000, m_noise010, deltaY);
    const f64 xz10 = math::lerp(m_noise100, m_noise110, deltaY);
    const f64 xz01 = math::lerp(m_noise001, m_noise011, deltaY);
    const f64 xz11 = math::lerp(m_noise101, m_noise111, deltaY);

    const f64 z0 = math::lerp(xz00, xz10, deltaX);
    const f64 z1 = math::lerp(xz01, xz11, deltaX);

    return math::lerp(z0, z1, deltaZ);
}

void NoiseInterpolator::swapSlices()
{
    std::swap(m_slice0, m_slice1);
}

// ============================================================================
// CellCache 实现
// ============================================================================

CellCache::CellCache(std::unique_ptr<DensityFunction> filler, i32 cellWidth, i32 cellHeight)
    : m_filler(std::move(filler))
    , m_cellWidth(cellWidth)
    , m_cellHeight(cellHeight)
    , m_values(static_cast<size_t>(cellWidth * cellWidth * cellHeight), 0.0)
{}

f64 CellCache::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    // 如果不在 NoiseChunk 的插值循环中，直接委托给原始函数
    if (!m_filled || m_currentIndex < 0) {
        return m_filler->compute(blockX, blockY, blockZ);
    }
    return m_values[static_cast<size_t>(m_currentIndex)];
}

void CellCache::fillCell(i32 blockX0, i32 blockY0, i32 blockZ0)
{
    // 从底部到顶部遍历 cell 内所有方块位置
    // MC 1.21 的顺序: Y 从高到低, 然后 X, 然后 Z
    i32 idx = 0;
    for (i32 y = m_cellHeight - 1; y >= 0; --y) {
        const i32 blockY = blockY0 + y;
        for (i32 x = 0; x < m_cellWidth; ++x) {
            const i32 bx = blockX0 + x;
            for (i32 z = 0; z < m_cellWidth; ++z) {
                const i32 bz = blockZ0 + z;
                m_values[static_cast<size_t>(idx)] = m_filler->compute(bx, blockY, bz);
                ++idx;
            }
        }
    }
    m_filled = true;
}

void CellCache::setInCellPos(i32 inCellX, i32 inCellY, i32 inCellZ)
{
    // MC 1.21 索引: ((cellHeight - 1 - inCellY) * cellWidth + inCellX) * cellWidth + inCellZ
    m_currentIndex = ((m_cellHeight - 1 - inCellY) * m_cellWidth + inCellX) * m_cellWidth + inCellZ;
}

f64 CellCache::getCachedValue() const
{
    if (m_currentIndex >= 0 && m_currentIndex < static_cast<i32>(m_values.size())) {
        return m_values[static_cast<size_t>(m_currentIndex)];
    }
    return 0.0;
}

// ============================================================================
// CacheOnce 实现
// ============================================================================

CacheOnce::CacheOnce(std::unique_ptr<DensityFunction> input)
    : m_input(std::move(input))
    , m_lastCounter(0)
    , m_lastValue(0.0)
{}

f64 CacheOnce::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    // MC 1.21: 如果绑定了 NoiseChunk 的 interpolationCounter，
    // 在同一插值步骤内直接返回缓存值
    if (m_interpolationCounter != nullptr) {
        const u64 currentCounter = *m_interpolationCounter;
        if (currentCounter == m_lastCounter) {
            return m_lastValue;
        }
        const f64 value = m_input->compute(blockX, blockY, blockZ);
        m_lastCounter = currentCounter;
        m_lastValue = value;
        return value;
    }
    // 未绑定计数器时直接委托（不应发生，但作为安全回退）
    return m_input->compute(blockX, blockY, blockZ);
}

// ============================================================================
// NoiseChunk 实现
// ============================================================================

NoiseChunk::NoiseChunk(NoiseRouter router,
    i32 cellWidth,
    i32 cellHeight,
    i32 cellCountY,
    i32 startBlockX,
    i32 startBlockY,
    i32 startBlockZ,
    std::unique_ptr<DensityFunction> beardifier)
    : m_cellConfig{cellWidth, cellHeight, 0, cellCountY}
    , m_startBlockX(startBlockX)
    , m_startBlockZ(startBlockZ)
    , m_firstCellX(math::floorDiv(startBlockX, cellWidth))
    , m_firstCellY(math::floorDiv(startBlockY, cellHeight))
    , m_firstCellZ(math::floorDiv(startBlockZ, cellWidth))
    , m_beardifier(std::move(beardifier))
    , m_router(std::move(router))
{
    m_cellConfig.cellCountXZ = world::CHUNK_WIDTH / cellWidth;

    // MC 1.21: 在 finalDensity 上叠加 BeardifierMarker，包装在 CacheAllInCell 中
    // 对应 Java: DensityFunctions.cacheAllInCell(
    //     DensityFunctions.add(noiserouter.finalDensity(), DensityFunctions.BeardifierMarker.INSTANCE))
    // mapAll 时 BeardifierMarker 会被替换为实际的 Beardifier 实例（或 Constant(0.0)）
    auto currentFinalDensity = m_router.extractFinalDensity();
    auto composite = std::make_unique<TwoArgument>(
        std::move(currentFinalDensity), factory::beardifierMarker(), TwoArgumentType::Add);
    auto cachedComposite = std::make_unique<Marker>(MarkerType::CacheAllInCell, std::move(composite));
    m_router.replaceFinalDensity(std::move(cachedComposite));

    // MC 1.21: 使用 mapAll(*this) 遍历密度函数树，将 Marker 替换为 NoiseChunk 特定实现
    // 对所有 15 个密度函数执行 mapAll，将 Interpolated → NoiseInterpolator，
    // CacheAllInCell → CellCache，BeardifierMarker → 实际 Beardifier，
    // CacheOnce/FlatCache/Cache2D 保持原样
    m_router.mapAll(*this);
}

NoiseChunk::~NoiseChunk() = default;
NoiseChunk::NoiseChunk(NoiseChunk&&) noexcept = default;

void NoiseChunk::setAquifer(std::unique_ptr<aquifer::Aquifer> aq)
{
    m_aquifer = std::move(aq);
}

std::unique_ptr<DensityFunction> NoiseChunk::apply(std::unique_ptr<DensityFunction> function)
{
    // MC 1.21: 根据 Marker 类型替换为 NoiseChunk 特定实现
    if (auto* marker = dynamic_cast<Marker*>(function.get())) {
        switch (marker->markerType()) {
            case MarkerType::Interpolated: {
                // Interpolated → NoiseInterpolator
                auto filler = marker->releaseWrapped();
                auto interpolator = std::make_unique<NoiseInterpolator>(
                    std::move(filler), m_cellConfig.cellCountXZ, m_cellConfig.cellCountY);
                // 注册到插值器列表，以便 fillSlice/selectCellYZ/updateForXYZ 驱动
                auto* rawPtr = interpolator.get();
                m_interpolators.push_back(std::move(interpolator));
                // 返回 NoiseInterpolator 的引用（不拥有，interpolators 列表拥有）
                return std::make_unique<DensityFunctionReference>(*rawPtr);
            }
            case MarkerType::CacheAllInCell: {
                // CacheAllInCell → CellCache（在 selectCellYZ 时预填充）
                auto filler = marker->releaseWrapped();
                auto cache =
                    std::make_unique<CellCache>(std::move(filler), m_cellConfig.cellWidth, m_cellConfig.cellHeight);
                m_cellCaches.push_back(std::move(cache));
                // 返回最后一个 CellCache 的引用
                return std::make_unique<DensityFunctionReference>(*m_cellCaches.back());
            }
            case MarkerType::CacheOnce: {
                // CacheOnce → 替换为绑定 interpolationCounter 的 CacheOnce
                auto filler = marker->releaseWrapped();
                auto cacheOnce = std::make_unique<CacheOnce>(std::move(filler));
                cacheOnce->bindInterpolationCounter(&m_interpolationCounter);
                return cacheOnce;
            }
            case MarkerType::FlatCache: {
                // FlatCache → 替换为区块级扁平缓存实例
                auto filler = marker->releaseWrapped();
                return std::make_unique<FlatCache>(std::move(filler));
            }
            case MarkerType::Cache2D: {
                // Cache2D → 替换为 XZ 位置缓存实例
                auto filler = marker->releaseWrapped();
                return std::make_unique<Cache2D>(std::move(filler));
            }
            case MarkerType::BeardifierMarker: {
                // MC 1.21: 替换为实际的 Beardifier 实例
                // 如果提供了 beardifier，返回指向它的引用（与 NoiseInterpolator/CellCache 相同模式）
                // 如果没有提供（如高度查询），释放包装的 Constant(0.0)
                if (m_beardifier) {
                    return std::make_unique<DensityFunctionReference>(*m_beardifier);
                }
                auto filler = marker->releaseWrapped();
                return filler;
            }
        }
    }
    return function;
}

void NoiseChunk::initializeForFirstCellX()
{
    m_interpolating = true;

    for (auto& interp : m_interpolators) {
        interp->fillSlice(
            true, m_firstCellX, m_firstCellZ, m_firstCellY, m_cellConfig.cellWidth, m_cellConfig.cellHeight);
    }
}

void NoiseChunk::advanceCellX(i32 cellX)
{
    for (auto& interp : m_interpolators) {
        interp->fillSlice(false,
            m_firstCellX + cellX + 1,
            m_firstCellZ,
            m_firstCellY,
            m_cellConfig.cellWidth,
            m_cellConfig.cellHeight);
    }
}

void NoiseChunk::selectCellXYZ(i32 cellX, i32 cellY, i32 cellZ)
{
    m_selectedCellX = cellX;
    m_selectedCellY = cellY;
    m_selectedCellZ = cellZ;

    // 为所有插值器加载 8 个角点值
    for (auto& interp : m_interpolators) {
        interp->selectCellYZ(cellY, cellZ);
    }

    // 预填充所有 CellCache
    m_fillingCell = true;

    const i32 cellStartBlockX = (m_firstCellX + cellX) * m_cellConfig.cellWidth;
    const i32 cellStartBlockY = (m_firstCellY + cellY) * m_cellConfig.cellHeight;
    const i32 cellStartBlockZ = (m_firstCellZ + cellZ) * m_cellConfig.cellWidth;

    for (auto& cache : m_cellCaches) {
        cache->fillCell(cellStartBlockX, cellStartBlockY, cellStartBlockZ);
    }

    m_fillingCell = false;
}

void NoiseChunk::updateForY(f64 delta)
{
    for (auto& interp : m_interpolators) {
        interp->updateForY(delta);
    }
    // MC 1.21: interpolationCounter 仅在 updateForZ 中递增，不在 updateForY 中递增
    // CacheOnce 依赖此计数器判断缓存是否有效
}

void NoiseChunk::updateForX(f64 delta)
{
    for (auto& interp : m_interpolators) {
        interp->updateForX(delta);
    }
}

void NoiseChunk::updateForZ(f64 delta)
{
    for (auto& interp : m_interpolators) {
        interp->updateForZ(delta);
    }
    ++m_interpolationCounter;
}

void NoiseChunk::swapSlices()
{
    for (auto& interp : m_interpolators) {
        interp->swapSlices();
    }
}

i32 NoiseChunk::samplePreliminarySurfaceLevel(i32 blockX, i32 blockZ) const
{
    const i32 quartAlignedX = math::floorDiv(blockX, 4) * 4;
    const i32 quartAlignedZ = math::floorDiv(blockZ, 4) * 4;
    const i64 cacheKey = packXZ(quartAlignedX, quartAlignedZ);

    const auto found = m_preliminarySurfaceLevelCache.find(cacheKey);
    if (found != m_preliminarySurfaceLevelCache.end()) {
        return found->second;
    }

    const i32 surfaceLevel =
        static_cast<i32>(std::floor(m_router.preliminarySurfaceLevel().compute(quartAlignedX, 0, quartAlignedZ)));
    m_preliminarySurfaceLevelCache.emplace(cacheKey, surfaceLevel);
    return surfaceLevel;
}

i32 NoiseChunk::maxPreliminarySurfaceLevel(i32 minBlockX, i32 minBlockZ, i32 maxBlockX, i32 maxBlockZ) const
{
    i32 result = std::numeric_limits<i32>::min();
    for (i32 z = minBlockZ; z <= maxBlockZ; z += 4) {
        for (i32 x = minBlockX; x <= maxBlockX; x += 4) {
            result = std::max(result, samplePreliminarySurfaceLevel(x, z));
        }
    }
    return result;
}

void NoiseChunk::setInCellPos(i32 inCellX, i32 inCellY, i32 inCellZ)
{
    m_inCellX = inCellX;
    m_inCellY = inCellY;
    m_inCellZ = inCellZ;

    for (auto& cache : m_cellCaches) {
        cache->setInCellPos(inCellX, inCellY, inCellZ);
    }
}

biome::climate::Sampler NoiseChunk::cachedClimateSampler(const std::vector<biome::climate::ParameterPoint>& spawnTarget)
{
    // MC 1.21: NoiseChunk.cachedClimateSampler()
    // 使用经过 mapAll(this::wrap) 包装的密度函数创建 Climate::Sampler。
    // 这些密度函数已被 NoiseInterpolator/CacheOnce/CellCache 包装，
    // 在区块生成上下文中采样时使用插值缓存。
    (void)spawnTarget; // TODO: spawnTarget 用于 findSpawnPosition，暂不实现

    if (!m_cachedSampler) {
        m_cachedSampler = std::make_unique<biome::climate::Sampler>(m_router.temperature(),
            m_router.vegetation(),
            m_router.continents(),
            m_router.erosion(),
            m_router.depth(),
            m_router.ridges());
    }

    // 返回采样器副本（Sampler 只持有指针，拷贝是廉价的）
    return *m_cachedSampler;
}

} // namespace mc::world::gen::density
