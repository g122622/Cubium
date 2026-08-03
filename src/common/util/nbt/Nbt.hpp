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
 * @file Nbt.hpp
 * @brief NBT (Named Binary Tag) 库 - Minecraft数据序列化格式
 *
 * 本库提供完整的NBT格式支持，包括：
 * - Java Edition: 大端序二进制格式
 * - Bedrock Edition网络: 小端序 + VarInt/Zigzag
 * - Bedrock Edition磁盘: 小端序二进制格式
 * - Mojangson: 文本格式（字符串表示）
 *
 * @example 读取Java版NBT文件
 * @code
 * std::ifstream file("level.dat", std::ios::binary);
 * file >> mc::nbt::contexts::java;
 * auto compound = mc::nbt::tags::compound_tag::read(file);
 * auto& levelName = compound->get<mc::nbt::tags::string_tag>("LevelName");
 * @endcode
 *
 * @example 写入NBT数据
 * @code
 * mc::nbt::tags::compound_tag tag;
 * tag.put("name", std::string("Test"));
 * tag.put("value", 42);
 * std::ofstream out("data.nbt", std::ios::binary);
 * out << mc::nbt::contexts::java << tag;
 * @endcode
 */

#pragma once

#include "core/Types.hpp"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ios>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// 平台字节序检测
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || defined(__BIG_ENDIAN__) || defined(__ARMEB__) || \
    defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#define MC_NBT_BIG_ENDIAN
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || \
    defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || \
    defined(__MINGW32__)
// 小端序平台 - 默认
#else
// 未知架构，假设小端序
#endif

namespace mc {
namespace nbt {

/**
 * @brief NBT标签类型枚举
 *
 * 对应Minecraft NBT格式的12种标签类型
 */
enum class TagId : u8 {
    End,       ///< 结束标记 (0x00)
    Byte,      ///< 字节标签 (0x01) - 8位有符号整数
    Short,     ///< 短整型标签 (0x02) - 16位有符号整数
    Int,       ///< 整型标签 (0x03) - 32位有符号整数
    Long,      ///< 长整型标签 (0x04) - 64位有符号整数
    Float,     ///< 单精度浮点标签 (0x05)
    Double,    ///< 双精度浮点标签 (0x06)
    ByteArray, ///< 字节数组标签 (0x07)
    String,    ///< 字符串标签 (0x08) - UTF-8编码
    List,      ///< 列表标签 (0x09) - 同类型元素列表
    Compound,  ///< 复合标签 (0x0A) - 键值对映射
    IntArray,  ///< 整型数组标签 (0x0B)
    LongArray  ///< 长整型数组标签 (0x0C)
};

// 保留原有命名以兼容
using tag_id = TagId;

} // namespace nbt
} // namespace mc

// std::to_string 重载声明
namespace std {
string to_string(mc::nbt::TagId tid);
} // namespace std

namespace mc {
namespace nbt {

/**
 * @brief 从输入流读取一个字符，遇到EOF抛出异常
 * @param input 输入流
 * @return 读取的字符
 * @throws std::runtime_error 如果到达EOF
 */
inline int cheof(std::istream& input)
{
    int value = input.get();
    if (value == EOF) throw std::runtime_error("Unexpected EOF while reading NBT data");
    return value;
}

/**
 * @brief 输出TagId到流
 */
std::ostream& operator<<(std::ostream& output, TagId tid);

/**
 * @brief 反转字节序
 */
template <typename number_t>
number_t reverse(number_t number)
{
    constexpr std::size_t n = sizeof(number_t);
    union {
        number_t number;
        std::uint8_t data[n];
    } tmp{number};
    for (std::size_t i = 0; i < n / 2; ++i) {
        std::uint8_t z = tmp.data[i];
        tmp.data[i] = tmp.data[n - 1 - i];
        tmp.data[n - 1 - i] = z;
    }
    return tmp.number;
}

/**
 * @brief 转换为网络字节序（大端）
 */
template <typename number_t>
number_t net_order(number_t number)
{
#ifndef MC_NBT_BIG_ENDIAN
    return reverse(number);
#else
    return number;
#endif
}

/**
 * @brief 转换为磁盘字节序（小端）
 */
template <typename number_t>
number_t disk_order(number_t number)
{
#ifdef MC_NBT_BIG_ENDIAN
    return reverse(number);
#else
    return number;
#endif
}

/**
 * @brief NBT序列化上下文
 *
 * 控制NBT数据的读写格式
 */
struct Context {
    /**
     * @brief 字节序
     */
    enum class Order {
        BigEndian,   ///< 大端序（Java Edition）
        LittleEndian ///< 小端序（Bedrock Edition）
    } order;

    /**
     * @brief 数据格式
     */
    enum class Format {
        Bin,       ///< 标准二进制格式
        Zigzag,    ///< Zigzag编码（VarInt + Zigzag，Bedrock网络）
        Mojangson, ///< 文本格式（Mojangson字符串表示）
        Zint       ///< Zint编码（有符号整数zigzag编码）
    } format;

