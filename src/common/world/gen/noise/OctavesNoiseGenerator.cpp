#include "OctavesNoiseGenerator.hpp"
#include "../../../util/math/random/Random.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

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
    m_octaves.resize(static_cast<size_t>(octaveCount));

    // 参考 MC 的实现
    // 创建第一个噪声生成器
    m_octaves[0] = std::make_unique<ImprovedNoiseGenerator>(rng);

    // 为其他倍频创建噪声生成器
    for (i32 i = 1; i < octaveCount; ++i) {
        // 跳过一些随机数来确保不同的噪声
        rng.skip(262);
        m_octaves[static_cast<size_t>(i)] = std::make_unique<ImprovedNoiseGenerator>(rng);
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
        return m_octaves[static_cast<size_t>(index)].get();
    }
    return nullptr;
}

const ImprovedNoiseGenerator* OctavesNoiseGenerator::getOctave(i32 octave) const
{
    const i32 index = static_cast<i32>(m_octaves.size()) - 1 - octave;
    if (index >= 0 && index < static_cast<i32>(m_octaves.size())) {
        return m_octaves[static_cast<size_t>(index)].get();
    }
    return nullptr;
}

// ============================================================================
// PerlinNoiseGenerator 实现
// ============================================================================

PerlinNoiseGenerator::PerlinNoiseGenerator(u64 seed, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
    , m_maxOctave(maxOctave)
{
    math::Random rng(seed);
    initNoiseLevels(rng);
}

PerlinNoiseGenerator::PerlinNoiseGenerator(math::IRandom& rng, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
    , m_maxOctave(maxOctave)
{
    initNoiseLevels(rng);
}

void PerlinNoiseGenerator::initNoiseLevels(math::IRandom& rng)
{
    // 参考 MC PerlinNoiseGenerator 构造函数
    // int i = -p_i225881_2_.firstInt();  // i = -minOctave
    // int j = p_i225881_2_.lastInt();     // j = maxOctave
    // int k = i + j + 1;                  // k = count
    const i32 i = -m_minOctave;
    const i32 j = m_maxOctave;
    const i32 k = i + j + 1;

    m_noiseLevels.resize(static_cast<size_t>(k));

    // 创建第一个 Simplex 噪声生成器
    auto firstSimplex = std::make_unique<SimplexNoiseGenerator>(rng);

    // field_227463_c_ = 2^j (maxOctave)
    m_xFactor = static_cast<f32>(std::pow(2.0, static_cast<f64>(j)));
    // field_227462_b_ = 1 / (2^k - 1)
    m_yFactor = static_cast<f32>(1.0 / (std::pow(2.0, static_cast<f64>(k)) - 1.0));

    // 设置第 j 个倍频层（如果 j 在有效范围内）
    if (j >= 0 && j < k) {
        m_noiseLevels[static_cast<size_t>(j)] = std::move(firstSimplex);
    }

    // 为 j+1 到 k-1 的倍频层创建新的 Simplex 噪声生成器
    for (i32 idx = j + 1; idx < k; ++idx) {
        if (idx >= 0) {
            m_noiseLevels[static_cast<size_t>(idx)] = std::make_unique<SimplexNoiseGenerator>(rng);
        } else {
            // 跳过一些随机数
            rng.skip(262);
        }
    }

    // 如果 j > 0，为 0 到 j-1 的倍频层创建新的 Simplex 噪声生成器
    if (j > 0) {
        // 参考 MC：使用第一个 simplex 在偏移点的 3D 采样结果派生种子。
        // 这里使用 noise(0,0,0) 等价于采样 func_227464_a_(xo, yo, zo)。
        const f64 seedNoise = static_cast<f64>(firstSimplex->noise(0.0f, 0.0f, 0.0f));
        const i64 reseedValue = static_cast<i64>(seedNoise * static_cast<f64>(std::numeric_limits<i64>::max()));
        math::Random reseedRng(static_cast<u64>(reseedValue));

        for (i32 idx = j - 1; idx >= 0; --idx) {
            if (idx < k) {
                m_noiseLevels[static_cast<size_t>(idx)] = std::make_unique<SimplexNoiseGenerator>(reseedRng);
            } else {
                reseedRng.skip(262);
            }
        }
    }
}

f32 PerlinNoiseGenerator::noise(f32 x, f32 y, f32 z) const
{
    // 参考 MC 的实现：转换为 2D 采样
    return noiseAt(x, z, true);
}

f32 PerlinNoiseGenerator::noiseAt(f32 x, f32 z, bool useNoiseOffsets) const
{
    // 参考 MC PerlinNoiseGenerator.noiseAt
    f32 result = 0.0f;
    f32 xFactor = m_xFactor;
    f32 yFactor = m_yFactor;

    for (const auto& level : m_noiseLevels) {
        if (level) {
            // simplex.getValue(x * xFactor + offset, z * xFactor + offset) * yFactor
            f32 offsetX = useNoiseOffsets ? level->xOffset() : 0.0f;
            f32 offsetY = useNoiseOffsets ? level->yOffset() : 0.0f;

            result += static_cast<f32>(level->getValue(
                static_cast<f64>(x) * static_cast<f64>(xFactor) + static_cast<f64>(offsetX),
                static_cast<f64>(z) * static_cast<f64>(xFactor) + static_cast<f64>(offsetY)
            )) * yFactor;
        }

        xFactor /= 2.0f;
        yFactor *= 2.0f;
    }

    return result;
}

f32 PerlinNoiseGenerator::noise2D(f32 x, f32 z) const
{
    return noiseAt(x, z, true);
}

SimplexNoiseGenerator* PerlinNoiseGenerator::getOctave(i32 octave)
{
    const i32 index = m_maxOctave - octave;
    if (index >= 0 && index < static_cast<i32>(m_noiseLevels.size())) {
        return m_noiseLevels[static_cast<size_t>(index)].get();
    }
    return nullptr;
}

const SimplexNoiseGenerator* PerlinNoiseGenerator::getOctave(i32 octave) const
{
    const i32 index = m_maxOctave - octave;
    if (index >= 0 && index < static_cast<i32>(m_noiseLevels.size())) {
        return m_noiseLevels[static_cast<size_t>(index)].get();
    }
    return nullptr;
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

// 2D Simplex 梯度向量
constexpr f64 SIMPLEX_GRAD2D[8][2] = {
    {1.0, 1.0}, {-1.0, 1.0}, {1.0, -1.0}, {-1.0, -1.0},
    {1.0, 0.0}, {-1.0, 0.0}, {0.0, 1.0}, {0.0, -1.0}
};

// Simplex 斜切因子
// F2 = 0.5 * (sqrt(3.0) - 1.0) = 0.3660254037844386...
// G2 = (3.0 - sqrt(3.0)) / 6.0 = 0.2113248654051871...
constexpr f32 F2 = 0.36602540378443864676372317075294f;
constexpr f32 G2 = 0.21132486540518711774542545986184f;
constexpr f64 F2D = 0.36602540378443864676372317075294;
constexpr f64 G2D = 0.21132486540518711774542545986184;
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
        m_permutation[static_cast<size_t>(i)] = static_cast<u8>(i);
    }

    // Fisher-Yates 洗牌
    for (i32 i = 0; i < 256; ++i) {
        const u32 j = static_cast<u32>(i) + static_cast<u32>(rng.nextInt(256 - i));
        std::swap(m_permutation[static_cast<size_t>(i)], m_permutation[static_cast<size_t>(j)]);
    }

    // 复制到工作数组
    for (i32 i = 0; i < 256; ++i) {
        m_p[static_cast<size_t>(i)] = m_permutation[static_cast<size_t>(i)];
        m_p[static_cast<size_t>(i + 256)] = m_permutation[static_cast<size_t>(i)];
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

i32 SimplexNoiseGenerator::fastFloor(f64 x)
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
        const i32 gi0 = m_p[static_cast<size_t>(ii + m_p[static_cast<size_t>(jj + m_p[static_cast<size_t>(kk)])])] % 12;
        n0 = t0 * t0 * (SIMPLEX_GRAD[gi0][0] * x0 + SIMPLEX_GRAD[gi0][1] * y0 + SIMPLEX_GRAD[gi0][2] * z0);
    }

    f32 t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 >= 0.0f) {
        t1 *= t1;
        const i32 gi1 = m_p[static_cast<size_t>(ii + i1 + m_p[static_cast<size_t>(jj + j1 + m_p[static_cast<size_t>(kk + k1)])])] % 12;
        n1 = t1 * t1 * (SIMPLEX_GRAD[gi1][0] * x1 + SIMPLEX_GRAD[gi1][1] * y1 + SIMPLEX_GRAD[gi1][2] * z1);
    }

    f32 t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 >= 0.0f) {
        t2 *= t2;
        const i32 gi2 = m_p[static_cast<size_t>(ii + i2 + m_p[static_cast<size_t>(jj + j2 + m_p[static_cast<size_t>(kk + k2)])])] % 12;
        n2 = t2 * t2 * (SIMPLEX_GRAD[gi2][0] * x2 + SIMPLEX_GRAD[gi2][1] * y2 + SIMPLEX_GRAD[gi2][2] * z2);
    }

    f32 t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 >= 0.0f) {
        t3 *= t3;
        const i32 gi3 = m_p[static_cast<size_t>(ii + 1 + m_p[static_cast<size_t>(jj + 1 + m_p[static_cast<size_t>(kk + 1)])])] % 12;
        n3 = t3 * t3 * (SIMPLEX_GRAD[gi3][0] * x3 + SIMPLEX_GRAD[gi3][1] * y3 + SIMPLEX_GRAD[gi3][2] * z3);
    }

    // 缩放到 [-1, 1]
    return 32.0f * (n0 + n1 + n2 + n3);
}

