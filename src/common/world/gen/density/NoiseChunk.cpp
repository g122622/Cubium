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

#include "common/world/gen/density/NoiseChunk.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include "common/world/biome/climate/Sampler.hpp"
#include "common/world/gen/aquifer/Aquifer.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

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
    , m_yPoints(cellCountY + 1)
    , m_cellCountZ(cellCountZ)
    , m_cellCountY(cellCountY)
{
    const i32 zPoints = cellCountZ + 1;
    const i32 yPoints = cellCountY + 1;

    // 扁平布局：单层 vector 大小 zPoints * yPoints，索引 [z * yPoints + y]，
    // 替代原 vector<vector<f64>>（每 interpolator 省 zPoints*2 次内层 vector 堆分配）。
    m_slice0.assign(static_cast<size_t>(zPoints) * static_cast<size_t>(yPoints), 0.0);
    m_slice1.assign(static_cast<size_t>(zPoints) * static_cast<size_t>(yPoints), 0.0);
}

f64 NoiseInterpolator::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    // MC 1.21 NoiseInterpolator.compute():
    // 1. 如果不在 NoiseChunk 上下文中 → 委托给 filler
    // 2. 如果不在插值循环中 → 报错（不应发生）
    // 3. 如果 fillingCell == true → 直接做三线性插值（lerp3），
    //    使用 inCellX/cellWidth, inCellY/cellHeight, inCellZ/cellWidth 作为插值比例
    // 4. 否则 → 返回 updateForZ() 递增更新的 m_value
    if (m_noiseChunk != nullptr && m_noiseChunk->fillingCell()) {
        // MC 1.21: fillingCell 模式 — 直接三线性插值
        const f64 deltaX = static_cast<f64>(m_noiseChunk->inCellX()) / static_cast<f64>(m_cellWidth);
        const f64 deltaY = static_cast<f64>(m_noiseChunk->inCellY()) / static_cast<f64>(m_cellHeight);
        const f64 deltaZ = static_cast<f64>(m_noiseChunk->inCellZ()) / static_cast<f64>(m_cellWidth);
        return math::lerp3(deltaX,
            deltaY,
            deltaZ,
            m_noise000,
            m_noise100,
            m_noise010,
            m_noise110,
            m_noise001,
            m_noise101,
            m_noise011,
            m_noise111);
    }

    // MC 1.21: 在插值循环中且 m_value 已有效 → 返回 m_value
    // 当 m_value 未就绪时（fillSlice 阶段，selectCellYZ 还没被调用），
    // 委托给 m_filler->compute() 直接计算原始函数值，
    // 避免 CellCache 未填充时递归到 NoiseInterpolator 返回未初始化的 m_value=0
    if (m_valueReady) {
        return m_value;
    }

    // m_value 未就绪：委托给原始函数直接计算
    // 这对应 MC Java 的 fillArray 机制：fillSlice 中 NoiseInterpolator.fillArray()
    // 调用 wrapped().fillArray()，递归计算叶节点值，绕过插值器缓存
    return m_filler->compute(blockX, blockY, blockZ);
}