    /**
     * @brief 从IO流获取当前上下文
     * @param ios IO流对象
     * @return 当前设置的上下文，如果未设置则返回Java Edition默认上下文
     */
    static const Context& get(std::ios_base& ios);

    /**
     * @brief 设置IO流的上下文
     * @param ios IO流对象
     */
    void set(std::ios_base& ios) const;
};

// 保留原有类型别名
using context = Context;

/**
 * @brief 从输入流设置上下文
 * @code
 * std::ifstream file("level.dat", std::ios::binary);
 * file >> mc::nbt::contexts::java;  // 设置为Java Edition格式
 * @endcode
 */
inline std::istream& operator>>(std::istream& input, const Context& ctxt)
{
    ctxt.set(input);
    return input;
}

/**
 * @brief 向输出流设置上下文
 * @code
 * std::ofstream out("data.nbt", std::ios::binary);
 * out << mc::nbt::contexts::java;  // 设置为Java Edition格式
 * @endcode
 */
inline std::ostream& operator<<(std::ostream& output, const Context& ctxt)
{
    ctxt.set(output);
    return output;
}

/**
 * @brief 预定义的NBT上下文
 */
namespace Contexts {

/**
 * @brief Java Edition格式（大端序二进制）
 * 用于Java版的level.dat、player.dat等文件
 */
inline const Context java{Context::Order::BigEndian, Context::Format::Bin};

/**
 * @brief Bedrock Edition网络格式（小端序 + Zigzag编码）
 * 用于Bedrock Edition的网络协议
 */
inline const Context bedrock_net{Context::Order::LittleEndian, Context::Format::Zigzag};

/**
 * @brief Bedrock Edition磁盘格式（小端序二进制）
 * 用于Bedrock版的level.dat等文件
 */
inline const Context bedrock_disk{Context::Order::LittleEndian, Context::Format::Bin};

/**
 * @brief KBT格式（大端序 + Zint编码）
 */
inline const Context kbt{Context::Order::BigEndian, Context::Format::Zint};

/**
 * @brief Mojangson文本格式
 * 用于命令行和调试输出的字符串表示
 */
inline const Context mojangson{Context::Order::BigEndian, Context::Format::Mojangson};

} // namespace Contexts

// 保留原有命名空间别名
namespace contexts = Contexts;

/**
 * @brief 根据上下文字节序调整数值
 */
template <typename number_t>
number_t correct_order(number_t number, const Context::Order order)
{
    switch (order) {
        case Context::Order::LittleEndian:
            return disk_order(number);
        case Context::Order::BigEndian:
        default:
            return net_order(number);
    }
}

/**
 * @brief 从输入流加载VarInt
 */
std::int32_t load_varint(std::istream& input);

/**
 * @brief 向输出流写入VarInt
 */
void dump_varint(std::ostream& output, std::int32_t value);

/**
 * @brief 从输入流加载VarLong
 */
std::int64_t load_varlong(std::istream& input);

/**
 * @brief 向输出流写入VarLong
 */
void dump_varlong(std::ostream& output, std::int64_t value);

/**
 * @brief 从输入流加载原始数值（按上下文字节序）
 */
template <typename number_t>
number_t load_flat(std::istream& input, const Context::Order order)
{
    constexpr std::size_t n = sizeof(number_t);
    union {
        number_t number;
        char data[n];
    } tmp;
    input.read(tmp.data, n);
    return correct_order(tmp.number, order);
}

/**
 * @brief 从输入流加载文本格式数值（特化版本）
 */
template <typename number_t>
number_t load_text(std::istream& input)
{
    return static_cast<number_t>(load_text<std::make_signed_t<number_t>>(input));
}

template <>
std::int8_t load_text<std::int8_t>(std::istream& input);

template <>
std::int16_t load_text<std::int16_t>(std::istream& input);

template <>
std::int32_t load_text<std::int32_t>(std::istream& input);

template <>
std::int64_t load_text<std::int64_t>(std::istream& input);

template <>
float load_text<float>(std::istream& input);

template <>
double load_text<double>(std::istream& input);

/**
 * @brief 根据上下文格式从输入流加载数值
 */
template <typename number_t>
number_t load(std::istream& input, const Context& ctxt)
{
    if (ctxt.format == Context::Format::Mojangson)
        return load_text<number_t>(input);
    else
        return load_flat<number_t>(input, ctxt.order);
}

template <>
std::int32_t load<std::int32_t>(std::istream& input, const Context& ctxt);

template <>
inline std::uint32_t load<std::uint32_t>(std::istream& input, const Context& ctxt)
{
    return static_cast<std::uint32_t>(load<std::int32_t>(input, ctxt));
}

template <>
std::int64_t load<std::int64_t>(std::istream& input, const Context& ctxt);

template <>
inline std::uint64_t load<std::uint64_t>(std::istream& input, const Context& ctxt)
{
    return static_cast<std::uint64_t>(load<std::int64_t>(input, ctxt));
}

/**
 * @brief 跳过空白字符
 */
void skip_space(std::istream& input);

/**
 * @brief 加载数组/列表大小
 */
std::size_t load_size(std::istream& input, const Context& ctxt);

/**
 * @brief 写入数组/列表大小
 */
void dump_size(std::ostream& output, const Context& ctxt, std::size_t size);

/**
 * @brief 扫描文本格式的序列
 */
template <typename F>
void scan_sequence_text(std::istream& input, F element_action)
{
    for (;;) {
        skip_space(input);
        int nextChar = cheof(input);
        char c = static_cast<char>(nextChar);
        if (c == ']') {
            break;
        }
        input.putback(c);
        element_action();
        skip_space(input);
        int next = cheof(input);
        switch (next) {
            case ',':
                continue;
            case ']':
                return;
            default:
                throw std::runtime_error(std::string("unexpected character: ") + static_cast<char>(next));
        }
    }
}

/**
 * @brief 加载文本格式数组
 */
template <typename number_t>
std::vector<number_t> load_array_text(std::istream& input);

/**
 * @brief 加载二进制格式数组
 */
template <typename number_t>
std::vector<number_t> load_array_bin(std::istream& input, const Context& ctxt)
{
    auto size = load_size(input, ctxt);
    std::vector<number_t> result;
    result.reserve(size);
    for (std::size_t i = 0; i < size; i++)
        result.emplace_back(load<number_t>(input, ctxt));
    return result;
}

/**
 * @brief 根据上下文格式加载数组
 */
template <typename number_t>
std::vector<number_t> load_array(std::istream& input, const Context& ctxt)
{
    if (ctxt.format == Context::Format::Mojangson)
        return load_array_text<number_t>(input);
    else
        return load_array_bin<number_t>(input, ctxt);
}

/**
 * @brief 写入原始数值（按上下文字节序）
 */
template <typename number_t>
void dump_flat(std::ostream& output, number_t number, const Context::Order order)
{
    constexpr std::size_t n = sizeof(number_t);
    union {
        number_t number;
        char data[n];
    } tmp{correct_order(number, order)};
    output.write(tmp.data, n);
}

/**
 * @brief 写入文本格式数值（特化版本）
 */
template <typename number_t>
void dump_text(std::ostream& output, number_t number)
{
    dump_text(output, std::make_signed_t<number_t>(number));
}

template <>
void dump_text<std::int8_t>(std::ostream& output, std::int8_t number);

template <>
void dump_text<std::int16_t>(std::ostream& output, std::int16_t number);

template <>
void dump_text<std::int32_t>(std::ostream& output, std::int32_t number);

template <>
void dump_text<std::int64_t>(std::ostream& output, std::int64_t number);

template <>
void dump_text<float>(std::ostream& output, float number);

template <>
void dump_text<double>(std::ostream& output, double number);

/**
 * @brief 根据上下文格式写入数值
 */
template <typename number_t>
void dump(std::ostream& output, number_t number, const Context& ctxt)
{
    if (ctxt.format == Context::Format::Mojangson)
        dump_text(output, number);
    else
        dump_flat(output, number, ctxt.order);
}

template <>
void dump<std::int32_t>(std::ostream& output, std::int32_t number, const Context& ctxt);

template <>
inline void dump<std::uint32_t>(std::ostream& output, std::uint32_t number, const Context& ctxt)
{
    dump(output, static_cast<std::int32_t>(number), ctxt);
}

template <>
void dump<std::int64_t>(std::ostream& output, std::int64_t number, const Context& ctxt);

template <>
inline void dump<std::uint64_t>(std::ostream& output, std::uint64_t number, const Context& ctxt)
{
    dump(output, static_cast<std::int64_t>(number), ctxt);
}

/**
 * @brief 写入文本格式数组
 */
template <typename number_t>
void dump_array_text(std::ostream& output, const std::vector<number_t>& array);

/**
 * @brief 写入二进制格式数组
 */
template <typename number_t>
void dump_array_bin(std::ostream& output, const std::vector<number_t>& array, const Context& ctxt)
{
    dump_size(output, ctxt, array.size());
    for (const auto& element : array)
        dump(output, element, ctxt);
}

/**
 * @brief 根据上下文格式写入数组
 */
template <typename number_t>
void dump_array(std::ostream& output, const std::vector<number_t>& array, const Context& ctxt)
{
    if (ctxt.format == Context::Format::Mojangson)
        dump_array_text(output, array);
    else
        dump_array_bin(output, array, ctxt);
}

/**
 * @brief 写入列表
 */
template <typename element_type, typename F>
void dump_list(std::ostream& output, TagId aid, const std::vector<element_type>& list, F action)
{
    const Context& ctxt = Context::get(output);
    if (ctxt.format == Context::Format::Mojangson) {
        output << '[';
    } else {
        output.put(static_cast<char>(aid));
        auto size = static_cast<std::size_t>(list.size());
        dump_size(output, ctxt, size);
    }
    auto iter = list.cbegin();
    auto end = list.cend();
    if (iter != end) {
        action(*iter, ctxt);
        for (++iter; iter != end; ++iter) {
            if (ctxt.format == Context::Format::Mojangson) output << ',';
            action(*iter, ctxt);
        }
    }
    if (ctxt.format == Context::Format::Mojangson) output << ']';
}

/**
 * @brief 加载列表
 */
template <typename tag_type, typename F>
std::unique_ptr<tag_type> load_list(std::istream& input, F action)
{
    const Context& ctxt = Context::get(input);
    auto ptr = std::make_unique<tag_type>();
    typename tag_type::value_type& result = ptr->value;
    if (ctxt.format != Context::Format::Mojangson) {
        std::size_t size = load_size(input, ctxt);
        result.reserve(size);
        for (std::size_t i = 0; i < size; i++)
            result.emplace_back(action(ctxt));
    } else {
        scan_sequence_text(input, [&] { result.emplace_back(action(ctxt)); });
    }
    result.shrink_to_fit();
    return ptr;
}

/**
 * @brief 推断文本格式的标签类型
 */
TagId deduce_tag(std::istream& input);

/**
 * @brief NBT标签命名空间
 */
namespace tags {

/**
 * @brief NBT标签基类
 */
struct tag {
    virtual TagId id() const noexcept = 0;
    virtual void write(std::ostream& output) const = 0;
    virtual std::unique_ptr<tag> copy() const = 0;

