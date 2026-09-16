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

#include "common/world/gen/noise/SimplexNoise.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/IRandom.hpp"
#include <utility>

namespace mc::world::gen::noise {

SimplexNoise::SimplexNoise(math::IRandom& rng)
{
    m_xOffset = rng.nextDouble() * 256.0;
    m_yOffset = rng.nextDouble() * 256.0;
    m_zOffset = rng.nextDouble() * 256.0;

    for (i32 i = 0; i < 256; ++i) {
        m_p[i] = i;
    }
    for (i32 i = 0; i < 256; ++i) {
        const i32 j = i + rng.nextInt(256 - i);
        std::swap(m_p[i], m_p[j]);
    }
    for (i32 i = 0; i < 256; ++i) {
        m_p[i + 256] = m_p[i];
    }
}

f64 SimplexNoise::getValue(f64 x, f64 y) const
{
    const f64 d0 = (x + y) * F2;
    const i32 i = math::floorTo<i32>(x + d0);
    const i32 j = math::floorTo<i32>(y + d0);
    const f64 d1 = static_cast<f64>(i + j) * G2;
    const f64 d2 = static_cast<f64>(i) - d1;
    const f64 d3 = static_cast<f64>(j) - d1;
    const f64 d4 = x - d2;
    const f64 d5 = y - d3;

    i32 k;
    i32 l;
    if (d4 > d5) {
        k = 1;
        l = 0;
    } else {
        k = 0;
        l = 1;
    }

    const f64 d6 = d4 - static_cast<f64>(k) + G2;
    const f64 d7 = d5 - static_cast<f64>(l) + G2;
    const f64 d8 = d4 - 1.0 + 2.0 * G2;
    const f64 d9 = d5 - 1.0 + 2.0 * G2;

    const i32 i1 = i & 0xFF;
    const i32 j1 = j & 0xFF;
    const i32 k1 = p(i1 + p(j1)) % 12;
    const i32 l1 = p(i1 + k + p(j1 + l)) % 12;
    const i32 i2 = p(i1 + 1 + p(j1 + 1)) % 12;

    f64 d10 = 0.5 - d4 * d4 - d5 * d5;
    const f64 n0 = (d10 < 0.0) ? 0.0 : (d10 *= d10, d10 * d10 * dot(GRADIENT[k1], d4, d5, 0.0));

    f64 d11 = 0.5 - d6 * d6 - d7 * d7;
    const f64 n1 = (d11 < 0.0) ? 0.0 : (d11 *= d11, d11 * d11 * dot(GRADIENT[l1], d6, d7, 0.0));

    f64 d12 = 0.5 - d8 * d8 - d9 * d9;
    const f64 n2 = (d12 < 0.0) ? 0.0 : (d12 *= d12, d12 * d12 * dot(GRADIENT[i2], d8, d9, 0.0));

    return 70.0 * (n0 + n1 + n2);
}

f64 SimplexNoise::getValue(f64 x, f64 y, f64 z) const
{
    constexpr f64 F3 = 1.0 / 3.0;
    constexpr f64 G3 = 1.0 / 6.0;

    const f64 d1 = (x + y + z) * F3;
    const i32 i = math::floorTo<i32>(x + d1);
    const i32 j = math::floorTo<i32>(y + d1);
    const i32 k = math::floorTo<i32>(z + d1);
    const f64 d2 = static_cast<f64>(i + j + k) * G3;
    const f64 d3 = static_cast<f64>(i) - d2;
    const f64 d4 = static_cast<f64>(j) - d2;
    const f64 d5 = static_cast<f64>(k) - d2;

    const f64 d6 = x - d3;
    const f64 d7 = y - d4;
    const f64 d8 = z - d5;

    i32 l;
    i32 i1;
    i32 j1;
    i32 k1;
    i32 l1;
    i32 i2;
    if (d6 >= d7) {
        if (d7 >= d8) {
            l = 1;
            i1 = 0;
            j1 = 0;
            k1 = 1;
            l1 = 1;
            i2 = 0;
        } else if (d6 >= d8) {
            l = 1;
            i1 = 0;
            j1 = 0;
            k1 = 1;
            l1 = 0;
            i2 = 1;
        } else {
            l = 0;
            i1 = 0;
            j1 = 1;
            k1 = 1;
            l1 = 0;
            i2 = 1;
        }
    } else if (d7 < d8) {
        l = 0;
        i1 = 0;
        j1 = 1;
        k1 = 0;
        l1 = 1;
        i2 = 1;
    } else if (d6 < d8) {
        l = 0;
        i1 = 1;
        j1 = 0;
        k1 = 0;
        l1 = 1;
        i2 = 1;
    } else {
        l = 0;
        i1 = 1;
        j1 = 0;
        k1 = 1;
        l1 = 1;
        i2 = 0;
    }

    const f64 d9 = d6 - static_cast<f64>(l) + G3;
    const f64 d10 = d7 - static_cast<f64>(i1) + G3;
    const f64 d11 = d8 - static_cast<f64>(j1) + G3;
    const f64 d12 = d6 - static_cast<f64>(k1) + 2.0 * G3;
    const f64 d13 = d7 - static_cast<f64>(l1) + 2.0 * G3;
    const f64 d14 = d8 - static_cast<f64>(i2) + 2.0 * G3;
    const f64 d15 = d6 - 1.0 + 0.5;
    const f64 d16 = d7 - 1.0 + 0.5;
    const f64 d17 = d8 - 1.0 + 0.5;

    const i32 j2 = i & 0xFF;
    const i32 k2 = j & 0xFF;
    const i32 l2 = k & 0xFF;
    const i32 i3 = p(j2 + p(k2 + p(l2))) % 12;
    const i32 j3 = p(j2 + l + p(k2 + i1 + p(l2 + j1))) % 12;
    const i32 k3 = p(j2 + k1 + p(k2 + l1 + p(l2 + i2))) % 12;
    const i32 l3 = p(j2 + 1 + p(k2 + 1 + p(l2 + 1))) % 12;

    const f64 n0 = getCornerNoise3D(i3, d6, d7, d8, 0.6);
    const f64 n1 = getCornerNoise3D(j3, d9, d10, d11, 0.6);
    const f64 n2 = getCornerNoise3D(k3, d12, d13, d14, 0.6);
    const f64 n3 = getCornerNoise3D(l3, d15, d16, d17, 0.6);

    return 32.0 * (n0 + n1 + n2 + n3);
}

i32 SimplexNoise::p(i32 index) const
{
    return m_p[index & 0xFF];
}

f64 SimplexNoise::dot(const i32 grad[3], f64 x, f64 y, f64 z)
{
    return static_cast<f64>(grad[0]) * x + static_cast<f64>(grad[1]) * y + static_cast<f64>(grad[2]) * z;
}

f64 SimplexNoise::getCornerNoise3D(i32 hash, f64 x, f64 y, f64 z, f64 radius) const
{
    f64 d = radius - x * x - y * y - z * z;
    if (d < 0.0) {
        return 0.0;
    }
    d *= d;
    return d * d * dot(GRADIENT[hash], x, y, z);
}

} // namespace mc::world::gen::noise
