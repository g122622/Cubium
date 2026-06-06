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

namespace mc::world::gen::density {

namespace {

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
    // CacheOnce 需要与 NoiseChunk 的 interpolationCounter 配合
    // 在当前简化实现中，暂不使用计数器缓存，直接委托
    // 完整实现需要 NoiseChunk 传递计数器
    return m_input->compute(blockX, blockY, blockZ);
}

// ============================================================================
// NoiseChunk 实现
// ============================================================================

NoiseChunk::NoiseChunk(
    const NoiseRouter& router, i32 cellWidth, i32 cellHeight, i32 startBlockX, i32 startBlockY, i32 startBlockZ)
    : m_router(router)
    , m_cellConfig{cellWidth, cellHeight, 0, 0}
    , m_startBlockX(startBlockX)
    , m_startBlockY(startBlockY)
    , m_startBlockZ(startBlockZ)
    , m_firstCellY(startBlockY / cellHeight)
{
    // MC 1.21: 区块大小 = 16 方块
    m_cellConfig.cellCountXZ = 16 / cellWidth;
    m_cellConfig.cellCountY = (world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT) / cellHeight;

    auto finalDensityInterpolator =
        std::make_unique<NoiseInterpolator>(std::make_unique<DensityFunctionReference>(m_router.finalDensity()),
            m_cellConfig.cellCountXZ,
            m_cellConfig.cellCountY);
    m_wrappedFinalDensity = std::make_unique<DensityFunctionReference>(*finalDensityInterpolator);
    m_interpolators.push_back(std::move(finalDensityInterpolator));
    m_wrappedPreliminarySurfaceLevel = std::make_unique<DensityFunctionReference>(m_router.preliminarySurfaceLevel());
}

NoiseChunk::~NoiseChunk() = default;
NoiseChunk::NoiseChunk(NoiseChunk&&) noexcept = default;

void NoiseChunk::setAquifer(std::unique_ptr<aquifer::Aquifer> aq)
{
    m_aquifer = std::move(aq);
}

std::unique_ptr<DensityFunction> NoiseChunk::wrap(std::unique_ptr<DensityFunction> function)
{
    // MC 1.21 中 Marker 类型会被替换为 NoiseChunk 特定实现
    // 当前实现中，Marker 类型不在 DensityFunction 类层次中，
    // 而是在 NoiseRouterData 中由工厂函数决定包装方式。
    // 这里保留接口以备将来实现 Marker 遍历时使用。
    return function;
}

void NoiseChunk::initializeForFirstCellX()
{
    m_interpolating = true;
    const i32 firstCellX = m_startBlockX / m_cellConfig.cellWidth;
    const i32 firstCellZ = m_startBlockZ / m_cellConfig.cellWidth;

    // 填充所有插值器的 slice0
    for (auto& interp : m_interpolators) {
        interp->fillSlice(true, firstCellX, firstCellZ, m_firstCellY, m_cellConfig.cellWidth, m_cellConfig.cellHeight);
    }
}

void NoiseChunk::advanceCellX(i32 cellX)
{
    const i32 firstCellZ = m_startBlockZ / m_cellConfig.cellWidth;

    // 填充所有插值器的 slice1
    for (auto& interp : m_interpolators) {
        interp->fillSlice(false, cellX + 1, firstCellZ, m_firstCellY, m_cellConfig.cellWidth, m_cellConfig.cellHeight);
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

    const i32 firstCellX = m_startBlockX / m_cellConfig.cellWidth;
    const i32 firstCellZ = m_startBlockZ / m_cellConfig.cellWidth;
    const i32 cellStartBlockX = (firstCellX + cellX) * m_cellConfig.cellWidth;
    const i32 cellStartBlockY = (m_firstCellY + cellY) * m_cellConfig.cellHeight;
    const i32 cellStartBlockZ = (firstCellZ + cellZ) * m_cellConfig.cellWidth;

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
    ++m_interpolationCounter;
}

void NoiseChunk::updateForX(f64 delta)
{
    for (auto& interp : m_interpolators) {
        interp->updateForX(delta);
    }
}

f64 NoiseChunk::updateForZ(f64 delta)
{
    f64 result = 0.0;
    for (auto& interp : m_interpolators) {
        result = interp->updateForZ(delta);
    }
    ++m_interpolationCounter;
    return result;
}

void NoiseChunk::swapSlices()
{
    for (auto& interp : m_interpolators) {
        interp->swapSlices();
    }
}

f64 NoiseChunk::sampleFinalDensity(i32 blockX, i32 blockY, i32 blockZ) const
{
    return m_wrappedFinalDensity->compute(blockX, blockY, blockZ);
}

f64 NoiseChunk::samplePreliminarySurfaceLevel(i32 blockX, i32 blockZ) const
{
    return m_wrappedPreliminarySurfaceLevel->compute(blockX, 0, blockZ);
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

} // namespace mc::world::gen::density