    /**
     * @brief 深度比较两个标签是否相等
     *
     * 递归比较标签内容。对于复合标签和列表标签，会递归比较所有子标签。
     * 对于浮点数标签（float_tag、double_tag），使用 IEEE 754 精确比较
     *（与 MC Java 的 record 自动 equals 行为一致）。
     *
     * @param other 要比较的另一个标签
     * @return true 如果两个标签类型和内容完全相同
     */
    virtual bool equals(const tag& other) const = 0;

    virtual ~tag() {}
};

template <TagId tid>
struct find_by;

template <typename value_type>
struct find_of;

std::unique_ptr<tag> read(TagId tid, std::istream& input);

template <typename tag_type>
std::unique_ptr<tag_type> cast(std::unique_ptr<tag>&& ptr)
{
    static_assert(std::is_base_of<tag, tag_type>::value);
    std::unique_ptr<tag_type> result(dynamic_cast<tag_type*>(ptr.release()));
    return result;
}

template <typename tag_type>
std::unique_ptr<const tag_type> cast(std::unique_ptr<const tag>& ptr)
{
    static_assert(std::is_base_of<tag, tag_type>::value);
    std::unique_ptr<const tag_type> result(dynamic_cast<const tag_type*>(ptr.release()));
    return result;
}

#define TAG_FIND(T)                       \
    template <>                           \
    struct find_by<T::tid> final {        \
        typedef T type;                   \
    };                                    \
    template <>                           \
    struct find_of<T::value_type> final { \
        typedef T type;                   \
    };

template <TagId tid>
using tag_by = typename find_by<tid>::type;

template <typename value_type>
using tag_of = typename find_of<value_type>::type;

/**
 * @brief 结束标签
 */
struct end_tag final : public tag {
    typedef std::nullptr_t value_type;
    static constexpr TagId tid = TagId::End;
    virtual TagId id() const noexcept override { return tid; }
    virtual void write(std::ostream& output) const override;
    virtual std::unique_ptr<tag> copy() const override;
    virtual bool equals(const tag& other) const override { return other.id() == TagId::End; }
};

inline end_tag end;
TAG_FIND(end_tag)

/**
 * @brief 数值标签模板
 */
template <TagId TID, typename number_t>
struct numeric_tag : public tag {
    typedef number_t value_type;
    value_type value;
    static constexpr TagId tid = TID;
    virtual TagId id() const noexcept override { return tid; }
    numeric_tag() = default;
    constexpr numeric_tag(value_type number)
        : value(number)
    {}
    static std::unique_ptr<numeric_tag> read(std::istream& input)
    {
        return std::make_unique<numeric_tag>(load<value_type>(input, Context::get(input)));
    }
    virtual void write(std::ostream& output) const override { dump(output, value, Context::get(output)); }
    virtual std::unique_ptr<tag> copy() const override { return std::make_unique<numeric_tag>(value); }
    virtual bool equals(const tag& other) const override
    {
        if (other.id() != tid) {
            return false;
        }
        return value == static_cast<const numeric_tag&>(other).value;
    }
};

#define NUMERIC_TAG(name, tid, number_t)     \
    using name = numeric_tag<tid, number_t>; \
    TAG_FIND(name)

NUMERIC_TAG(byte_tag, TagId::Byte, std::int8_t)
NUMERIC_TAG(short_tag, TagId::Short, std::int16_t)
NUMERIC_TAG(int_tag, TagId::Int, std::int32_t)
NUMERIC_TAG(long_tag, TagId::Long, std::int64_t)
NUMERIC_TAG(float_tag, TagId::Float, float)
NUMERIC_TAG(double_tag, TagId::Double, double)

#undef NUMERIC_TAG

/**
 * @brief 数组标签模板
 */
template <TagId TID, typename number_t, char prefix_c>
struct array_tag final : public tag {
    typedef number_t element_type;
    typedef std::vector<element_type> value_type;
    static constexpr char prefix = prefix_c;
    value_type value;
    static constexpr TagId tid = TID;
    virtual TagId id() const noexcept override { return tid; }
    array_tag() = default;
    array_tag(const value_type& array)
        : value(array)
    {}
    array_tag(value_type&& array)
        : value(std::move(array))
    {}
    static std::unique_ptr<array_tag> read(std::istream& input)
    {
        const Context& ctxt = Context::get(input);
        return std::make_unique<array_tag>(load_array<element_type>(input, ctxt));
    }
    virtual void write(std::ostream& output) const override
    {
        const Context& ctxt = Context::get(output);
        dump_array(output, value, ctxt);
    }
    virtual std::unique_ptr<tag> copy() const override { return std::make_unique<array_tag>(value); }
    virtual bool equals(const tag& other) const override
    {
        if (other.id() != tid) {
            return false;
        }
        return value == static_cast<const array_tag&>(other).value;
    }
};

#define ARRAY_TAG(name, tid, number_t, prefix)     \
    using name = array_tag<tid, number_t, prefix>; \
    TAG_FIND(name)

ARRAY_TAG(bytearray_tag, TagId::ByteArray, std::int8_t, 'B')
ARRAY_TAG(intarray_tag, TagId::IntArray, std::int32_t, 'I')
ARRAY_TAG(longarray_tag, TagId::LongArray, std::int64_t, 'L')

#undef ARRAY_TAG

/**
 * @brief 字符串标签
 */
struct string_tag final : public tag {
    typedef std::string value_type;
    value_type value;
    static constexpr TagId tid = TagId::String;
    virtual TagId id() const noexcept override { return TagId::String; }
    string_tag() = default;
    string_tag(const value_type& string);
    string_tag(value_type&& string);
    static std::unique_ptr<string_tag> read(std::istream& input);
    virtual void write(std::ostream& output) const override;
    virtual std::unique_ptr<tag> copy() const override;
    virtual bool equals(const tag& other) const override
    {
        if (other.id() != TagId::String) {
            return false;
        }
        return value == static_cast<const string_tag&>(other).value;
    }
};

TAG_FIND(string_tag)

struct tag_list_tag;

/**
 * @brief 列表标签基类
 */
struct list_tag : public tag {
    static constexpr TagId tid = TagId::List;
    virtual TagId id() const noexcept override { return TagId::List; }
    virtual TagId element_id() const noexcept = 0;
    virtual size_t size() const noexcept = 0;
    virtual bool heavy() const noexcept = 0;
    virtual std::unique_ptr<tag> operator[](size_t i) const = 0;
    static std::unique_ptr<list_tag> read(std::istream& input);
    virtual tag_list_tag as_tags() = 0;
    virtual bool equals(const tag& other) const override;
};

template <>
struct find_by<TagId::List> final {
    typedef list_tag type;
};

/**
 * @brief 通用列表标签（存储任意类型标签）
 */
struct tag_list_tag final : public list_tag {
    TagId eid;
    typedef std::unique_ptr<tag> element_type;
    typedef std::vector<element_type> value_type;
    value_type value;
    virtual TagId element_id() const noexcept override
    {
        return value.empty() ? eid : (eid != TagId::End ? eid : value[0]->id());
    }
    virtual size_t size() const noexcept override { return value.size(); }
    virtual bool heavy() const noexcept override { return true; }
    virtual std::unique_ptr<tag> operator[](size_t i) const override;
    tag_list_tag();
    explicit tag_list_tag(TagId tid);
    tag_list_tag(const tag_list_tag& other);
    tag_list_tag(const value_type& list, TagId tid = TagId::End);
    tag_list_tag(value_type&& list, TagId tid = TagId::End);
    virtual void write(std::ostream& output) const override;
    virtual std::unique_ptr<tag> copy() const override;
    virtual tag_list_tag as_tags() override;
};

template <TagId tid>
struct find_list_by;

template <typename value_type>
struct find_list_of;

template <TagId tid>
using list_by = typename find_list_by<tid>::type;

template <typename value_type>
using list_of = typename find_list_of<value_type>::type;

#define FIND_LIST_TAG(name)                   \
    template <>                               \
    struct find_list_by<name::eid> {          \
        typedef name type;                    \
    };                                        \
    template <>                               \
    struct find_list_of<name::element_type> { \
        typedef name type;                    \
    };

/**
 * @brief 空列表标签
 */
struct end_list_tag final : public list_tag {
    typedef end_tag tag_type;
    static_assert(std::is_base_of<tag, tag_type>::value);
    typedef typename end_tag::value_type element_type;
    typedef std::vector<element_type> value_type;
    static constexpr TagId eid = tag_type::tid;
    virtual TagId element_id() const noexcept override { return eid; }
    virtual size_t size() const noexcept override { return 0u; }
    virtual bool heavy() const noexcept override { return false; }
    virtual std::unique_ptr<tag> operator[](size_t i) const override;
    end_list_tag() = default;
    static std::unique_ptr<end_list_tag> read_content(std::istream& input);
    virtual void write(std::ostream& output) const override;
    virtual std::unique_ptr<tag> copy() const override;
    virtual tag_list_tag as_tags() override;
};

inline end_list_tag end_list;

FIND_LIST_TAG(end_list_tag)

/**
 * @brief 数值列表标签模板
 */
template <typename T>
struct number_list_tag final : public list_tag {
    typedef T tag_type;
    static_assert(std::is_base_of<tag, tag_type>::value);
    typedef typename T::value_type element_type;
    typedef std::vector<element_type> value_type;
    static constexpr TagId eid = tag_type::tid;
    virtual TagId element_id() const noexcept override { return eid; }
    value_type value;
    virtual size_t size() const noexcept override { return value.size(); }
    virtual bool heavy() const noexcept override { return false; }
    virtual std::unique_ptr<tag> operator[](size_t i) const override { return std::make_unique<tag_type>(value.at(i)); }
    number_list_tag() = default;
    number_list_tag(const value_type& list)
        : value(list)
    {}
    number_list_tag(value_type&& list)
        : value(std::move(list))
    {}
    static std::unique_ptr<number_list_tag> read_content(std::istream& input)
    {
        return load_list<number_list_tag>(
            input, [&input](const Context&) -> element_type { return load<element_type>(input, Context::get(input)); });
    }
    virtual void write(std::ostream& output) const override
    {
        dump_list(output, eid, value, [&output](const element_type& number, const Context& ctxt) {
            dump(output, number, ctxt);
        });
    }
    virtual std::unique_ptr<tag> copy() const override { return std::make_unique<number_list_tag>(*this); }
    virtual tag_list_tag as_tags() override
    {
        tag_list_tag result(eid);
        result.value.reserve(value.size());
        for (auto each : value)
            result.value.push_back(std::make_unique<tag_type>(each));
        value.clear();
        value.shrink_to_fit();
        return result;
    }
};

#define NUMBER_LIST_TAG(name, tag_type)     \
    using name = number_list_tag<tag_type>; \
    FIND_LIST_TAG(name)

NUMBER_LIST_TAG(byte_list_tag, byte_tag)
NUMBER_LIST_TAG(short_list_tag, short_tag)
NUMBER_LIST_TAG(int_list_tag, int_tag)
NUMBER_LIST_TAG(long_list_tag, long_tag)
NUMBER_LIST_TAG(float_list_tag, float_tag)
NUMBER_LIST_TAG(double_list_tag, double_tag)

#undef NUMBER_LIST_TAG

/**
 * @brief 数组列表标签模板
 */
template <typename T>
struct array_list_tag final : public list_tag {
    typedef T tag_type;
    static_assert(std::is_base_of<tag, tag_type>::value);
    typedef typename T::value_type element_type;
    typedef std::vector<element_type> value_type;
    static constexpr TagId eid = tag_type::tid;
    virtual TagId element_id() const noexcept override { return eid; }
    value_type value;
    virtual size_t size() const noexcept override { return value.size(); }
    virtual bool heavy() const noexcept override { return false; }
    virtual std::unique_ptr<tag> operator[](size_t i) const override { return std::make_unique<tag_type>(value.at(i)); }
    array_list_tag() = default;
    array_list_tag(const value_type& list)
        : value(list)
    {}
    array_list_tag(value_type&& list)
        : value(std::move(list))
    {}
    static std::unique_ptr<array_list_tag> read_content(std::istream& input)
    {
        return load_list<array_list_tag>(input, [&input](const Context& ctxt) -> element_type {
            return load_array<typename element_type::value_type>(input, ctxt);
        });
    }
    virtual void write(std::ostream& output) const override
    {
        dump_list(output, eid, value, [&output](const element_type& element, const Context& ctxt) {
            dump_array(output, element, ctxt);
        });
    }
    virtual std::unique_ptr<tag> copy() const override { return std::make_unique<array_list_tag>(*this); }
    virtual tag_list_tag as_tags() override
    {
        tag_list_tag result(eid);
        result.value.reserve(value.size());
        for (auto& each : value)
            result.value.push_back(std::make_unique<tag_type>(std::move(each)));
        value.clear();
        value.shrink_to_fit();
        return result;
    }
};

#define ARRAY_LIST_TAG(name, tag_type)     \
    using name = array_list_tag<tag_type>; \
    FIND_LIST_TAG(name)

ARRAY_LIST_TAG(bytearray_list_tag, bytearray_tag)
ARRAY_LIST_TAG(intarray_list_tag, intarray_tag)
ARRAY_LIST_TAG(longarray_list_tag, longarray_tag)

#undef ARRAY_LIST_TAG

/**
 * @brief 字符串列表标签
 */
struct string_list_tag final : public list_tag {
    typedef string_tag tag_type;
    typedef std::string element_type;
    typedef std::vector<element_type> value_type;
    static constexpr TagId eid = tag_type::tid;
    virtual TagId element_id() const noexcept override { return eid; }
    value_type value;
    virtual size_t size() const noexcept override { return value.size(); }
    virtual bool heavy() const noexcept override { return false; }
    virtual std::unique_ptr<tag> operator[](size_t i) const override;
    string_list_tag() = default;
    string_list_tag(const value_type& list);
    string_list_tag(value_type&& list);
    static std::unique_ptr<string_list_tag> read_content(std::istream& input);
    virtual void write(std::ostream& output) const override;
    virtual std::unique_ptr<tag> copy() const override;
    virtual tag_list_tag as_tags() override;
};

FIND_LIST_TAG(string_list_tag)

/**
 * @brief 嵌套列表标签
 */
struct list_list_tag final : public list_tag {
    typedef list_tag tag_type;
    typedef std::unique_ptr<tag_type> element_type;
    typedef std::vector<element_type> value_type;
    static constexpr TagId eid = tag_type::tid;
    virtual TagId element_id() const noexcept override { return eid; }
    value_type value;
    virtual size_t size() const noexcept override { return value.size(); }
    virtual bool heavy() const noexcept override { return false; }
    virtual std::unique_ptr<tag> operator[](size_t i) const override;
    list_list_tag() = default;
    list_list_tag(const list_list_tag& other);
    list_list_tag(const value_type& list);
    list_list_tag(value_type&& list);
    static std::unique_ptr<list_list_tag> read_content(std::istream& input);
    virtual void write(std::ostream& output) const override;
    virtual std::unique_ptr<tag> copy() const override;
    virtual tag_list_tag as_tags() override;
};

FIND_LIST_TAG(list_list_tag)

/**
 * @brief 复合标签（键值对映射）
 *
 * 最常用的NBT标签类型，用于存储实体、方块实体、世界数据等。
 *
 * @example
 * @code
 * compound_tag tag;
 * tag.put("name", std::string("Test"));
 * tag.put<int_tag>("value", 42);  // 显式指定类型
 * auto& pos = tag.tag<list_tag>("Pos");  // 获取或创建
 * @endcode
 */
struct compound_tag final : public tag {
    typedef std::map<std::string, std::unique_ptr<tag>> value_type;
    bool is_root = false;
    value_type value;
    static constexpr TagId tid = TagId::Compound;
    virtual TagId id() const noexcept override { return TagId::Compound; }
    compound_tag() = default;
    compound_tag(const compound_tag& other);
    explicit compound_tag(bool root);
    compound_tag(const value_type& map, bool root = false);
    compound_tag(value_type&& map, bool root = false);
    static std::unique_ptr<compound_tag> read(std::istream& input);
    virtual void write(std::ostream& output) const override;
    virtual std::unique_ptr<tag> copy() const override;
    compound_tag& operator=(const compound_tag& other);
    void make_heavy();

