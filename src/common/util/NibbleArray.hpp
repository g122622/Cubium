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

// macOS系统头文件<mach/arm/vm_param.h>定义了BYTE_SIZE宏(=8)，
// 与NibbleArray::BYTE_SIZE静态常量冲突，在此push/undef屏蔽。
// 不在文件末尾pop_macro，确保include此头文件的代码不受影响。
#ifdef __APPLE__
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE
#endif

#include "../core/Types.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace mc {

/**
 * @brief 4位紧凑存储数组 (Nibble Array)
 *
 * 用于存储光照数据等0-15范围的值，每个元素仅占用半个字节。
 * 存储格式与Minecraft NibbleArray一致，用于区块光照存储。
 *
 * 参考: net.minecraft.world.chunk.NibbleArray
 *
 * 内存布局:
 * - 每个字节存储两个相邻元素
 * - 偶数索引存储在低4位，奇数索引存储在高4位
 * - 16x16x16区块段需要2048字节存储4096个值
 *
 * 用法示例:
 * @code
 * NibbleArray light;
 * light.set(5, 10, 3, 12);  // 设置位置(5,10,3)的光照为12
 * u8 value = light.get(5, 10, 3);  // 获取值为12
 * @endcode
 */
class NibbleArray {
public:
    /**
     * @brief 字节数组大小 (16*16*16 / 2 = 2048)
     */
    static constexpr size_t BYTE_SIZE = 2048;

    /**
     * @brief 元素数量 (16*16*16 = 4096)
     */
    static constexpr size_t VALUE_COUNT = 4096;

    /**
     * @brief 最大值 (4位最大值 = 15)
     */
    static constexpr u8 MAX_VALUE = 15;

    // ========================================================================
    // 构造函数
    // ========================================================================

    /**
     * @brief 默认构造函数，创建空数组
     *
     * 空数组不会分配内存，所有读取返回0。
     */
    NibbleArray() = default;

    /**
     * @brief 从现有数据构造
     * @param data 字节数组（必须为2048字节）
     * @throws std::invalid_argument 如果数据大小不正确
     */
    explicit NibbleArray(std::vector<u8> data);

    /**
     * @brief 创建并填充指定值
     * @param value 填充值 (0-15)
     */
    static NibbleArray filled(u8 value);

    // ========================================================================
    // 元素访问
    // ========================================================================

    /**
     * @brief 获取指定位置的值
     *
     * @param x X坐标 (0-15)
     * @param y Y坐标 (0-15)
     * @param z Z坐标 (0-15)
     * @return 值 (0-15)，如果数组为空返回0
     *
     * @note 坐标会自动取模，不会越界
     */
    [[nodiscard]] u8 get(i32 x, i32 y, i32 z) const;

    /**
     * @brief 设置指定位置的值
     *
     * @param x X坐标 (0-15)
     * @param y Y坐标 (0-15)
     * @param z Z坐标 (0-15)
     * @param value 值 (0-15)，大于15会被截断
     *
     * @note 如果数组为空，会自动分配内存
     */
    void set(i32 x, i32 y, i32 z, u8 value);

    /**
     * @brief 获取指定索引的值
     *
     * @param index 线性索引 (0-4095)
     * @return 值 (0-15)，如果数组为空返回0
     */
    [[nodiscard]] u8 get(i32 index) const;

    /**
     * @brief 设置指定索引的值
     *
     * @param index 线性索引 (0-4095)
     * @param value 值 (0-15)
     */
    void set(i32 index, u8 value);

    // ========================================================================
    // 批量操作
    // ========================================================================

    /**
     * @brief 用指定值填充整个数组
     *
     * @param value 填充值 (0-15)
     *
     * @note 如果数组为空，会自动分配内存
     */
    void fill(u8 value);

    /**
     * @brief 创建副本
     * @return 数组的深拷贝
     */
    [[nodiscard]] NibbleArray copy() const;

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 检查数组是否为空
     * @return 如果未分配内存返回true
     */
    [[nodiscard]] bool isEmpty() const noexcept { return m_data.empty(); }

    /**
     * @brief 检查数组是否已分配
     * @return 如果已分配内存返回true
     */
    [[nodiscard]] bool isValid() const noexcept { return !m_data.empty(); }

    // ========================================================================
    // 数据访问
    // ========================================================================

    /**
     * @brief 获取底层数据（只读）
     * @return 字节数组引用
     */
    [[nodiscard]] const std::vector<u8>& data() const noexcept { return m_data; }

    /**
     * @brief 获取底层数据（可变）
     * @return 字节数组引用
     *
     * @note 如果数组为空，会分配内存
     */
    [[nodiscard]] std::vector<u8>& data();

    /**
     * @brief 获取数据指针
     * @return 数据指针，如果为空返回nullptr
     */
    [[nodiscard]] const u8* rawData() const noexcept { return m_data.empty() ? nullptr : m_data.data(); }

    /**
     * @brief 获取数据指针（可变）
     * @return 数据指针，如果为空返回nullptr
     */
    [[nodiscard]] u8* rawData() noexcept { return m_data.empty() ? nullptr : m_data.data(); }

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 计算线性索引
     *
     * @param x X坐标 (0-15)
     * @param y Y坐标 (0-15)
     * @param z Z坐标 (0-15)
     * @return 线性索引 (0-4095)
     *
     * 索引公式: y * 256 + z * 16 + x (与MC一致)
     */
    [[nodiscard]] static constexpr i32 getIndex(i32 x, i32 y, i32 z)
    {
        return (y & 0xF) << 8 | (z & 0xF) << 4 | (x & 0xF);
    }

    /**
     * @brief 从线性索引解包坐标
     *
     * @param index 线性索引 (0-4095)
     * @param[out] x X坐标
     * @param[out] y Y坐标
     * @param[out] z Z坐标
     */
    static void unpackIndex(i32 index, i32& x, i32& y, i32& z);

private:
    /**
     * @brief 确保数据已分配
     */
    void ensureAllocated();

    /**
     * @brief 获取字节索引
     * @param index 线性索引
     * @return 字节数组中的索引
     */
    [[nodiscard]] static constexpr i32 getByteIndex(i32 index)
    {
        return index >> 1; // index / 2
    }

    /**
     * @brief 检查是否为低4位（偶数索引）
     * @param index 线性索引
     * @return 如果是低4位返回true
     */
    [[nodiscard]] static constexpr bool isLowerNibble(i32 index) { return (index & 1) == 0; }

    std::vector<u8> m_data;
};

} // namespace mc
