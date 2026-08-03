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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/buffer/RegistryByteBuf.hpp"

#include <array>
#include <cmath>

namespace mc::network::backend::java::wire {

/**
 * @brief Java 1.21.11 wire 层共享编解码工具
 *
 * 集中实现与 FriendlyByteBuf / ByteBufCodecs 行为一致、但项目 buffer 层未直接提供的
 * 复合编码原语：UUID、LpVec3（低精度变长 Vec3）、packed degrees、OptionalTag、
 * 容器 id（VarInt）等。Play codec 经这些工具组装字段。
 *
 * 注意：LpVec3 严格逐字节复刻 net.minecraft.network.LpVec3（含零向量特例、continuation
 * bit、6 字节起步 + 可选 VarInt 续传），是 AddEntity/SetEntityMotion 的 movement 编码。
 */

using B = buffer::RegistryByteBuf;

// ============================================================================
// UUID（128 位，2× long，大端）
// ============================================================================

inline void writeUuid(B& buf, const std::array<u8, 16>& uuid)
{
    buf.writeBytes(uuid.data(), uuid.size());
}

[[nodiscard]] inline Result<std::array<u8, 16>> readUuid(B& buf)
{
    std::array<u8, 16> uuid{};
    MC_TRY(buf.readBytes(uuid.data(), uuid.size()));
    return uuid;
}

// ============================================================================
// packed degrees（float → byte）
// 对应 Java Mth.packDegrees：把 0..360 度压成 0..255 的 byte。
// ============================================================================

[[nodiscard]] inline u8 packDegrees(f32 degrees) noexcept
{
    // Java: (byte)(int)(degrees * 256.0F / 360.0F)
    i32 v = static_cast<i32>(degrees * 256.0F / 360.0F);
    return static_cast<u8>(v);
}

[[nodiscard]] inline f32 unpackDegrees(u8 packed) noexcept
{
    // Java: (float)(packed * 360) / 256.0F
    return static_cast<f32>(packed) * 360.0F / 256.0F;
}

// ============================================================================
// LpVec3（低精度变长 Vec3）
// 逐字节复刻 net.minecraft.network.LpVec3。
// ============================================================================

namespace lpvec_detail {

inline constexpr double kAbsMaxValue = 1.7179869183E10;
inline constexpr double kAbsMinValue = 3.051944088384301E-5;
inline constexpr i64 kDataBitsMask = 32767;
inline constexpr double kMaxQuantizedValue = 32766.0;

[[nodiscard]] inline double sanitize(double v) noexcept
{
    if (v != v) { // NaN
        return 0.0;
    }
    if (v < -kAbsMaxValue) {
        return -kAbsMaxValue;
    }
    if (v > kAbsMaxValue) {
        return kAbsMaxValue;
    }
    return v;
}

[[nodiscard]] inline i64 pack(double v) noexcept
{
    // round((v * 0.5 + 0.5) * 32766.0)
    return static_cast<i64>(std::llround((v * 0.5 + 0.5) * kMaxQuantizedValue));
}

[[nodiscard]] inline double unpack(i64 v) noexcept
{
    double q = static_cast<double>(v & kDataBitsMask);
    if (q > kMaxQuantizedValue) {
        q = kMaxQuantizedValue;
    }
    return q * 2.0 / kMaxQuantizedValue - 1.0;
}

[[nodiscard]] inline i64 ceilLong(double v) noexcept
{
    i64 i = static_cast<i64>(v);
    return (v > static_cast<double>(i)) ? i + 1 : i;
}

[[nodiscard]] inline double absMax(double a, double b) noexcept
{
    double aa = a < 0 ? -a : a;
    double bb = b < 0 ? -b : b;
    return aa > bb ? aa : bb;
}

} // namespace lpvec_detail

/**
 * @brief 写 LpVec3（三个 double 分量）
 *
 * 零向量特例：max(|x|,|y|,|z|) < ABS_MIN_VALUE 时只写 1 字节 0x00。
 * 否则 6 字节起步（2× byte + 1× int 大端），scale 超 2 bit 时追加 VarInt。
 */
inline void writeLpVec3(B& buf, double x, double y, double z)
{
    using namespace lpvec_detail;
    const double d0 = sanitize(x);
    const double d1 = sanitize(y);
    const double d2 = sanitize(z);
    const double d3 = absMax(d0, absMax(d1, d2));
    if (d3 < kAbsMinValue) {
        buf.writeU8(0);
        return;
    }
    const i64 i = ceilLong(d3);
    const bool flag = (static_cast<u64>(i) & 0x03ULL) != static_cast<u64>(i);
    // scale 段：续传时低 2 bit + 置 continuation(4)；否则直接 i
    const i64 j = flag ? (i & 0x03LL) | 0x04LL : i;
    const i64 k = pack(d0 / static_cast<double>(i)) << 3;
    const i64 l = pack(d1 / static_cast<double>(i)) << 18;
    const i64 m = pack(d2 / static_cast<double>(i)) << 33;
    const u64 combined = static_cast<u64>(j | k | l | m);
    buf.writeU8(static_cast<u8>(combined));
    buf.writeU8(static_cast<u8>(combined >> 8));
    buf.writeU32(static_cast<u32>(combined >> 16)); // writeInt 大端
    if (flag) {
        buf.writeVarInt(static_cast<i32>(static_cast<u64>(i) >> 2));
    }
}

/**
 * @brief 读 LpVec3，返回三个 double 分量（经引用）
 */
[[nodiscard]] inline Result<void> readLpVec3(B& buf, double& outX, double& outY, double& outZ)
{
    using namespace lpvec_detail;
    u8 b0 = 0;
    MC_TRY_ASSIGN(b0, buf.readU8());
    if (b0 == 0) {
        outX = 0.0;
        outY = 0.0;
        outZ = 0.0;
        return Result<void>::ok();
    }
    u8 b1 = 0;
    MC_TRY_ASSIGN(b1, buf.readU8());
    u32 k = 0;
    MC_TRY_ASSIGN(k, buf.readU32()); // readUnsignedInt 大端 4 字节
    u64 l = (static_cast<u64>(k) << 16) | (static_cast<u64>(b1) << 8) | static_cast<u64>(b0);
    u64 scale = b0 & 0x03ULL;
    if ((b0 & 0x04ULL) != 0) {
        u32 cont = 0;
        MC_TRY_ASSIGN(cont, buf.readVarUInt());
        scale |= (static_cast<u64>(cont) & 0xFFFFFFFFULL) << 2;
    }
    const double s = static_cast<double>(scale);
    outX = unpack(static_cast<i64>(l >> 3)) * s;
    outY = unpack(static_cast<i64>(l >> 18)) * s;
    outZ = unpack(static_cast<i64>(l >> 33)) * s;
    return Result<void>::ok();
}

} // namespace mc::network::backend::java::wire
