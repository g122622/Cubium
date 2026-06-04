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

#include "LayerContext.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "transformers/TransformerTraits.hpp"

namespace mc {

// ============================================================================
// LayerContext 实现
// ============================================================================

LayerContext::LayerContext(i32 maxCacheSize, u64 worldSeed, u64 modifier)
    : m_maxCacheSize(maxCacheSize)
    , m_worldSeed(worldSeed)
    , m_layerSeed(_hashLayerSeed(worldSeed, modifier))
    , m_noise(worldSeed) // 使用种子初始化噪声生成器
    , m_cache(maxCacheSize)
{}

void LayerContext::setPosition(i64 x, i64 z)
{
    // 使用 FastRandom.mix 算法计算位置种子
    u64 seed = m_layerSeed;
    seed = _mix(seed, static_cast<u64>(x));
    seed = _mix(seed, static_cast<u64>(z));
    seed = _mix(seed, static_cast<u64>(x));
    seed = _mix(seed, static_cast<u64>(z));
    m_positionSeed = seed;
}

i32 LayerContext::nextInt(i32 bound)
{
    if (bound <= 0) {
        return 0;
    }

    i32 result = static_cast<i32>((m_positionSeed >> 24) % bound);
    if (result < 0) {
        result += bound;
    }
    m_positionSeed = _mix(m_positionSeed, m_layerSeed);
    return result;
}

i32 LayerContext::pickRandom(i32 a, i32 b)
{
    return nextInt(2) == 0 ? a : b;
}

i32 LayerContext::pickRandom(i32 a, i32 b, i32 c, i32 d)
{
    i32 i = nextInt(4);
    if (i == 0) return a;
    if (i == 1) return b;
    if (i == 2) return c;
    return d;
}

std::unique_ptr<IArea> LayerContext::makeArea(PixelFunc pixelFunc)
{
    return std::make_unique<LazyArea>(m_cache, m_maxCacheSize, std::move(pixelFunc));
}

std::unique_ptr<IArea> LayerContext::makeArea(PixelFunc pixelFunc, std::unique_ptr<IArea> input)
{
    std::vector<std::unique_ptr<IArea>> ownedAreas;
    ownedAreas.push_back(std::move(input));
    return std::make_unique<LazyArea>(
        m_cache, std::min(1024, m_maxCacheSize * 4), std::move(pixelFunc), std::move(ownedAreas));
}

std::unique_ptr<IArea> LayerContext::makeArea(
    PixelFunc pixelFunc, std::unique_ptr<IArea> input1, std::unique_ptr<IArea> input2)
{
    std::vector<std::unique_ptr<IArea>> ownedAreas;
    ownedAreas.push_back(std::move(input1));
    ownedAreas.push_back(std::move(input2));
    i32 newSize = std::min(1024, m_maxCacheSize * 4);
    return std::make_unique<LazyArea>(m_cache, newSize, std::move(pixelFunc), std::move(ownedAreas));
}

std::unique_ptr<LayerContext> LayerContext::withModifier(u64 modifier) const
{
    return std::make_unique<LayerContext>(m_maxCacheSize, m_worldSeed, modifier);
}

// 静态成员函数

u64 LayerContext::_mix(u64 left, u64 right)
{
    left = left * (left * 6364136223846793005ULL + 1442695040888963407ULL);
    return left + right;
}

u64 LayerContext::_hashLayerSeed(u64 worldSeed, u64 modifier)
{
    u64 hash = _mix(modifier, modifier);
    hash = _mix(hash, modifier);
    hash = _mix(hash, modifier);

    u64 result = _mix(worldSeed, hash);
    result = _mix(result, hash);
    result = _mix(result, hash);

    return result;
}

// ============================================================================
// LazyArea 实现
// ============================================================================

LazyArea::LazyArea(Long2IntLRUCache& cache, i32 maxCacheSize, PixelFunc pixelFunc)
    : m_sharedCache(&cache)
    , m_ownCache(nullptr)
    , m_pixelFunc(std::move(pixelFunc))
    , m_maxCacheSize(maxCacheSize)
{}

LazyArea::LazyArea(i32 maxCacheSize, PixelFunc pixelFunc)
    : m_sharedCache(nullptr)
    , m_ownCache(std::make_unique<Long2IntLRUCache>(maxCacheSize))
    , m_pixelFunc(std::move(pixelFunc))
    , m_maxCacheSize(maxCacheSize)
{}

LazyArea::LazyArea(
    Long2IntLRUCache& cache, i32 maxCacheSize, PixelFunc pixelFunc, std::vector<std::unique_ptr<IArea>> ownedAreas)
    : m_sharedCache(&cache)
    , m_ownCache(nullptr)
    , m_pixelFunc(std::move(pixelFunc))
    , m_maxCacheSize(maxCacheSize)
    , m_ownedAreas(std::move(ownedAreas))
{}

i32 LazyArea::getValue(i32 x, i32 z) const
{
    // MC_TRACE_EVENT("world.biome", "LazyArea_GetValue", "x", x, "z", z);
    i64 key = Long2IntLRUCache::packCoords(x, z);
    i32 value;

    Long2IntLRUCache& cache = m_sharedCache ? *m_sharedCache : *m_ownCache;

    if (cache.get(key, value)) {
        return value;
    }

    // 计算并缓存
    value = m_pixelFunc(x, z);
    cache.put(key, value);
    return value;
}

void LazyArea::getValuesBatch(i32 startX, i32 startZ, i32 width, i32 height, i32* output) const
{
    MC_TRACE_EVENT("world.biome", "LazyArea_GetValuesBatch", "width", width, "height", height);

    if (width <= 0 || height <= 0 || output == nullptr) {
        return;
    }

    Long2IntLRUCache& cache = m_sharedCache ? *m_sharedCache : *m_ownCache;

    // 批量预计算所有坐标键
    const size_t totalSize = static_cast<size_t>(width) * height;
    std::vector<i64> keys(totalSize);
    std::vector<bool> needsCompute(totalSize, false);

    // 单次加锁批量查询
    std::lock_guard<std::mutex> lock(cache.getMutex());

    // 第一遍：查询缓存
    size_t idx = 0;
    for (i32 z = 0; z < height; ++z) {
        for (i32 x = 0; x < width; ++x) {
            i64 key = Long2IntLRUCache::packCoords(startX + x, startZ + z);
            keys[idx] = key;

            i32 value;
            if (cache.getLocked(key, value)) {
                output[idx] = value;
            } else {
                needsCompute[idx] = true;
            }
            ++idx;
        }
    }

    // 第二遍：计算未命中的值（在锁内计算并写入缓存）
    idx = 0;
    for (i32 z = 0; z < height; ++z) {
        for (i32 x = 0; x < width; ++x) {
            if (needsCompute[idx]) {
                i32 value = m_pixelFunc(startX + x, startZ + z);
                cache.putLocked(keys[idx], value);
                output[idx] = value;
            }
            ++idx;
        }
    }
}

// ============================================================================
// SourceFactory 实现
// ============================================================================

SourceFactory::SourceFactory(ITransformer0* transformer, std::shared_ptr<LayerContext> context)
    : m_transformer(transformer)
    , m_context(std::move(context))
{}

std::unique_ptr<IArea> SourceFactory::create() const
{
    // 捕获 shared_ptr 以保持生命周期
    ITransformer0* transformer = m_transformer;
    std::shared_ptr<LayerContext> ctx = m_context;
    const bool useRandom = transformer->usesRandom();

    PixelFunc func = [transformer, ctx, useRandom](i32 x, i32 z) -> i32 {
        if (useRandom) {
            ctx->setPosition(x, z);
        }
        return transformer->apply(*ctx, x, z);
    };

    return m_context->makeArea(func);
}

// ============================================================================
// TransformFactory 实现
// ============================================================================

TransformFactory::TransformFactory(
    ITransformer1* transformer, std::shared_ptr<LayerContext> context, std::unique_ptr<IAreaFactory> input)
    : m_transformer(transformer)
    , m_context(std::move(context))
    , m_input(std::move(input))
{}

std::unique_ptr<IArea> TransformFactory::create() const
{
    // 创建输入区域
    std::unique_ptr<IArea> inputArea = m_input->create();

    // 捕获 shared_ptr 以保持生命周期
    ITransformer1* transformer = m_transformer;
    std::shared_ptr<LayerContext> ctx = m_context;
    IArea* inputPtr = inputArea.get();
    const bool useRandom = transformer->usesRandom();

    PixelFunc func = [transformer, ctx, inputPtr, useRandom](i32 x, i32 z) -> i32 {
        if (useRandom) {
            ctx->setPosition(x, z);
        }
        return transformer->apply(*ctx, *inputPtr, x, z);
    };

    return m_context->makeArea(func, std::move(inputArea));
}

// ============================================================================
// MergeFactory 实现
// ============================================================================

MergeFactory::MergeFactory(ITransformer2* transformer,
    std::shared_ptr<LayerContext> context,
    std::unique_ptr<IAreaFactory> input1,
    std::unique_ptr<IAreaFactory> input2)
    : m_transformer(transformer)
    , m_context(std::move(context))
    , m_input1(std::move(input1))
    , m_input2(std::move(input2))
{}

std::unique_ptr<IArea> MergeFactory::create() const
{
    std::unique_ptr<IArea> area1 = m_input1->create();
    std::unique_ptr<IArea> area2 = m_input2->create();

    // 捕获 shared_ptr 以保持生命周期
    ITransformer2* transformer = m_transformer;
    std::shared_ptr<LayerContext> ctx = m_context;
    IArea* ptr1 = area1.get();
    IArea* ptr2 = area2.get();
    const bool useRandom = transformer->usesRandom();

    PixelFunc func = [transformer, ctx, ptr1, ptr2, useRandom](i32 x, i32 z) -> i32 {
        if (useRandom) {
            ctx->setPosition(x, z);
        }
        return transformer->apply(*ctx, *ptr1, *ptr2, x, z);
    };

    return m_context->makeArea(func, std::move(area1), std::move(area2));
}

} // namespace mc
