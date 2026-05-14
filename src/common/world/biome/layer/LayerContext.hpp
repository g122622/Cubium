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

#pragma once

#include "../../../util/cache/Long2IntLRUCache.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../gen/noise/ImprovedNoiseGenerator.hpp"
#include "Layer.hpp"
#include "LayerCacheConfig.hpp"
#include <memory>

namespace mc {

/**
 * @brief Layer 上下文实现
 *
 * 提供位置感知的随机数生成和缓存管理。
 * 参考 MC LazyAreaLayerContext + IExtendedNoiseRandom
 */
class LayerContext : public IExtendedAreaContext {
public:
    /**
     * @brief 构造 Layer 上下文
     * @param maxCacheSize 最大缓存大小
     * @param worldSeed 世界种子
     * @param modifier 层修饰符（每层唯一）
     */
    LayerContext(i32 maxCacheSize, u64 worldSeed, u64 modifier);

    ~LayerContext() override = default;

    // === IAreaContext 接口 ===

    /**
     * @brief 设置当前位置（必须在采样前调用）
     *
     * 使用 MC 的 FastRandom.mix 算法混合种子：
     * mix(seed, x) -> mix(result, z) -> mix(result, x) -> mix(result, z)
     *
     * @param x X 坐标
     * @param z Z 坐标
     */
    void setPosition(i64 x, i64 z) override;

    /**
     * @brief 获取随机整数
     * @param bound 上界
     * @return [0, bound) 范围内的随机整数
     */
    [[nodiscard]] i32 nextInt(i32 bound) override;

    /**
     * @brief 从两个值中随机选择
     */
    [[nodiscard]] i32 pickRandom(i32 a, i32 b) override;

    /**
     * @brief 从四个值中随机选择
     */
    [[nodiscard]] i32 pickRandom(i32 a, i32 b, i32 c, i32 d) override;

    /**
     * @brief 获取噪声生成器
     */
    [[nodiscard]] ImprovedNoiseGenerator* getNoiseGenerator() override { return &m_noise; }

    // === IExtendedAreaContext 接口 ===

    /**
     * @brief 创建延迟计算区域
     */
    [[nodiscard]] std::unique_ptr<IArea> makeArea(PixelFunc pixelFunc) override;

    /**
     * @brief 创建延迟计算区域（单输入，持有所有权）
     */
    [[nodiscard]] std::unique_ptr<IArea> makeArea(PixelFunc pixelFunc, std::unique_ptr<IArea> input) override;

    /**
     * @brief 创建延迟计算区域（双输入，持有所有权）
     */
    [[nodiscard]] std::unique_ptr<IArea> makeArea(
        PixelFunc pixelFunc, std::unique_ptr<IArea> input1, std::unique_ptr<IArea> input2) override;

    // === 工厂方法 ===

    /**
     * @brief 创建子上下文（用于层链）
     * @param modifier 层修饰符
     * @return 新的上下文
     */
    [[nodiscard]] std::unique_ptr<LayerContext> withModifier(u64 modifier) const;

    /**
     * @brief 获取最大缓存大小
     */
    [[nodiscard]] i32 getMaxCacheSize() const { return m_maxCacheSize; }

    /**
     * @brief 获取共享缓存
     */
    [[nodiscard]] Long2IntLRUCache& getCache() { return m_cache; }

private:
    i32 m_maxCacheSize;
    u64 m_worldSeed;
    u64 m_layerSeed;    // 每层唯一的种子
    u64 m_positionSeed; // 当前位置的种子
    ImprovedNoiseGenerator m_noise;
    Long2IntLRUCache m_cache;

    /**
     * @brief FastRandom.mix 算法
     *
     * 参考 MC FastRandom.mix:
     * left = left * (left * 6364136223846793005L + 1442695040888963407L);
     * return left + right;
     */
    static u64 mix(u64 left, u64 right);

