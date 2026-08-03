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

// macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include "NibbleArray.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

NibbleArray::NibbleArray(std::vector<u8> data)
{
    if (data.size() != BYTE_SIZE && !data.empty()) {
        throw std::invalid_argument(
            "NibbleArray data must be " + std::to_string(BYTE_SIZE) + " bytes, got " + std::to_string(data.size()));
    }
    m_data = std::move(data);
}

NibbleArray NibbleArray::filled(u8 value)
{
    NibbleArray result;
    result.m_data.resize(BYTE_SIZE);

    // 每个字节存储两个相同的值
    u8 packed = (value & MAX_VALUE) | ((value & MAX_VALUE) << 4);
    std::fill(result.m_data.begin(), result.m_data.end(), packed);

    return result;
}

// ============================================================================
// 元素访问
// ============================================================================

u8 NibbleArray::get(i32 x, i32 y, i32 z) const
{
    return get(getIndex(x, y, z));
}

void NibbleArray::set(i32 x, i32 y, i32 z, u8 value)
{
    set(getIndex(x, y, z), value);
}

u8 NibbleArray::get(i32 index) const
{
    if (m_data.empty()) {
        return 0;
    }

    // 确保索引在有效范围内
    index &= 0xFFF; // 0-4095

    i32 byteIndex = getByteIndex(index);
    u8 byte = m_data[static_cast<size_t>(byteIndex)];

    if (isLowerNibble(index)) {
        // 偶数索引：低4位
        return byte & MAX_VALUE;
    } else {
        // 奇数索引：高4位
        return (byte >> 4) & MAX_VALUE;
    }
}

void NibbleArray::set(i32 index, u8 value)
{
    ensureAllocated();

    // 确保索引在有效范围内
    index &= 0xFFF; // 0-4095

    // 截断值到4位
    value &= MAX_VALUE;

    i32 byteIndex = getByteIndex(index);
    u8& byte = m_data[static_cast<size_t>(byteIndex)];

    if (isLowerNibble(index)) {
        // 偶数索引：设置低4位，保留高4位
        byte = (byte & 0xF0) | value;
    } else {
        // 奇数索引：设置高4位，保留低4位
        byte = (byte & 0x0F) | (value << 4);
    }
}

// ============================================================================
// 批量操作
// ============================================================================

void NibbleArray::fill(u8 value)
{
    ensureAllocated();

    // 每个字节存储两个相同的值
    u8 packed = (value & MAX_VALUE) | ((value & MAX_VALUE) << 4);
    std::fill(m_data.begin(), m_data.end(), packed);
}

NibbleArray NibbleArray::copy() const
{
    if (m_data.empty()) {
        return NibbleArray();
    }
    return NibbleArray(std::vector<u8>(m_data));
}

// ============================================================================
// 数据访问
// ============================================================================

std::vector<u8>& NibbleArray::data()
{
    ensureAllocated();
    return m_data;
}

// ============================================================================
// 工具方法
// ============================================================================

void NibbleArray::unpackIndex(i32 index, i32& x, i32& y, i32& z)
{
    index &= 0xFFF; // 0-4095
    x = index & 0xF;
    y = (index >> 8) & 0xF;
    z = (index >> 4) & 0xF;
}

// ============================================================================
// 私有方法
// ============================================================================

void NibbleArray::ensureAllocated()
{
    if (m_data.empty()) {
        m_data.resize(BYTE_SIZE, 0);
    }
}

} // namespace mc

#pragma pop_macro("BYTE_SIZE")