void NoiseInterpolator::fillSlice(NoiseChunk& noiseChunk, bool isSlice0, i32 cellX)
{
    // MC 1.21 fillSlice 逻辑：
    // 1. 设置 cellStartBlockX 和 inCellX = 0
    // 2. 遍历 Z 列，设置 cellStartBlockZ 和 inCellZ = 0，递增 arrayInterpolationCounter
    // 3. 对每个插值器，使用 sliceFillingContextProvider 填充 Y 值
    //    sliceFillingContextProvider 在每个 Y 点设置 cellStartBlockY、递增 interpolationCounter、设置 inCellY=0
    // 4. 填充完后再次递增 arrayInterpolationCounter

    const i32 cellWidth = noiseChunk.cellConfig().cellWidth;
    const i32 cellHeight = noiseChunk.cellConfig().cellHeight;
    const i32 firstCellZ = noiseChunk.firstCellZ();
    const i32 firstCellY = noiseChunk.firstCellY();
    const i32 cellCountXZ = noiseChunk.cellConfig().cellCountXZ;

    noiseChunk.m_cellStartBlockX = cellX * cellWidth;
    noiseChunk.m_inCellX = 0;

    const i32 zPoints = cellCountXZ + 1;
    const i32 yPoints = m_cellCountY + 1;

    auto& targetSlice = isSlice0 ? m_slice0 : m_slice1;
    for (i32 z = 0; z < zPoints; ++z) {
        const i32 cellZ = firstCellZ + z;
        noiseChunk.m_cellStartBlockZ = cellZ * cellWidth;
        noiseChunk.m_inCellZ = 0;
        ++noiseChunk.m_arrayInterpolationCounter;

        const size_t zBase = static_cast<size_t>(z) * static_cast<size_t>(yPoints);
        for (i32 y = 0; y < yPoints; ++y) {
            // MC 1.21: sliceFillingContextProvider.fillAllDirectly / forIndex
            // 设置 cellStartBlockY, 递增 interpolationCounter, 设置 inCellY = 0
            noiseChunk.m_cellStartBlockY = (y + firstCellY) * cellHeight;
            ++noiseChunk.m_interpolationCounter;
            noiseChunk.m_inCellY = 0;
            noiseChunk.m_arrayIndex = y;

            targetSlice[zBase + static_cast<size_t>(y)] =
                m_filler->compute(noiseChunk.blockX(), noiseChunk.blockY(), noiseChunk.blockZ());
        }
    }

    ++noiseChunk.m_arrayInterpolationCounter;
}

void NoiseInterpolator::selectCellYZ(i32 cellY, i32 cellZ)
{
    // MC 1.21 索引: slice[z][y] → 扁平 [z * m_yPoints + y]
    const size_t y = static_cast<size_t>(cellY);
    const size_t y1 = static_cast<size_t>(cellY + 1);
    const size_t z = static_cast<size_t>(cellZ);
    const size_t z1 = static_cast<size_t>(cellZ + 1);
    const size_t stride = static_cast<size_t>(m_yPoints);

    m_noise000 = m_slice0[z * stride + y];
    m_noise010 = m_slice0[z * stride + y1];
    m_noise001 = m_slice0[z1 * stride + y];
    m_noise011 = m_slice0[z1 * stride + y1];

    m_noise100 = m_slice1[z * stride + y];
    m_noise110 = m_slice1[z * stride + y1];
    m_noise101 = m_slice1[z1 * stride + y];
    m_noise111 = m_slice1[z1 * stride + y1];

    // MC 1.21: selectCellYZ 加载了新的角点值，m_value 需要通过 updateForY/X/Z 重新计算
    m_valueReady = false;
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
    m_valueReady = true;
    return m_value;
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
    // MC 1.21 CacheAllInCell.compute():
    // 1. 如果不在 NoiseChunk 上下文：委托给原始函数
    // 2. 如果不在插值循环中：报错
    // 3. 使用 inCellX/Y/Z 查表，越界时委托给原始函数
    if (!m_filled || m_noiseChunk == nullptr) {
        return m_filler->compute(blockX, blockY, blockZ);
    }

    // MC 1.21: interpolating 检查 — 在 C++ 中使用 m_filled 作为守卫
    const i32 inCellX = m_noiseChunk->inCellX();
    const i32 inCellY = m_noiseChunk->inCellY();
    const i32 inCellZ = m_noiseChunk->inCellZ();

    if (inCellX >= 0 && inCellY >= 0 && inCellZ >= 0 && inCellX < m_cellWidth && inCellY < m_cellHeight &&
        inCellZ < m_cellWidth) {
        const i32 idx = ((m_cellHeight - 1 - inCellY) * m_cellWidth + inCellX) * m_cellWidth + inCellZ;
        return m_values[static_cast<size_t>(idx)];
    }

    return m_filler->compute(blockX, blockY, blockZ);
}

