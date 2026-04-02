#include "OctavesNoiseGenerator.hpp"
#include "../../../util/math/random/Random.hpp"
#include <cmath>
#include <algorithm>

namespace mc {

// ============================================================================
// OctavesNoiseGenerator 实现
// ============================================================================

OctavesNoiseGenerator::OctavesNoiseGenerator(u64 seed, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
    , m_maxOctave(maxOctave)
{
    math::Random rng(seed);
    initOctaves(rng);
}

OctavesNoiseGenerator::OctavesNoiseGenerator(math::IRandom& rng, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
    , m_maxOctave(maxOctave)
{
    initOctaves(rng);
}

void OctavesNoiseGenerator::initOctaves(math::IRandom& rng)
{
    const i32 octaveCount = m_maxOctave - m_minOctave + 1;
    m_octaves.resize(octaveCount);

    // 参考 MC 的实现
    // 创建第一个噪声生成器
    m_octaves[0] = std::make_unique<ImprovedNoiseGenerator>(rng);

    // 为其他倍频创建噪声生成器
    for (i32 i = 1; i < octaveCount; ++i) {
        // 跳过一些随机数来确保不同的噪声
        rng.skip(262);
        m_octaves[i] = std::make_unique<ImprovedNoiseGenerator>(rng);
    }

    // 计算振幅
    // 参考 MC: field_227460_b_ 和 field_227461_c_
    m_amplitudeLow = static_cast<f32>(std::pow(2.0, static_cast<f64>(-m_minOctave)));
    m_amplitudeHigh = static_cast<f32>(std::pow(2.0, static_cast<f64>(octaveCount - 1)) /
                      (std::pow(2.0, static_cast<f64>(octaveCount)) - 1.0));
}

f32 OctavesNoiseGenerator::noise(f32 x, f32 y, f32 z) const
{
    return getValue(x, y, z, 0.0f, 0.0f, false);
}

f32 OctavesNoiseGenerator::getValue(f32 x, f32 y, f32 z, f32 yScale, f32 yBound, bool fixY) const
{
    f32 result = 0.0f;
    f32 freq = m_amplitudeLow;
    f32 amp = m_amplitudeHigh;

    for (size_t i = 0; i < m_octaves.size(); ++i) {
        const auto& octave = m_octaves[i];
        if (octave) {
            // 保持精度
            const f32 px = maintainPrecision(x * freq);
            const f32 py = maintainPrecision(y * freq);
            const f32 pz = maintainPrecision(z * freq);

            // 采样噪声
            const f32 sample = octave->noise(
                px,
                fixY ? -octave->yOffset() : py,
                pz,
                yScale * freq,
                yBound * freq
            );

            result += amp * sample;
        }

        freq *= 2.0f;
        amp /= 2.0f;
    }

    return result;
}

f32 OctavesNoiseGenerator::noiseAt(f32 x, f32 y, f32 z, f32 scale) const
{
    return getValue(x, y, 0.0f, z, scale, false);
}

ImprovedNoiseGenerator* OctavesNoiseGenerator::getOctave(i32 octave)
{
    const i32 index = static_cast<i32>(m_octaves.size()) - 1 - octave;
    if (index >= 0 && index < static_cast<i32>(m_octaves.size())) {
        return m_octaves[index].get();
    }
    return nullptr;
}

const ImprovedNoiseGenerator* OctavesNoiseGenerator::getOctave(i32 octave) const
{
    const i32 index = static_cast<i32>(m_octaves.size()) - 1 - octave;
    if (index >= 0 && index < static_cast<i32>(m_octaves.size())) {
        return m_octaves[index].get();
    }
    return nullptr;
}

// ============================================================================
// PerlinNoiseGenerator 实现
// ============================================================================

PerlinNoiseGenerator::PerlinNoiseGenerator(u64 seed, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
{
    math::Random rng(seed);

    const i32 count = maxOctave - minOctave + 1;
    m_noiseLevels.resize(count);
    m_amplitudes.resize(count, 1.0f);

    for (i32 i = 0; i < count; ++i) {
        m_noiseLevels[i] = std::make_unique<ImprovedNoiseGenerator>(rng);
    }

    // 计算最大振幅
    m_maxAmplitude = 0.0f;
    for (f32 amp : m_amplitudes) {
        m_maxAmplitude += amp;
    }
}

PerlinNoiseGenerator::PerlinNoiseGenerator(math::IRandom& rng, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
{
    const i32 count = maxOctave - minOctave + 1;
    m_noiseLevels.resize(count);
    m_amplitudes.resize(count, 1.0f);

    for (i32 i = 0; i < count; ++i) {
        m_noiseLevels[i] = std::make_unique<ImprovedNoiseGenerator>(rng);
    }

    // 计算最大振幅
    m_maxAmplitude = 0.0f;
    for (f32 amp : m_amplitudes) {
        m_maxAmplitude += amp;
    }
}

f32 PerlinNoiseGenerator::noise(f32 x, f32 y, f32 z) const
{
    f32 result = 0.0f;
    f32 freq = 1.0f;
    f32 amp = 1.0f;

    for (size_t i = 0; i < m_noiseLevels.size(); ++i) {
        if (m_noiseLevels[i]) {
            result += m_amplitudes[i] * m_noiseLevels[i]->noise(
                x * freq,
                y * freq,
                z * freq
            ) * amp;
        }
        freq *= 2.0f;
        amp /= 2.0f;
    }

    return result / m_maxAmplitude;
}

f32 PerlinNoiseGenerator::noise2D(f32 x, f32 z) const
{
    return noise(x, 0.0f, z);
}

// ============================================================================
// SimplexNoiseGenerator 实现
// ============================================================================

// Simplex 噪声的梯度向量
constexpr f32 SIMPLEX_GRAD[12][3] = {
    {1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {-1.0f, -1.0f, 0.0f},
    {1.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, -1.0f},
    {0.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 1.0f}, {0.0f, 1.0f, -1.0f}, {0.0f, -1.0f, -1.0f}
};

// Simplex 斜切因子
// F2 = 0.5 * (sqrt(3.0) - 1.0) = 0.3660254037844386...
// G2 = (3.0 - sqrt(3.0)) / 6.0 = 0.2113248654051871...
constexpr f32 F2 = 0.36602540378443864676372317075294f;
constexpr f32 G2 = 0.21132486540518711774542545986184f;
constexpr f32 F3 = 1.0f / 3.0f;
constexpr f32 G3 = 1.0f / 6.0f;

SimplexNoiseGenerator::SimplexNoiseGenerator(u64 seed)
{
    math::Random rng(seed);
    initPermutation(rng);
}

SimplexNoiseGenerator::SimplexNoiseGenerator(math::IRandom& rng)
{
    initPermutation(rng);
}

void SimplexNoiseGenerator::initPermutation(math::IRandom& rng)
{
    // 初始化排列数组
    for (i32 i = 0; i < 256; ++i) {
        m_permutation[i] = static_cast<u8>(i);
    }

    // Fisher-Yates 洗牌
    for (i32 i = 0; i < 256; ++i) {
        const u32 j = static_cast<u32>(i) + static_cast<u32>(rng.nextInt(256 - i));
        std::swap(m_permutation[i], m_permutation[j]);
    }

    // 复制到工作数组
    for (i32 i = 0; i < 256; ++i) {
        m_p[i] = m_permutation[i];
        m_p[i + 256] = m_permutation[i];
    }

    // 设置随机偏移
    m_offset[0] = rng.nextFloat(0.0f, 256.0f);
    m_offset[1] = rng.nextFloat(0.0f, 256.0f);
    m_offset[2] = rng.nextFloat(0.0f, 256.0f);
}

i32 SimplexNoiseGenerator::fastFloor(f32 x)
{
    return static_cast<i32>(x > 0 ? x : x - 1);
}

f32 SimplexNoiseGenerator::noise(f32 x, f32 y, f32 z) const
{
    // 添加偏移
    x += m_offset[0];
    y += m_offset[1];
    z += m_offset[2];

    // 斜切输入空间以确定单元格
    const f32 s = (x + y + z) * F3;
    const i32 i = fastFloor(x + s);
    const i32 j = fastFloor(y + s);
    const i32 k = fastFloor(z + s);

    const f32 t = static_cast<f32>(i + j + k) * G3;
    const f32 X0 = static_cast<f32>(i) - t;
    const f32 Y0 = static_cast<f32>(j) - t;
    const f32 Z0 = static_cast<f32>(k) - t;

    const f32 x0 = x - X0;
    const f32 y0 = y - Y0;
    const f32 z0 = z - Z0;

    // 确定单纯形
    i32 i1, j1, k1;
    i32 i2, j2, k2;

    if (x0 >= y0) {
        if (y0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        } else if (x0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
        } else {
            i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
        }
    } else {
        if (y0 < z0) {
            i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
        } else if (x0 < z0) {
            i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
        } else {
            i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
    }

    const f32 x1 = x0 - static_cast<f32>(i1) + G3;
    const f32 y1 = y0 - static_cast<f32>(j1) + G3;
    const f32 z1 = z0 - static_cast<f32>(k1) + G3;
    const f32 x2 = x0 - static_cast<f32>(i2) + 2.0f * G3;
    const f32 y2 = y0 - static_cast<f32>(j2) + 2.0f * G3;
    const f32 z2 = z0 - static_cast<f32>(k2) + 2.0f * G3;
    const f32 x3 = x0 - 1.0f + 3.0f * G3;
    const f32 y3 = y0 - 1.0f + 3.0f * G3;
    const f32 z3 = z0 - 1.0f + 3.0f * G3;

    // 哈希坐标
    const i32 ii = i & 255;
    const i32 jj = j & 255;
    const i32 kk = k & 255;

    // 计算贡献
    f32 n0 = 0.0f, n1 = 0.0f, n2 = 0.0f, n3 = 0.0f;

    f32 t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 >= 0.0f) {
        t0 *= t0;
        const i32 gi0 = m_p[ii + m_p[jj + m_p[kk]]] % 12;
        n0 = t0 * t0 * (SIMPLEX_GRAD[gi0][0] * x0 + SIMPLEX_GRAD[gi0][1] * y0 + SIMPLEX_GRAD[gi0][2] * z0);
    }

    f32 t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 >= 0.0f) {
        t1 *= t1;
        const i32 gi1 = m_p[ii + i1 + m_p[jj + j1 + m_p[kk + k1]]] % 12;
        n1 = t1 * t1 * (SIMPLEX_GRAD[gi1][0] * x1 + SIMPLEX_GRAD[gi1][1] * y1 + SIMPLEX_GRAD[gi1][2] * z1);
    }

    f32 t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 >= 0.0f) {
        t2 *= t2;
        const i32 gi2 = m_p[ii + i2 + m_p[jj + j2 + m_p[kk + k2]]] % 12;
        n2 = t2 * t2 * (SIMPLEX_GRAD[gi2][0] * x2 + SIMPLEX_GRAD[gi2][1] * y2 + SIMPLEX_GRAD[gi2][2] * z2);
    }

    f32 t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 >= 0.0f) {
        t3 *= t3;
        const i32 gi3 = m_p[ii + 1 + m_p[jj + 1 + m_p[kk + 1]]] % 12;
        n3 = t3 * t3 * (SIMPLEX_GRAD[gi3][0] * x3 + SIMPLEX_GRAD[gi3][1] * y3 + SIMPLEX_GRAD[gi3][2] * z3);
    }

    // 缩放到 [-1, 1]
    return 32.0f * (n0 + n1 + n2 + n3);
}

f32 SimplexNoiseGenerator::noise2D(f32 x, f32 z) const
{
    return noise(x, 0.0f, z);
}

f32 SimplexNoiseGenerator::sampleEndHeight(i32 x, i32 z) const
{
    // 参考 MC EndBiomeProvider.func_235317_a_
    // 计算末地维度的高度偏移
    const i32 i = x / 2;
    const i32 j = z / 2;
    // 注: k 和 l 保留用于与原版采样流程一致的中间量
    (void)(x % 2);  // k
    (void)(z % 2);  // l

    // 使用 2D Simplex 噪声
    constexpr f32 SCALE = 0.05f;
    const f32 sample = noise2D(static_cast<f32>(i) * SCALE, static_cast<f32>(j) * SCALE);

    // 计算高度
    constexpr f32 BASE_HEIGHT = 8.0f;
    return static_cast<f32>((sample + 1.0f) * 0.5f * BASE_HEIGHT);
}

f32 SimplexNoiseGenerator::grad(i32 hash, f32 x, f32 y, f32 z) const
{
    const i32 h = hash & 11;
    return SIMPLEX_GRAD[h][0] * x + SIMPLEX_GRAD[h][1] * y + SIMPLEX_GRAD[h][2] * z;
}

} // namespace mc