    /**
     * @brief 插入值（自动推断标签类型）
     */
    template <typename T>
    auto put(std::string&& name, T&& item)
    {
        using DecayedT = std::decay_t<T>;
        return value.insert_or_assign(std::move(name),
            std::make_unique<tag_of<DecayedT>>(static_cast<const typename tag_of<DecayedT>::value_type&>(item)));
    }

    template <typename T>
    auto put(const std::string& name, T&& item)
    {
        using DecayedT = std::decay_t<T>;
        return value.insert_or_assign(
            name, std::make_unique<tag_of<DecayedT>>(static_cast<const typename tag_of<DecayedT>::value_type&>(item)));
    }

    /**
     * @brief 获取值（标签类型版本）
     */
    template <typename TagT, typename = std::enable_if_t<std::is_base_of<::mc::nbt::tags::tag, TagT>::value>>
    typename TagT::value_type& get(const std::string& name)
    {
        return dynamic_cast<TagT&>(*value.at(name)).value;
    }

    template <typename TagT, typename = std::enable_if_t<std::is_base_of<::mc::nbt::tags::tag, TagT>::value>>
    const typename TagT::value_type& get(const std::string& name) const
    {
        return dynamic_cast<const TagT&>(*value.at(name)).value;
    }

    /**
     * @brief 获取或创建标签
     */
    template <typename T>
    typename tag_of<std::decay_t<T>>::value_type& tag(const std::string& name)
    {
        using TagType = tag_of<std::decay_t<T>>;
        auto iter = value.find(name);
        if (iter == value.end()) {
            auto ptr = std::make_unique<TagType>();
            typename TagType::value_type& result = ptr->value;
            value.emplace(name, std::move(ptr));
            return result;
        } else {
            return dynamic_cast<TagType&>(*iter->second).value;
        }
    }