void CellCache::fillCell(NoiseChunk& noiseChunk)
{
    // MC 1.21 selectCellYZ 中的 fillAllDirectly 逻辑：
    // noiseChunk.fillingCell = true
    // 递增 arrayInterpolationCounter
    // 对每个 CacheAllInCell: filler.fillArray(values, noiseChunk)
    //   → noiseChunk.fillAllDirectly: 遍历 Y(高→低), X, Z, 设置 inCellX/Y/Z 和 arrayIndex
    //     对每个位置: values[arrayIndex++] = filler.compute(noiseChunk)
    // 递增 arrayInterpolationCounter
    // noiseChunk.fillingCell = false
    //
    // MC Java 的 fillAllDirectly 不递增 interpolationCounter，但 CacheOnce 通过
    // fillArray + arrayInterpolationCounter 的数组级缓存来避免重复计算。
    // C++ 缺少 fillArray 机制，因此必须在每个位置递增 interpolationCounter，
    // 使 CacheOnce 的位置级缓存在每个位置正确失效，否则同一 cell 内
    // 所有位置会返回第一个位置的缓存值（CacheOnce 缓存污染）。

    i32 idx = 0;
    for (i32 y = m_cellHeight - 1; y >= 0; --y) {
        noiseChunk.m_inCellY = y;
        for (i32 x = 0; x < m_cellWidth; ++x) {
            noiseChunk.m_inCellX = x;
            for (i32 z = 0; z < m_cellWidth; ++z) {
                noiseChunk.m_inCellZ = z;
                noiseChunk.m_arrayIndex = idx;
                ++noiseChunk.m_interpolationCounter;
                m_values[static_cast<size_t>(idx)] =
                    m_filler->compute(noiseChunk.blockX(), noiseChunk.blockY(), noiseChunk.blockZ());
                ++idx;
            }
        }
    }
    m_filled = true;
}

// ============================================================================
// CacheOnce 实现
// ============================================================================

CacheOnce::CacheOnce(std::unique_ptr<DensityFunction> input)
    : m_input(std::move(input))
    , m_lastCounter(0)
    , m_lastValue(0.0)
    , m_lastArrayCounter(0)
{}

