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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>

namespace mc::world::gen::noise {

// 强制内联标注:perlinSampleSoA 内核及其依赖函数必须内联进 octave 循环,clang 才能跨 octave
// 自动向量化(未内联时循环体是 call instruction,clang 报 "call instruction cannot be vectorized")。
// clang-cl 用 __attribute__((always_inline)),须配合 inline 关键字。
#define MC_PERLIN_ALWAYS_INLINE __attribute__((always_inline)) inline

// ============================================================================
// Perlin SoA 采样内核(效仿 C2ME c2me-opts-natives-math 的 ext_math.h)
//
// 把 NormalNoise / BlendedNoise 内部多个 PerlinNoise 的非零 octave 子层在构造期
// 拍平成连续 SoA 数组(排列表 + 偏移 + 振幅 + 缩放因子),运行期单循环遍历求值,
// 让 clang 跨 octave 自动向量化(每 SIMD 通道算一个 octave,各自独立置换表做独立 gather 链,
// hash 链 p[p[p[h]+y]+z] 内部串行不碰)。
//
// 采样内核 perlinSampleSoA 与 Cubium PerlinLayer::noise/noiseWithSmear 数值 bit-exact 等价
// (已由回退前 PerlinSoA.hpp 的 perlinSample 验证,见 memory noise-soa-optimization)。
//
// 三个回退根因(commit 495832dd9)已逐条规避:
// 1. 置换表不再按值存进 PerlinSoALayer::std::array<u8,256>,而是所有 octave 置换表
//    背靠背连续存于 PerlinNoiseSoA::perms(u8[256*N]),gather base 跨 octave 连续。
// 2. perlinSampleSoA 内联进 octave 循环,标注 #pragma clang loop vectorize(enable),
//    clang 跨 octave 内联向量化(回退前是跨 perlinSample 函数调用,无法向量化)。
// 3. PerlinNoiseSoA 持续指向同一次 64 字节对齐分配,零拷贝,运行期只读连续块。
// ============================================================================

/// 单个 Perlin 采样可处理的最大 octave 数(主世界 JAGGED = 17 是上限)。
inline constexpr u32 kMaxPerlinOctaves = 32;

/**
 * @brief Perlin 噪声 SoA 数据载体(RAII)
 *
 * 单次 64 字节对齐分配持有所有 octave 数据:置换表连续背靠背(u8[256*N])+
 * 各标量参数 SoA 数组(f64[N])。析构统一释放。构造期由 PerlinNoise::buildSoA()
 * 从各 PerlinLayer 拷贝一次到连续块(构造期一次性,非热点);运行期 perlinSampleSoA
 * 只读连续块,不再持有 PerlinLayer 对象。
 *
 * NormalNoise 路径用 amplitude/inputFactor/valueFactor(PerlinNoise 语义);
 * BlendedNoise 路径只用 inputFactor(复用存 d11),amplitude/valueFactor 不用(置 0)。
 */
class PerlinNoiseSoA {
public:
    /// 默认构造的空载体(count=0),供延迟初始化或移动语义使用。
    PerlinNoiseSoA() = default;

    /**
     * @brief 分配容纳 count 个 octave 的连续 SoA 存储
     *
     * 内存布局(单次 64 字节对齐分配):
     *   [perms: u8[256*N]] (按 64 字节对齐填充)
     *   [originX: f64[N]] [originY] [originZ]
     *   [amplitude] [inputFactor] [valueFactor]
     *
     * 构造后数据未初始化,调用方须通过 permsData()/originXData()/... 可写指针填充。
     *
     * @param count 非零 octave 数 N,须 ∈ [1, kMaxPerlinOctaves]
     */
    explicit PerlinNoiseSoA(u32 count)
        : m_count(count)
    {
        MC_ASSERT_RELEASE_MSG(count > 0 && count <= kMaxPerlinOctaves, "PerlinNoiseSoA: invalid octave count");
        const size_t permsBytes = static_cast<size_t>(count) * 256ull;
        const size_t permsAligned = (permsBytes + 63ull) & ~63ull; // 64 字节对齐
        const size_t arrayBytes = static_cast<size_t>(count) * sizeof(f64);
        const size_t total = permsAligned + arrayBytes * 6ull;

        m_storage = static_cast<u8*>(allocateAligned64(total));
        m_perms = m_storage;
        m_originX = reinterpret_cast<f64*>(m_storage + permsAligned);
        m_originY = m_originX + count;
        m_originZ = m_originY + count;
        m_amplitude = m_originZ + count;
        m_inputFactor = m_amplitude + count;
        m_valueFactor = m_inputFactor + count;
    }

