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

/**
 * @file NbtInternal.hpp
 * @brief NBT库内部实现头文件
 *
 * 本文件包含NBT库的内部实现细节，不应被外部直接使用。
 */

#pragma once

#include "Nbt.hpp"
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <type_traits>

namespace mc {
namespace nbt {

/**
 * @brief 计算VarNum编码的最大字节数
 */
template <typename number_t>
constexpr std::size_t varnum_max()
{
    return sizeof(number_t) * 8 / 7 + 1;
}

/**
 * @brief 计算Zint编码的最大字节数（无符号版本）
 */
template <typename numeric>
constexpr std::enable_if_t<std::is_unsigned_v<numeric>, std::size_t> zint_max()
{
    return varnum_max<numeric>();
}

/**
 * @brief 计算Zint编码的最大字节数（有符号版本）
 */
template <typename numeric>
constexpr std::enable_if_t<std::is_signed_v<numeric>, std::size_t> zint_max()
{
    return (sizeof(numeric) * 8 + 1) / 7 + ((sizeof(numeric) * 8 + 1) % 7 > 0);
}

/**
 * @brief 从输入流加载VarNum
 */
template <typename number_t>
number_t load_varnum(std::istream& input)
{
    std::size_t numRead = 0;
    number_t value = 0;
    int read;
    do {
        read = cheof(input);
        number_t tmp = static_cast<number_t>(read & 0b01111111);
        value |= (tmp << (7 * numRead));
        numRead++;
        if (numRead > varnum_max<number_t>()) throw std::runtime_error("varint is too big");
    } while ((read & 0b10000000) != 0);
    return value;
}

/**
 * @brief 向输出流写入VarNum
 */
template <typename number_t>
void dump_varnum(std::ostream& output, number_t value)
{
    std::make_unsigned_t<number_t> uval = value;
    do {
        auto temp = static_cast<std::int8_t>(uval & 0b01111111u);
        uval >>= 7;
        if (uval != 0) temp |= 0b10000000;
        output.put(temp);
    } while (uval != 0);
}

/**
 * @brief 从输入流加载Zint（无符号版本）
 */
template <typename numeric>
std::enable_if_t<std::is_unsigned_v<numeric>, numeric> load_zint(std::istream& input)
{
    return load_varnum<numeric>(input);
}

/**
 * @brief 从输入流加载Zint（有符号版本）
 */
template <typename numeric>
std::enable_if_t<std::is_signed_v<numeric>, numeric> load_zint(std::istream& input)
{
    std::uint8_t read = cheof(input);
    bool sign = read & 1;
    numeric value = (read >> 1) & 0b00111111;
    unsigned numRead = 0;
    while ((read & 0b10000000) != 0) {
        read = cheof(input);
        numeric tmp = (read & 0b01111111);
        value |= (tmp << (7 * numRead + 6));
        ++numRead;
        if (numRead > zint_max<numeric>() - 1) throw std::runtime_error("szint is too big");
    }
    if (sign)
        return -value;
    else
        return value;
}

/**
 * @brief 向输出流写入Zint（无符号版本）
 */
template <typename numeric>
std::enable_if_t<std::is_unsigned_v<numeric>> dump_zint(std::ostream& output, numeric value)
{
    dump_varnum(output, value);
}

/**
 * @brief 向输出流写入Zint（有符号版本）
 */
template <typename numeric>
std::enable_if_t<std::is_signed_v<numeric>> dump_zint(std::ostream& output, numeric value)
{
    std::uint8_t sign = value < 0;
    std::make_unsigned_t<numeric> uval = sign ? -value : value;
    std::uint8_t temp = (static_cast<std::uint8_t>(uval & 0b00111111) << 1) | sign;
    uval >>= 6;
    if (uval != 0) temp |= 0b10000000;
    output.put(temp);
    while (uval != 0) {
        temp = static_cast<std::uint8_t>(uval & 0b01111111);
        uval >>= 7;
        if (uval != 0) temp |= 0b10000000;
        output.put(temp);
    }
}

} // namespace nbt
} // namespace mc