f64 CacheOnce::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    // MC 1.21 CacheOnce.compute():
    // 1. 如果不在 NoiseChunk 上下文：委托（已实现，对齐 Java FunctionContext != NoiseChunk.this）
    // 2. 如果 lastArray != null && lastArrayCounter == arrayInterpolationCounter：
    //    返回 lastArray[arrayIndex] — 数组级缓存命中
    // 3. 如果 lastCounter == interpolationCounter：返回 lastValue — 位置级缓存命中
    // 4. 否则计算、缓存、返回

    if (m_interpolationCounter != nullptr) {
        // 身份检查：非 NoiseChunk 采样上下文时委托给原始函数，绕过缓存。
        // 原版 Java 通过 FunctionContext != NoiseChunk.this 判断；Cubium 以 interpolating()
        // 区分采样窗口（仅在 initializeForFirstCellX 与 stopInterpolation 之间为 true）。
        // FlatCache 预计算（构造期 mapAll）、generateBiomes/buildSurface/applyCarvers 等非插值
        // 路径 interpolating()==false → 委托，避免误命中位置级缓存返回过期 m_lastValue=0.0。
        if (m_noiseChunk != nullptr && !m_noiseChunk->interpolating()) {
            return m_input->compute(blockX, blockY, blockZ);
        }

        // 数组级缓存（在 fillSlice/selectCellYZ 期间有效）
        if (!m_lastArray.empty() && m_arrayInterpolationCounter != nullptr &&
            m_lastArrayCounter == *m_arrayInterpolationCounter && m_arrayIndex != nullptr) {
            const i32 idx = *m_arrayIndex;
            if (idx >= 0 && idx < static_cast<i32>(m_lastArray.size())) {
                return m_lastArray[static_cast<size_t>(idx)];
            }
        }

        // 位置级缓存（在 updateForZ 期间有效）
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
    std::unique_ptr<DensityFunction> beardifier,
    i32 cellCountXZ)
    : m_cellConfig{cellWidth, cellHeight, 0, cellCountY}
    , m_startBlockX(startBlockX)
    , m_startBlockZ(startBlockZ)
    , m_firstCellX(math::floorDiv(startBlockX, cellWidth))
    , m_firstCellY(math::floorDiv(startBlockY, cellHeight))
    , m_firstCellZ(math::floorDiv(startBlockZ, cellWidth))
    , m_beardifier(std::move(beardifier))
    , m_router(std::move(router))
{
    // MC 1.21: 区块生成时 cellCountXZ = CHUNK_WIDTH / cellWidth (通常=4)，
    // 单列查询时 cellCountXZ = 1 (iterateNoiseColumn 传入 1)
    m_cellConfig.cellCountXZ = (cellCountXZ >= 0) ? cellCountXZ : (world::CHUNK_WIDTH / cellWidth);

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
                // 绑定到 NoiseChunk 以支持 fillingCell 模式下的 lerp3
                interpolator->bindNoiseChunk(this, m_cellConfig.cellWidth, m_cellConfig.cellHeight);
                // 注册到插值器列表，以便 fillSlice/selectCellYZ/updateForXYZ 驱动
                auto* rawPtr = interpolator.get();
                m_interpolators.push_back(std::move(interpolator));
                // 返回 NoiseInterpolator 的引用（不拥有，interpolators 列表拥有）
                return std::make_unique<DensityFunctionReference>(*rawPtr);
            }
            case MarkerType::CacheAllInCell: {
                // CacheAllInCell → CellCache（在 selectCellXYZ 时预填充）
                auto filler = marker->releaseWrapped();
                auto cache =
                    std::make_unique<CellCache>(std::move(filler), m_cellConfig.cellWidth, m_cellConfig.cellHeight);
                cache->bindNoiseChunk(this);
                m_cellCaches.push_back(std::move(cache));
                // 返回最后一个 CellCache 的引用
                return std::make_unique<DensityFunctionReference>(*m_cellCaches.back());
            }
            case MarkerType::CacheOnce: {
                // CacheOnce → 替换为绑定 interpolationCounter 和 arrayInterpolationCounter 的 CacheOnce
                auto filler = marker->releaseWrapped();
                auto cacheOnce = std::make_unique<CacheOnce>(std::move(filler));
                cacheOnce->bindInterpolationCounter(
                    &m_interpolationCounter, &m_arrayInterpolationCounter, &m_arrayIndex);
                cacheOnce->bindNoiseChunk(this);
                return cacheOnce;
            }
            case MarkerType::FlatCache: {
                // FlatCache → 替换为区块级扁平缓存实例，构造期预计算整张 quart XZ 表
                // 对齐原版 NoiseChunk.FlatCache（NoiseChunk.java:619-665）：构造时传 precompute=true，
                // 双 for 预计算 values[(noiseSizeXZ+1)²]，之后 compute() O(1) 查表。
                // 几何参数：firstNoiseX/Z = floorDiv(startBlockX/Z, 4)，noiseSizeXZ = cellCountXZ*cellWidth/4。
                auto filler = marker->releaseWrapped();
                return std::make_unique<FlatCache>(
                    std::move(filler), firstNoiseX(), firstNoiseZ(), noiseSizeXZ(), true);
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
    m_interpolationCounter = 0;

    // MC 1.21: 在 fillSlice 之前，重置所有插值器的 m_valueReady 标志。
    // fillSlice 期间 NoiseInterpolator::compute() 应委托给原始函数计算角点值，
    // 而非返回上一个 cell 的过期 m_value 缓存。
    // MC Java 通过 fillArray 机制直接遍历计算，不存在此问题；
    // C++ 实现中 fillSlice 调用 m_filler->compute()，若 m_valueReady 仍为 true，
    // NoiseInterpolator 会返回过期 m_value 而非重新计算，导致角点值错误。
    for (auto& interp : m_interpolators) {
        interp->resetValueReady();
    }

    // MC 1.21: 在 fillSlice 之前，重置所有 CellCache 的缓存状态。
    // MC Java 通过 fillArray 机制在 fillSlice 期间绕过 CellCache 缓存，
    // C++ 简化为在 fillSlice 之前将 m_filled 重置为 false，
    // 使 CellCache::compute() 委托给原始函数而不是查表返回上一个 cell 的过期缓存值。
    for (auto& cache : m_cellCaches) {
        cache->invalidate();
    }

    // MC 1.21: fillSlice 设置 cellStartBlockX/Z 和 inCellX/Z，并递增 arrayInterpolationCounter
    for (auto& interp : m_interpolators) {
        interp->fillSlice(*this, true, m_firstCellX);
    }
}