    bool erase(const std::string& name);

    /**
     * @brief 深度比较两个复合标签是否相等
     *
     * 递归比较所有键值对。键集合必须完全相同（大小和每个键都必须存在），
     * 每个键对应的值也必须通过 tag::equals 递归比较相等。
     * 这与 MC Java 的 CompoundTag.equals 行为一致。
     */
    virtual bool equals(const struct tag& other) const override;
};

TAG_FIND(compound_tag)

/**
 * @brief 复合标签列表
 */
struct compound_list_tag final : public list_tag {
    typedef compound_tag tag_type;
    typedef tag_type element_type;
    typedef std::vector<element_type> value_type;
    static constexpr TagId eid = tag_type::tid;
    virtual TagId element_id() const noexcept override { return eid; }
    value_type value;
    virtual size_t size() const noexcept override { return value.size(); }
    virtual bool heavy() const noexcept override { return false; }
    virtual std::unique_ptr<tag> operator[](size_t i) const override;
    compound_list_tag() = default;
    compound_list_tag(const value_type& list);
    compound_list_tag(value_type&& list);
    static std::unique_ptr<compound_list_tag> read_content(std::istream& input);
    virtual void write(std::ostream& output) const override;
    virtual std::unique_ptr<tag> copy() const override;
    virtual tag_list_tag as_tags() override;
};

FIND_LIST_TAG(compound_list_tag)

#undef FIND_LIST_TAG
#undef TAG_FIND

/**
 * @brief 包装值为标签
 */
template <typename T>
tag_of<std::decay_t<T>> wrap(T&& value)
{
    return tag_of<std::decay_t<T>>(value);
}

/**
 * @brief 根据标签ID读取标签
 */
template <TagId tid>
std::unique_ptr<tag> read(std::istream& input)
{
    return tag_by<tid>::read(input);
}

} // namespace tags

// 类型别名 - 符合项目命名风格
using Tag = tags::tag;
using CompoundTag = tags::compound_tag;
using ListTag = tags::list_tag;
using StringTag = tags::string_tag;
using IntTag = tags::int_tag;
using LongTag = tags::long_tag;
using DoubleTag = tags::double_tag;
using FloatTag = tags::float_tag;
using ByteTag = tags::byte_tag;
using ShortTag = tags::short_tag;
using ByteArrayTag = tags::bytearray_tag;
using IntArrayTag = tags::intarray_tag;
using LongArrayTag = tags::longarray_tag;

/**
 * @brief 比较两个NBT标签是否深度相等
 *
 * 通过虚方法 tag::equals 实现多态深度比较。
 * 对于复合标签和列表标签，递归比较所有子标签。
 *
 * @param a 第一个标签
 * @param b 第二个标签
 * @return true 如果两个标签类型和内容完全相同
 */
inline bool operator==(const tags::tag& a, const tags::tag& b)
{
    return a.equals(b);
}

/**
 * @brief 比较两个NBT标签是否不相等
 */
inline bool operator!=(const tags::tag& a, const tags::tag& b)
{
    return !a.equals(b);
}

/**
 * @brief 比较两个复合标签是否深度相等
 *
 * 对 unique_ptr<CompoundTag> 的便捷比较运算符。
 */
inline bool operator==(const tags::compound_tag& a, const tags::compound_tag& b)
{
    return a.equals(b);
}

inline bool operator!=(const tags::compound_tag& a, const tags::compound_tag& b)
{
    return !a.equals(b);
}

/**
 * @brief 从输入流读取复合标签
 */
std::istream& operator>>(std::istream& input, tags::compound_tag& compound);

/**
 * @brief 向输出流写入标签
 */
std::ostream& operator<<(std::ostream& output, const tags::tag& tag);

// 数组文本格式读写模板（定义在末尾）
template <typename number_t>
std::vector<number_t> load_array_text_impl(std::istream& input)
{
    std::vector<number_t> result;
    skip_space(input);
    char a = static_cast<char>(cheof(input));
    if (a != '[') throw std::runtime_error("failed to open array tag");
    a = static_cast<char>(cheof(input));
    if (a != tags::tag_of<std::vector<number_t>>::prefix) throw std::runtime_error("wrong array tag type");
    a = static_cast<char>(cheof(input));
    if (a != ';') throw std::runtime_error("unexpected symbol in array tag");
    scan_sequence_text(input, [&] { result.push_back(load_text<number_t>(input)); });
    result.shrink_to_fit();
    return result;
}

template <typename number_t>
void dump_array_text_impl(std::ostream& output, const std::vector<number_t>& array)
{
    output << '[' << tags::tag_of<std::vector<number_t>>::prefix << ';';
    auto iter = array.cbegin();
    auto end = array.cend();
    if (iter == end) return;
    dump_text(output, *iter);
    for (++iter; iter != end; ++iter) {
        output << ',';
        dump_text(output, *iter);
    }
    output << ']';
}

// 特化声明
template <>
std::vector<std::int8_t> load_array_text<std::int8_t>(std::istream& input);
template <>
std::vector<std::int32_t> load_array_text<std::int32_t>(std::istream& input);
template <>
std::vector<std::int64_t> load_array_text<std::int64_t>(std::istream& input);

template <>
void dump_array_text<std::int8_t>(std::ostream& output, const std::vector<std::int8_t>& array);
template <>
void dump_array_text<std::int32_t>(std::ostream& output, const std::vector<std::int32_t>& array);
template <>
void dump_array_text<std::int64_t>(std::ostream& output, const std::vector<std::int64_t>& array);

} // namespace nbt
} // namespace mc

namespace std {

/**
 * @brief 标签转换为字符串（Mojangson格式）
 */
string to_string(const mc::nbt::tags::tag& tag);

} // namespace std