f32 SimplexNoiseGenerator::noise2D(f32 x, f32 z) const
{
    return static_cast<f32>(getValue(static_cast<f64>(x), static_cast<f64>(z)));
}

f64 SimplexNoiseGenerator::getValue(f64 x, f64 z) const
{
    // 参考 MC SimplexNoiseGenerator.getValue
    // 斜切
    const f64 s = (x + z) * F2D;
    const i32 i = fastFloor(x + s);
    const i32 j = fastFloor(z + s);

    // 反斜切
    const f64 t = static_cast<f64>(i + j) * G2D;
    const f64 X0 = static_cast<f64>(i) - t;
    const f64 Z0 = static_cast<f64>(j) - t;

    const f64 x0 = x - X0;
    const f64 z0 = z - Z0;

    // 确定单纯形
    i32 i1, j1;
    if (x0 > z0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }

    const f64 x1 = x0 - static_cast<f64>(i1) + G2D;
    const f64 z1 = z0 - static_cast<f64>(j1) + G2D;
    const f64 x2 = x0 - 1.0 + 2.0 * G2D;
    const f64 z2 = z0 - 1.0 + 2.0 * G2D;

    // 哈希坐标
    const i32 ii = i & 255;
    const i32 jj = j & 255;

    // 计算贡献
    f64 n0 = 0.0, n1 = 0.0, n2 = 0.0;

    f64 t0 = 0.5 - x0 * x0 - z0 * z0;
    if (t0 >= 0.0) {
        t0 *= t0;
        const i32 gi0 = m_p[static_cast<size_t>(ii + m_p[static_cast<size_t>(jj)])] & 7;
        n0 = t0 * t0 * (SIMPLEX_GRAD2D[gi0][0] * x0 + SIMPLEX_GRAD2D[gi0][1] * z0);
    }

    f64 t1 = 0.5 - x1 * x1 - z1 * z1;
    if (t1 >= 0.0) {
        t1 *= t1;
        const i32 gi1 = m_p[static_cast<size_t>(ii + i1 + m_p[static_cast<size_t>(jj + j1)])] & 7;
        n1 = t1 * t1 * (SIMPLEX_GRAD2D[gi1][0] * x1 + SIMPLEX_GRAD2D[gi1][1] * z1);
    }

    f64 t2 = 0.5 - x2 * x2 - z2 * z2;
    if (t2 >= 0.0) {
        t2 *= t2;
        const i32 gi2 = m_p[static_cast<size_t>(ii + 1 + m_p[static_cast<size_t>(jj + 1)])] & 7;
        n2 = t2 * t2 * (SIMPLEX_GRAD2D[gi2][0] * x2 + SIMPLEX_GRAD2D[gi2][1] * z2);
    }

    // 缩放到 [-1, 1]，2D 版本的缩放因子是 70
    return 70.0 * (n0 + n1 + n2);
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

f64 SimplexNoiseGenerator::grad2D(i32 hash, f64 x, f64 z) const
{
    const i32 h = hash & 7;
    return SIMPLEX_GRAD2D[h][0] * x + SIMPLEX_GRAD2D[h][1] * z;
}

} // namespace mc