void NoiseChunk::advanceCellX(i32 cellX)
{
    // MC 1.21: 在 fillSlice 之前，重置所有插值器的 m_valueReady 标志。
    // 上一个 cellX 的迭代中 updateForZ 已将 m_valueReady 设为 true，
    // 若不重置，fillSlice 期间 NoiseInterpolator::compute() 会返回过期的 m_value
    // 而非委托给原始函数重新计算，导致切片角点值错误。
    for (auto& interp : m_interpolators) {
        interp->resetValueReady();
    }

    // MC 1.21: 在 fillSlice 之前，重置所有 CellCache 的缓存状态。
    // 此时 CellCache 包含上一个 cell 的缓存值，fillSlice 期间不应使用这些过期值。
    for (auto& cache : m_cellCaches) {
        cache->invalidate();
    }

    for (auto& interp : m_interpolators) {
        interp->fillSlice(*this, false, m_firstCellX + cellX + 1);
    }
    // MC 1.21: fillSlice 临时将 cellStartBlockX 设置为下一个 cell 的值，
    // 这里修正回当前 cell 的值，供后续迭代使用
    m_cellStartBlockX = (m_firstCellX + cellX) * m_cellConfig.cellWidth;
}

void NoiseChunk::selectCellXYZ(i32 cellX, i32 cellY, i32 cellZ)
{
    m_selectedCellX = cellX;
    m_selectedCellY = cellY;
    m_selectedCellZ = cellZ;

    // MC 1.21: 更新 cellStartBlockX/Y/Z
    m_cellStartBlockX = (m_firstCellX + cellX) * m_cellConfig.cellWidth;
    m_cellStartBlockY = (m_firstCellY + cellY) * m_cellConfig.cellHeight;
    m_cellStartBlockZ = (m_firstCellZ + cellZ) * m_cellConfig.cellWidth;

    // 为所有插值器加载 8 个角点值
    for (auto& interp : m_interpolators) {
        interp->selectCellYZ(cellY, cellZ);
    }

    // MC 1.21 selectCellYZ: 预填充 CellCache（设置 fillingCell 标志）
    m_fillingCell = true;
    ++m_arrayInterpolationCounter;

    for (auto& cache : m_cellCaches) {
        cache->fillCell(*this);
    }

    ++m_arrayInterpolationCounter;
    m_fillingCell = false;
}

void NoiseChunk::selectCellYZ(i32 cellY, i32 cellZ)
{
    m_selectedCellY = cellY;
    m_selectedCellZ = cellZ;

    // MC 1.21: 更新 cellStartBlockY/Z（不更新 X，因为 advanceCellX 已经设置了）
    m_cellStartBlockY = (m_firstCellY + cellY) * m_cellConfig.cellHeight;
    m_cellStartBlockZ = (m_firstCellZ + cellZ) * m_cellConfig.cellWidth;

    // 为所有插值器加载 8 个角点值
    for (auto& interp : m_interpolators) {
        interp->selectCellYZ(cellY, cellZ);
    }

    // MC 1.21 selectCellYZ: 预填充 CellCache（设置 fillingCell 标志）
    m_fillingCell = true;
    ++m_arrayInterpolationCounter;

    for (auto& cache : m_cellCaches) {
        cache->fillCell(*this);
    }

    ++m_arrayInterpolationCounter;
    m_fillingCell = false;
}