    ~PerlinNoiseSoA() { release(); }

    PerlinNoiseSoA(const PerlinNoiseSoA&) = delete;
    PerlinNoiseSoA& operator=(const PerlinNoiseSoA&) = delete;

    PerlinNoiseSoA(PerlinNoiseSoA&& other) noexcept
        : m_count(other.m_count)
        , m_storage(other.m_storage)
        , m_perms(other.m_perms)
        , m_originX(other.m_originX)
        , m_originY(other.m_originY)
        , m_originZ(other.m_originZ)
        , m_amplitude(other.m_amplitude)
        , m_inputFactor(other.m_inputFactor)
        , m_valueFactor(other.m_valueFactor)
    {
        other.reset();
    }

    PerlinNoiseSoA& operator=(PerlinNoiseSoA&& other) noexcept
    {
        if (this != &other) {
            release();
            m_count = other.m_count;
            m_storage = other.m_storage;
            m_perms = other.m_perms;
            m_originX = other.m_originX;
            m_originY = other.m_originY;
            m_originZ = other.m_originZ;
            m_amplitude = other.m_amplitude;
            m_inputFactor = other.m_inputFactor;
            m_valueFactor = other.m_valueFactor;
            other.reset();
        }
        return *this;
    }

    [[nodiscard]] u32 count() const noexcept { return m_count; }

    // 只读访问器(运行期 perlinSampleSoA 读取)
    [[nodiscard]] const u8* perms() const noexcept { return m_perms; }
    [[nodiscard]] const f64* originX() const noexcept { return m_originX; }
    [[nodiscard]] const f64* originY() const noexcept { return m_originY; }
    [[nodiscard]] const f64* originZ() const noexcept { return m_originZ; }
    [[nodiscard]] const f64* amplitude() const noexcept { return m_amplitude; }
    [[nodiscard]] const f64* inputFactor() const noexcept { return m_inputFactor; }
    [[nodiscard]] const f64* valueFactor() const noexcept { return m_valueFactor; }

    // 可写访问器(构造期 buildSoA 填充)
    [[nodiscard]] u8* permsData() noexcept { return m_perms; }
    [[nodiscard]] f64* originXData() noexcept { return m_originX; }
    [[nodiscard]] f64* originYData() noexcept { return m_originY; }
    [[nodiscard]] f64* originZData() noexcept { return m_originZ; }
    [[nodiscard]] f64* amplitudeData() noexcept { return m_amplitude; }
    [[nodiscard]] f64* inputFactorData() noexcept { return m_inputFactor; }
    [[nodiscard]] f64* valueFactorData() noexcept { return m_valueFactor; }

private:
    u32 m_count = 0;
    u8* m_storage = nullptr;
    u8* m_perms = nullptr;
    f64* m_originX = nullptr;
    f64* m_originY = nullptr;
    f64* m_originZ = nullptr;
    f64* m_amplitude = nullptr;
    f64* m_inputFactor = nullptr;
    f64* m_valueFactor = nullptr;

    void release() noexcept
    {
        if (m_storage != nullptr) {
            freeAligned64(m_storage);
        }
        reset();
    }

    void reset() noexcept
    {
        m_count = 0;
        m_storage = nullptr;
        m_perms = nullptr;
        m_originX = nullptr;
        m_originY = nullptr;
        m_originZ = nullptr;
        m_amplitude = nullptr;
        m_inputFactor = nullptr;
        m_valueFactor = nullptr;
    }

    /// 64 字节对齐分配(size 字节)。
    [[nodiscard]] static void* allocateAligned64(size_t size)
    {
#if defined(_MSC_VER)
        void* p = ::_aligned_malloc(size, 64);
#else
        void* p = std::aligned_alloc(64, (size + 63) & ~size_t(63));
#endif
        if (p == nullptr) {
            throw std::bad_alloc();
        }
        return p;
    }