    /**
     * @brief 计算层种子
     * @param worldSeed 世界种子
     * @param modifier 层修饰符
     * @return 层种子
     */
    static u64 hashLayerSeed(u64 worldSeed, u64 modifier);
};

/**
 * @brief 延迟计算区域实现
 *
 * 使用回调函数计算像素值，并缓存结果。
 * 参考 MC LazyArea
 */
class LazyArea : public IArea {
public:
    /**
     * @brief 构造延迟区域
     * @param cache 共享缓存
     * @param maxCacheSize 最大缓存大小
     * @param pixelFunc 像素计算函数
     */
    LazyArea(Long2IntLRUCache& cache, i32 maxCacheSize, PixelFunc pixelFunc);

    /**
     * @brief 构造延迟区域（独立缓存）
     * @param maxCacheSize 最大缓存大小
     * @param pixelFunc 像素计算函数
     */
    LazyArea(i32 maxCacheSize, PixelFunc pixelFunc);

    /**
     * @brief 构造延迟区域（带输入区域所有权）
     * @param cache 共享缓存
     * @param maxCacheSize 最大缓存大小
     * @param pixelFunc 像素计算函数
     * @param ownedAreas 需要持有的输入区域
     */
    LazyArea(
        Long2IntLRUCache& cache, i32 maxCacheSize, PixelFunc pixelFunc, std::vector<std::unique_ptr<IArea>> ownedAreas);

    ~LazyArea() override = default;

    [[nodiscard]] i32 getValue(i32 x, i32 z) const override;

    /**
     * @brief 批量采样接口
     *
     * 单次加锁批量获取多个坐标的值，减少锁竞争开销。
     *
     * @param startX 起始 X 坐标
     * @param startZ 起始 Z 坐标
     * @param width 宽度
     * @param height 高度
     * @param output 输出数组（大小必须 >= width * height）
     */
    void getValuesBatch(i32 startX, i32 startZ, i32 width, i32 height, i32* output) const override;

    /**
     * @brief 获取最大缓存大小
     */
    [[nodiscard]] i32 getMaxCacheSize() const { return m_maxCacheSize; }

private:
    Long2IntLRUCache* m_sharedCache;
    std::unique_ptr<Long2IntLRUCache> m_ownCache;
    PixelFunc m_pixelFunc;
    i32 m_maxCacheSize;
    std::vector<std::unique_ptr<IArea>> m_ownedAreas; // 持有输入区域的所有权
};

// ============================================================================
// 区域工厂实现
// ============================================================================

/**
 * @brief 源层工厂（零输入）
 *
 * 注意：此工厂拥有自己的 LayerContext 独立副本，确保生命周期安全。
 */
class SourceFactory : public IAreaFactory {
public:
    SourceFactory(ITransformer0* transformer, std::shared_ptr<LayerContext> context);
    [[nodiscard]] std::unique_ptr<IArea> create() const override;

private:
    ITransformer0* m_transformer;
    std::shared_ptr<LayerContext> m_context;
};

/**
 * @brief 单输入变换工厂
 *
 * 注意：此工厂拥有自己的 LayerContext 独立副本，确保生命周期安全。
 */
class TransformFactory : public IAreaFactory {
public:
    TransformFactory(
        ITransformer1* transformer, std::shared_ptr<LayerContext> context, std::unique_ptr<IAreaFactory> input);
    [[nodiscard]] std::unique_ptr<IArea> create() const override;

private:
    ITransformer1* m_transformer;
    std::shared_ptr<LayerContext> m_context;
    std::unique_ptr<IAreaFactory> m_input;
};

/**
 * @brief 双输入合并工厂
 *
 * 注意：此工厂拥有自己的 LayerContext 独立副本，确保生命周期安全。
 */
class MergeFactory : public IAreaFactory {
public:
    MergeFactory(ITransformer2* transformer,
        std::shared_ptr<LayerContext> context,
        std::unique_ptr<IAreaFactory> input1,
        std::unique_ptr<IAreaFactory> input2);
    [[nodiscard]] std::unique_ptr<IArea> create() const override;

private:
    ITransformer2* m_transformer;
    std::shared_ptr<LayerContext> m_context;
    std::unique_ptr<IAreaFactory> m_input1;
    std::unique_ptr<IAreaFactory> m_input2;
};

} // namespace mc