void NoiseChunk::updateForY(i32 blockY, f64 delta)
{
    // MC 1.21: inCellY = blockY - cellStartBlockY
    m_inCellY = blockY - m_cellStartBlockY;

    for (auto& interp : m_interpolators) {
        interp->updateForY(delta);
    }
}

void NoiseChunk::updateForX(i32 blockX, f64 delta)
{
    // MC 1.21: inCellX = blockX - cellStartBlockX
    m_inCellX = blockX - m_cellStartBlockX;

    for (auto& interp : m_interpolators) {
        interp->updateForX(delta);
    }
}

void NoiseChunk::updateForZ(i32 blockZ, f64 delta)
{
    // MC 1.21: inCellZ = blockZ - cellStartBlockZ
    m_inCellZ = blockZ - m_cellStartBlockZ;
    ++m_interpolationCounter;

    for (auto& interp : m_interpolators) {
        interp->updateForZ(delta);
    }
}

void NoiseChunk::swapSlices()
{
    for (auto& interp : m_interpolators) {
        interp->swapSlices();
    }
}

void NoiseChunk::setInCellFromIndex(i32 index)
{
    // MC 1.21 forIndex 逻辑：
    // i = index % cellWidth               → inCellZ
    // j = index / cellWidth
    // k = j % cellWidth                   → inCellX
    // l = cellHeight - 1 - j / cellWidth  → inCellY
    m_inCellZ = math::floorMod(index, m_cellConfig.cellWidth);
    const i32 j = math::floorDiv(index, m_cellConfig.cellWidth);
    m_inCellX = math::floorMod(j, m_cellConfig.cellWidth);
    m_inCellY = m_cellConfig.cellHeight - 1 - math::floorDiv(j, m_cellConfig.cellWidth);
    m_arrayIndex = index;
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

biome::climate::Sampler NoiseChunk::cachedClimateSampler(const std::vector<biome::climate::ParameterPoint>& spawnTarget)
{
    // MC 1.21.11: NoiseChunk.cachedClimateSampler(router, spawnTarget)
    // 使用经过 mapAll(this::wrap) 包装的密度函数创建 Climate::Sampler。
    // 这些密度函数已被 NoiseInterpolator/CacheOnce/CellCache 包装，
    // 在区块生成上下文中采样时使用插值缓存。
    //
    // spawnTarget 用于 Climate.Sampler.findSpawnPosition()，在气候空间中
    // 径向搜索最佳出生点。SpawnFinder 实现见 Climate.cpp。
    if (!m_cachedSampler) {
        m_cachedSampler = std::make_unique<biome::climate::Sampler>(m_router.temperature(),
            m_router.vegetation(),
            m_router.continents(),
            m_router.erosion(),
            m_router.depth(),
            m_router.ridges());
        // 将出生点目标传给采样器，供后续 findSpawnPosition 使用。
        // 与 MC 一致：Sampler 构造时即接收 spawnTarget，这里在缓存创建时设置。
        m_cachedSampler->setSpawnTarget(spawnTarget);
    } else if (!spawnTarget.empty() && m_cachedSampler->spawnTarget() != spawnTarget) {
        // 不同区块可能传入相同 spawnTarget（来自同一 DimensionSettings），通常无需更新；
        // 但若调用方显式传入不同目标，则刷新以保证语义一致。
        m_cachedSampler->setSpawnTarget(spawnTarget);
    }

    // 返回采样器副本（Sampler 只持有指针，拷贝是廉价的）
    return *m_cachedSampler;
}

} // namespace mc::world::gen::density