    static void freeAligned64(void* p) noexcept
    {
#if defined(_MSC_VER)
        ::_aligned_free(p);
#else
        std::free(p);
#endif
    }
};

/// 扁平梯度表:16 个梯度 × 4 个 double 槽(前 3 分量有效,第 4 槽填充对齐)。
/// 索引方式:hash & 0xF 得 0..15,(hash<<2) 定位梯度首槽。内容与 PerlinLayer::GRADIENTS
/// 逐项一致(含末 4 项的重复梯度布局,非前 12 项简单重复)。
alignas(64) inline constexpr std::array<f64, 64> kFlatSimplexGrad = {
    1.0,
    1.0,
    0.0,
    0.0,
    -1.0,
    1.0,
    0.0,
    0.0,
    1.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    -1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    1.0,
    0.0,
    -1.0,
    0.0,
    1.0,
    0.0,
    1.0,
    0.0,
    -1.0,
    0.0,
    -1.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    1.0,
    1.0,
    0.0,
    0.0,
    -1.0,
    1.0,
    0.0,
    0.0,
    1.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    -1.0,
    0.0,
    1.0,
    1.0,
    0.0,
    0.0,
    0.0,
    -1.0,
    1.0,
    0.0,
    -1.0,
    1.0,
    0.0,
    0.0,
    0.0,
    -1.0,
    -1.0,
    0.0,
};

/// smoothstep 衰减因子:t*t*t*(t*(t*6-15)+10)。
[[nodiscard]] MC_PERLIN_ALWAYS_INLINE f64 perlinFade(f64 value)
{
    return value * value * value * (value * (value * 6.0 - 15.0) + 10.0);
}

/// 线性插值:start + delta*(end-start)。
[[nodiscard]] MC_PERLIN_ALWAYS_INLINE f64 perlinLerp(f64 delta, f64 start, f64 end)
{
    return start + delta * (end - start);
}

/// 三线性插值(8 角点值 → 单值)。
[[nodiscard]] MC_PERLIN_ALWAYS_INLINE f64 perlinLerp3(
    f64 dx, f64 dy, f64 dz, f64 v000, f64 v100, f64 v010, f64 v110, f64 v001, f64 v101, f64 v011, f64 v111)
{
    const f64 y0 = perlinLerp(dx, v000, v100);
    const f64 y1 = perlinLerp(dx, v010, v110);
    const f64 y2 = perlinLerp(dx, v001, v101);
    const f64 y3 = perlinLerp(dx, v011, v111);
    const f64 z0 = perlinLerp(dy, y0, y1);
    const f64 z1 = perlinLerp(dy, y2, y3);
    return perlinLerp(dz, z0, z1);
}

/// 钳位插值:delta<0→start,delta>1→end,否则 lerp。语义同 MC clampedLerp。
[[nodiscard]] inline f64 perlinClampedLerp(f64 start, f64 end, f64 delta)
{
    if (delta < 0.0) {
        return start;
    }
    return delta > 1.0 ? end : perlinLerp(delta, start, end);
}

/// 坐标环绕,防止大坐标精度丢失(2^25 周期)。等价 PerlinNoise::wrap。
[[nodiscard]] MC_PERLIN_ALWAYS_INLINE f64 perlinWrap(f64 value)
{
    constexpr f64 WRAP_PERIOD = 33554432.0;
    return value - std::floor(value / WRAP_PERIOD + 0.5) * WRAP_PERIOD;
}

/// 计算 f64 a 与 b 的 ULP(Units in the Last Place)距离。
/// 两个 f64 重解释为 i64 取差;异号时取绝对值之和(跨越零的路径)。
/// 用于 ULP 监控测试,量化 SoA 向量化路径与标量 reference 路径的浮点漂移。
[[nodiscard]] inline i64 ulpDistance(f64 a, f64 b)
{
    i64 ia = 0;
    i64 ib = 0;
    std::memcpy(&ia, &a, sizeof(ia));
    std::memcpy(&ib, &b, sizeof(ib));
    if ((ia < 0) != (ib < 0)) {
        return std::abs(ia) + std::abs(ib);
    }
    return std::abs(ia - ib);
}

/// 8 角梯度点乘:hash 链 perm[(perm[(perm[ax]+ay)&0xFF]+az)&0xFF] & 0xF,梯度取 kFlatSimplexGrad[hash<<2]。
/// 提为独立 inline 函数(非 lambda 捕获),便于 clang 跨 octave 内联向量化。
/// hash 链内部串行(数据依赖),跨 octave 并行由外层循环负责。
[[nodiscard]] MC_PERLIN_ALWAYS_INLINE f64 perlinGradDot(const u8* perm, i32 px, i32 py, i32 pz, f64 gx, f64 gy, f64 gz)
{
    const u32 ax = static_cast<u32>(px) & 0xFFu;
    const u32 ay = static_cast<u32>(py) & 0xFFu;
    const u32 az = static_cast<u32>(pz) & 0xFFu;
    const u32 hash = perm[(perm[(perm[ax] + ay) & 0xFFu] + az) & 0xFFu] & 0x0Fu;
    const f64* grad = &kFlatSimplexGrad[hash << 2u];
    return grad[0] * gx + grad[1] * gy + grad[2] * gz;
}

/**
 * @brief SoA 持有的单点 Perlin 采样内核(含 Y 涂抹)
 *
 * 数值 bit-exact 等价 PerlinLayer::noiseWithSmear(yScale!=0)与 PerlinLayer::noise(yScale==0)。
 * 排列表 perm 为 256 项 u8(已洗牌),查表用 & 0xFF 折回(等价 PerlinLayer::m_p 双倍表)。
 *
 * @param soa   PerlinNoiseSoA,持有所有 octave 的连续置换表 + 标量参数数组
 * @param i     octave 索引(0..soa.count-1)
 * @param x/y/z 已 wrap 的采样坐标(调用前需 wrap(coord*inputFactor))
 * @param yScale Y 涂抹间隔(0 = 不涂抹)
 * @param yMax 原始 Y 分数,控制吸附基准(NormalNoise 路径传 0)
 */
[[nodiscard]] MC_PERLIN_ALWAYS_INLINE f64 perlinSampleSoA(
    const PerlinNoiseSoA& soa, u32 i, f64 x, f64 y, f64 z, f64 yScale, f64 yMax)
{
    const u8* perm = soa.perms() + static_cast<size_t>(i) * 256ull;
    const f64 ox = soa.originX()[i];
    const f64 oy = soa.originY()[i];
    const f64 oz = soa.originZ()[i];

    const f64 d = x + ox;
    const f64 e = y + oy;
    const f64 f = z + oz;
    const i32 cellX = math::floorTo<i32>(d);
    const i32 cellY = math::floorTo<i32>(e);
    const i32 cellZ = math::floorTo<i32>(f);
    const f64 fracX = d - static_cast<f64>(cellX);
    const f64 fracY = e - static_cast<f64>(cellY);
    const f64 fracZ = f - static_cast<f64>(cellZ);

    // Y 涂抹:yScale!=0 时把 fracY 吸附到 yScale 间隔网格线(用 yMax 或 fracY 作基准)。
    // smoothstep 用原始 fracY,梯度点乘用吸附后的 (fracY - smearOffset)。
    // epsilon 必须用 static_cast<f64>(1.0e-7f)(float 字面量转 double),与 PerlinLayer::noiseWithSmear
    // 逐位一致——1.0e-7(double)与 1.0e-7f→double 值不同,边界附近 floor 会跨越整数致 smearOffset
    // 差一个 yScale 量级,远超 1e-9(NormalNoise 路径 yScale=0 不触发,BlendedNoise 路径必须严格一致)。
    f64 smearOffset = 0.0;
    if (yScale != 0.0) {
        const f64 base = (yMax >= 0.0 && yMax < fracY) ? yMax : fracY;
        smearOffset = std::floor(base / yScale + static_cast<f64>(1.0e-7f)) * yScale;
    }
    const f64 gradY = fracY - smearOffset;

    const f64 v000 = perlinGradDot(perm, cellX, cellY, cellZ, fracX, gradY, fracZ);
    const f64 v100 = perlinGradDot(perm, cellX + 1, cellY, cellZ, fracX - 1.0, gradY, fracZ);
    const f64 v010 = perlinGradDot(perm, cellX, cellY + 1, cellZ, fracX, gradY - 1.0, fracZ);
    const f64 v110 = perlinGradDot(perm, cellX + 1, cellY + 1, cellZ, fracX - 1.0, gradY - 1.0, fracZ);
    const f64 v001 = perlinGradDot(perm, cellX, cellY, cellZ + 1, fracX, gradY, fracZ - 1.0);
    const f64 v101 = perlinGradDot(perm, cellX + 1, cellY, cellZ + 1, fracX - 1.0, gradY, fracZ - 1.0);
    const f64 v011 = perlinGradDot(perm, cellX, cellY + 1, cellZ + 1, fracX, gradY - 1.0, fracZ - 1.0);
    const f64 v111 = perlinGradDot(perm, cellX + 1, cellY + 1, cellZ + 1, fracX - 1.0, gradY - 1.0, fracZ - 1.0);

    const f64 dx = perlinFade(fracX);
    const f64 dy = perlinFade(fracY);
    const f64 dz = perlinFade(fracZ);
    return perlinLerp3(dx, dy, dz, v000, v100, v010, v110, v001, v101, v011, v111);
}

} // namespace mc::world::gen::noise
