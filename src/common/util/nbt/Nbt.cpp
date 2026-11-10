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
 * @file Nbt.cpp
 * @brief NBT库实现
 */

#include "NbtInternal.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <ios>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace nbt {

Result<void> skip_space(std::istream& input)
{
    for (;;) {
        i32 next = 0;
        MC_TRY_ASSIGN(next, cheof(input));
        if (!std::isspace(next)) {
            input.putback(next);
            return Result<void>::ok();
        }
    }
}

i32 context_id()
{
    static const i32 i = std::ios_base::xalloc();
    return i;
}

Context*& context_storage(std::ios_base& ios)
{
    i32 i = context_id();
    return reinterpret_cast<Context*&>(ios.pword(i));
}

const Context& Context::get(std::ios_base& ios)
{
    auto ctxt = context_storage(ios);
    if (ctxt == nullptr) {
        return Contexts::java;
    } else {
        return *ctxt;
    }
}

void ios_callback(std::ios_base::event event, std::ios_base& ios, i32)
{
    if (event == std::ios_base::event::erase_event) {
        delete context_storage(ios);
    }
}

void Context::set(std::ios_base& ios) const
{
    Context*& ctxt = context_storage(ios);
    if (ctxt == nullptr) {
        ios.register_callback(ios_callback, context_id());
        ctxt = new Context(*this);
    } else {
        *ctxt = *this;
    }
}

Result<std::int32_t> load_varint(std::istream& input)
{
    return load_varnum<std::int32_t>(input);
}

void dump_varint(std::ostream& output, std::int32_t value)
{
    dump_varnum(output, value);
}

Result<std::int64_t> load_varlong(std::istream& input)
{
    return load_varnum<std::int64_t>(input);
}

void dump_varlong(std::ostream& output, std::int64_t value)
{
    dump_varnum(output, value);
}

namespace {

/**
 * @brief 读取文本数值字面量中的十进制数字序列
 *
 * 数字之间允许出现下划线分隔符（例如 1_000）。下划线不允许出现在数字的开头或结尾：
 * 若下划线之后不是数字，则不消费该下划线，交由调用方按语法错误处理。
 *
 * @param input 输入流
 * @param literal 字面量缓冲区，读到的数字追加到末尾（下划线被丢弃）
 * @return 是否读到至少一个数字
 */
bool read_text_digits(std::istream& input, std::string& literal)
{
    bool hasDigit = false;
    for (;;) {
        i32 next = input.peek();
        if (next != EOF && std::isdigit(next) != 0) {
            literal.push_back(static_cast<char>(next));
            input.get();
            hasDigit = true;
            continue;
        }
        if (next == '_' && hasDigit) {
            input.get();
            i32 following = input.peek();
            if (following != EOF && std::isdigit(following) != 0) {
                continue;
            }
            input.putback('_');
        }
        break;
    }
    return hasDigit;
}

/**
 * @brief 读取文本数值字面量的数字主体（不含类型后缀）
 *
 * 覆盖符号、整数部分、小数部分与十进制指数三部分。
 *
 * 此处不能使用 std::istream::operator>> 读取浮点字面量：libc++ 的 num_get 为识别十六进制
 * 浮点会把 a-f 等字符当作数值的一部分消费掉，"3.14f" 这类带类型后缀的字面量会被整体取走，
 * 随后因无法完整转换而置位 failbit，使后续读取抛 EOF 异常（表现为
 * parseMojangson("{ratio:3.14f}") 返回 nullptr）。改为显式扫描字面量字符后再交由
 * strtof/strtod/strtoll 转换，行为不再依赖标准库对数值字符集合的定义。
 *
 * @param input 输入流
 * @param literal 输出字面量（指数统一写作 'e'，下划线已丢弃）
 * @return 字面量中是否包含数字
 */
bool read_text_number(std::istream& input, std::string& literal)
{
    const i32 sign = input.peek();
    if (sign == '+' || sign == '-') {
        literal.push_back(static_cast<char>(sign));
        input.get();
    }

    bool hasDigit = read_text_digits(input, literal);

    if (input.peek() == '.') {
        input.get();
        literal.push_back('.');
        hasDigit = read_text_digits(input, literal) || hasDigit;
    }

    const i32 exponentMark = input.peek();
    if (hasDigit && (exponentMark == 'e' || exponentMark == 'E')) {
        input.get();
        std::string exponent;
        const i32 exponentSign = input.peek();
        if (exponentSign == '+' || exponentSign == '-') {
            exponent.push_back(static_cast<char>(exponentSign));
            input.get();
        }
        if (read_text_digits(input, exponent)) {
            literal.push_back('e');
            literal += exponent;
        } else {
            // e/E 之后不是数字，说明它不是指数的一部分（例如 "1e"），把字符退回给调用方
            if (!exponent.empty()) {
                input.putback(exponent.front());
            }
            input.putback(static_cast<char>(exponentMark));
        }
    }

    return hasDigit;
}

/**
 * @brief 把文本整数转换为目标数值类型，并校验取值范围
 * @return 成功返回转换结果；字面量非法或超出目标类型范围时返回错误
 */
template <typename number_t>
Result<number_t> convert_text_integer(const std::string& literal)
{
    errno = 0;
    char* end = nullptr;
    const i64 parsed = std::strtoll(literal.c_str(), &end, 10);
    if (end == literal.c_str() || *end != '\0' || errno == ERANGE) {
        return Error(ErrorCode::InvalidData, "NBT: failed to parse number literal '" + literal + "'");
    }
    if (parsed < static_cast<i64>(std::numeric_limits<number_t>::min()) ||
        parsed > static_cast<i64>(std::numeric_limits<number_t>::max())) {
        return Error(ErrorCode::InvalidData, "NBT: number literal out of range for target type: '" + literal + "'");
    }
    return static_cast<number_t>(parsed);
}

inline bool is_valid_char(char c)
{
    return std::isalnum(c) || c == '-' || c == '_' || c == '+' || c == '.';
}

/**
 * @brief 窥探流中是否为 SNBT 布尔字面量 true/false（大小写不敏感）
 *
 * SNBT 规范把未加引号的 `true`/`false` 解析为 ByteTag 的 1b/0b，而非 StringTag。
 * 未加引号 token 的边界与 read_string_text 的对应分支一致，即连续 is_valid_char
 * 字符；因此 `trueblue` 这类以 true 开头的标识符不会被误判为布尔。
 *
 * 无论是否命中布尔，返回前都已把窥探的字符全部回退，流位置保持不变。
 *
 * @param input 输入流
 * @param outValue 命中布尔字面量时写入 1(true) / 0(false)
 * @param outLength 命中布尔字面量时写入 token 字符数（用于消费）
 * @return 是否命中布尔字面量
 */
bool peek_bool_literal(std::istream& input, std::int8_t& outValue, std::size_t& outLength)
{
    const i32 first = input.peek();
    if (first == EOF || first == '"' || first == '\'' || !is_valid_char(static_cast<char>(first))) {
        return false;
    }

    std::vector<char> token;
    for (;;) {
        const i32 c = input.peek();
        if (c == EOF || !is_valid_char(static_cast<char>(c))) {
            break;
        }
        token.push_back(static_cast<char>(input.get()));
    }

    // 回退整个 token，调用方（或后续读取）需要重新消费它。
    for (auto iter = token.crbegin(), end = token.crend(); iter != end; ++iter) {
        input.putback(*iter);
    }

    std::string lower;
    lower.reserve(token.size());
    for (char c : token) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (lower == "true") {
        outValue = 1;
        outLength = token.size();
        return true;
    }
    if (lower == "false") {
        outValue = 0;
        outLength = token.size();
        return true;
    }
    return false;
}

/**
 * @brief 读取文本格式整数并消费其类型后缀
 * @param input 输入流
 * @param suffix 类型后缀（如 'b'、's'、'l'，大小写均可），'\0' 表示该类型没有后缀
 */
template <typename number_t>
Result<number_t> load_text_integer(std::istream& input, char suffix)
{
    std::string literal;
    if (!read_text_number(input, literal)) {
        return Error(ErrorCode::InvalidData, "NBT: failed to parse number");
    }
    if (suffix != '\0') {
        const i32 next = input.peek();
        if (next != EOF && std::tolower(next) == suffix) {
            input.get();
        }
    }
    return convert_text_integer<number_t>(literal);
}

/**
 * @brief 读取文本格式浮点数并消费其类型后缀
 * @param input 输入流
 * @param suffix 类型后缀（'f' 或 'd'，大小写均可）
 * @param convert 转换函数（std::strtof 或 std::strtod）
 */
template <typename number_t>
Result<number_t> load_text_floating(std::istream& input, char suffix, number_t (*convert)(const char*, char**))
{
    std::string literal;
    if (!read_text_number(input, literal)) {
        return Error(ErrorCode::InvalidData, "NBT: failed to parse number");
    }
    if (suffix != '\0') {
        const i32 next = input.peek();
        if (next != EOF && std::tolower(next) == suffix) {
            input.get();
        }
    }
    char* end = nullptr;
    const number_t value = convert(literal.c_str(), &end);
    if (end == nullptr || *end != '\0') {
        return Error(ErrorCode::InvalidData, "NBT: failed to parse number literal '" + literal + "'");
    }
    return value;
}

} // namespace

template <>
Result<std::int8_t> load_text<std::int8_t>(std::istream& input)
{
    // SNBT 的 true/false 是布尔字面量，等价于 1b/0b，必须先于数字解析处理。
    std::int8_t boolValue = 0;
    std::size_t boolLength = 0;
    if (peek_bool_literal(input, boolValue, boolLength)) {
        for (std::size_t i = 0; i < boolLength; ++i) {
            input.get();
        }
        return boolValue;
    }
    return load_text_integer<std::int8_t>(input, 'b');
}

template <>
Result<std::int16_t> load_text<std::int16_t>(std::istream& input)
{
    return load_text_integer<std::int16_t>(input, 's');
}

template <>
Result<std::int32_t> load_text<std::int32_t>(std::istream& input)
{
    return load_text_integer<std::int32_t>(input, '\0');
}

template <>
Result<std::int64_t> load_text<std::int64_t>(std::istream& input)
{
    return load_text_integer<std::int64_t>(input, 'l');
}

template <>
Result<f32> load_text<f32>(std::istream& input)
{
    return load_text_floating<f32>(input, 'f', &std::strtof);
}

template <>
Result<f64> load_text<f64>(std::istream& input)
{
    return load_text_floating<f64>(input, 'd', &std::strtod);
}

template <>
Result<std::int32_t> load<std::int32_t>(std::istream& input, const Context& ctxt)
{
    switch (ctxt.format) {
        case Context::Format::Bin:
            return load_flat<std::int32_t>(input, ctxt.order);
        case Context::Format::Zigzag:
            return load_varint(input);
        case Context::Format::Mojangson:
            return load_text<std::int32_t>(input);
        case Context::Format::Zint:
        default:
            return load_zint<std::int32_t>(input);
    }
}

template <>
Result<std::int64_t> load<std::int64_t>(std::istream& input, const Context& ctxt)
{
    switch (ctxt.format) {
        case Context::Format::Bin:
            return load_flat<std::int64_t>(input, ctxt.order);
        case Context::Format::Zigzag:
            return load_varlong(input);
        case Context::Format::Mojangson:
            return load_text<std::int64_t>(input);
        case Context::Format::Zint:
        default:
            return load_zint<std::int64_t>(input);
    }
}

template <>
void dump<std::int32_t>(std::ostream& output, std::int32_t number, const Context& ctxt)
{
    switch (ctxt.format) {
        case Context::Format::Bin:
            return dump_flat(output, number, ctxt.order);
        case Context::Format::Zigzag:
            return dump_varint(output, number);
        case Context::Format::Mojangson:
            return dump_text(output, number);
        case Context::Format::Zint:
        default:
            return dump_zint(output, number);
    }
}

template <>
void dump<std::int64_t>(std::ostream& output, std::int64_t number, const Context& ctxt)
{
    switch (ctxt.format) {
        case Context::Format::Bin:
            return dump_flat(output, number, ctxt.order);
        case Context::Format::Zigzag:
            return dump_varlong(output, number);
        case Context::Format::Mojangson:
            return dump_text(output, number);
        case Context::Format::Zint:
        default:
            return dump_zint(output, number);
    }
}

template <>
void dump_text<std::int8_t>(std::ostream& output, std::int8_t number)
{
    output << i32(number) << 'b';
}

template <>
void dump_text<std::int16_t>(std::ostream& output, std::int16_t number)
{
    output << number << 's';
}

template <>
void dump_text<std::int32_t>(std::ostream& output, std::int32_t number)
{
    output << number;
}

template <>
void dump_text<std::int64_t>(std::ostream& output, std::int64_t number)
{
    output << number << 'l';
}

template <>
void dump_text<f32>(std::ostream& output, f32 number)
{
    output << number << 'f';
}

template <>
void dump_text<f64>(std::ostream& output, f64 number)
{
    // 双精度标签必须带 'd' 后缀：否则取值为整数时（如 3.0）会被打印成 "3"，重新解析时
    // 退化为 IntTag，文本往返丢失类型。解析端 load_text_floating<f64> 已接受 'd'/'D'。
    output << number << 'd';
}

Result<std::size_t> load_size(std::istream& input, const Context& ctxt)
{
    if (ctxt.format == Context::Format::Bin) {
        std::uint32_t value = 0;
        MC_TRY_ASSIGN(value, load_flat<std::uint32_t>(input, ctxt.order));
        return static_cast<std::size_t>(value);
    }
    std::int32_t value = 0;
    MC_TRY_ASSIGN(value, load_varint(input));
    if (value < 0) {
        return Error(ErrorCode::InvalidData, "NBT: negative element count declared");
    }
    return static_cast<std::size_t>(value);
}

void dump_size(std::ostream& output, const Context& ctxt, std::size_t size)
{
    auto sz = static_cast<std::uint32_t>(size);
    if (ctxt.format == Context::Format::Bin) {
        dump_flat(output, sz, ctxt.order);
    } else {
        dump_varint(output, sz);
    }
}

std::ostream& operator<<(std::ostream& output, TagId tid)
{
    return output << std::to_string(tid);
}

Result<TagId> deduce_tag(std::istream& input)
{
    MC_TRY(skip_space(input));
    i32 first = 0;
    MC_TRY_ASSIGN(first, cheof(input));
    char a = static_cast<char>(first);
    if (a == '}' || a == ']') {
        input.putback(a);
        return TagId::End;
    }
    if (a == '[') {
        i32 secondRead = 0;
        MC_TRY_ASSIGN(secondRead, cheof(input));
        char b = static_cast<char>(secondRead);
        TagId id;
        switch (b) {
            case 'B':
                id = TagId::ByteArray;
                break;
            case 'I':
                id = TagId::IntArray;
                break;
            case 'L':
                id = TagId::LongArray;
                break;
            default:
                id = TagId::List;
                break;
        }
        if (id != TagId::List) {
            const i32 c = input.peek();
            if (c != ';') {
                id = TagId::List;
            }
        }
        input.putback(b);
        input.putback(a);
        return id;
    }
    if (a == '{') {
        input.putback(a);
        return TagId::Compound;
    }
    if (std::isdigit(a) || a == '-' || a == '+') {
        std::string buffer(&a, 1);
        TagId deduced = TagId::Int; // 默认为Int类型
        for (;;) {
            i32 readChar = 0;
            MC_TRY_ASSIGN(readChar, cheof(input));
            char b = static_cast<char>(readChar);
            buffer.push_back(b);
            if (std::isdigit(b)) {
                continue;
            } else if (b == 'b' || b == 'B') {
                deduced = TagId::Byte;
            } else if (b == 's' || b == 'S') {
                deduced = TagId::Short;
            } else if (b == 'l' || b == 'L') {
                deduced = TagId::Long;
            } else if (b == 'f' || b == 'F') {
                deduced = TagId::Float;
            } else if (b == 'd' || b == 'D') {
                deduced = TagId::Double;
            } else if (b == 'e' || b == 'E') {
                i32 exponentRead = 0;
                MC_TRY_ASSIGN(exponentRead, cheof(input));
                char c = static_cast<char>(exponentRead);
                buffer.push_back(c);
                if (std::isdigit(c) || c == '-' || c == '+') {
                    for (;;) {
                        i32 digitRead = 0;
                        MC_TRY_ASSIGN(digitRead, cheof(input));
                        char d = static_cast<char>(digitRead);
                        buffer.push_back(d);
                        if (std::isdigit(d)) {
                            continue;
                        } else if (d == 'f' || d == 'F') {
                            deduced = TagId::Float;
                        } else {
                            deduced = TagId::Double;
                        }
                        break;
                    }
                } else {
                    deduced = TagId::Int;
                }
            } else if (b == '.') {
                for (;;) {
                    i32 fractionRead = 0;
                    MC_TRY_ASSIGN(fractionRead, cheof(input));
                    char c = static_cast<char>(fractionRead);
                    buffer.push_back(c);
                    if (std::isdigit(c)) {
                        continue;
                    } else if (c == 'e' || c == 'E') {
                        i32 exponentRead = 0;
                        MC_TRY_ASSIGN(exponentRead, cheof(input));
                        char d = static_cast<char>(exponentRead);
                        buffer.push_back(d);
                        if (std::isdigit(d) || d == '-' || d == '+') {
                            continue;
                        }
                    } else if (c == 'f' || c == 'F') {
                        deduced = TagId::Float;
                    } else {
                        deduced = TagId::Double;
                    }
                    break;
                }
            } else {
                deduced = TagId::Int;
            }
            break;
        }
        for (auto iter = buffer.crbegin(), end = buffer.crend(); iter != end; ++iter) {
            input.putback(*iter);
        }
        return deduced;
    }
    // 布尔字面量 true/false（大小写不敏感）在 SNBT 中解析为 ByteTag 的 1b/0b，而不是
    // StringTag。这里先窥探完整的未引用 token 再回退，保持与 read_string_text 的
    // 未引用分支（连续 is_valid_char 字符）完全一致的切分边界，避免把 "trueblue"
    // 之类以 true 开头的标识符误判为布尔。
    if (a == '"' || a == '\'' || is_valid_char(a)) {
        input.putback(a);
        std::int8_t boolValue = 0;
        std::size_t boolLength = 0;
        if (peek_bool_literal(input, boolValue, boolLength)) {
            return TagId::Byte;
        }
        return TagId::String;
    }
    input.putback(a);
    return TagId::End;
}

namespace tags {

Result<std::string> read_string_text(std::istream& input)
{
    MC_TRY(skip_space(input));
    i32 firstRead = 0;
    MC_TRY_ASSIGN(firstRead, cheof(input));
    char first = static_cast<char>(firstRead);
    if (first == '"' || first == '\'') {
        const char quote = first;
        std::string result;
        for (;;) {
            i32 c = 0;
            MC_TRY_ASSIGN(c, cheof(input));
            switch (c) {
                case '\'':
                    if (quote == '\'') {
                        return result;
                    }
                    result.push_back(static_cast<char>(c));
                    break;
                case '"':
                    if (quote == '"') {
                        return result;
                    }
                    result.push_back(static_cast<char>(c));
                    break;
                case '\\': {
                    i32 escaped = 0;
                    MC_TRY_ASSIGN(escaped, cheof(input));
                    switch (escaped) {
                        case '"':
                            result.push_back('"');
                            break;
                        case '\'':
                            result.push_back('\'');
                            break;
                        case '\\':
                            result.push_back('\\');
                            break;
                        case '/':
                            result.push_back('/');
                            break;
                        case 'b':
                            result.push_back('\b');
                            break;
                        case 'f':
                            result.push_back('\f');
                            break;
                        case 'n':
                            result.push_back('\n');
                            break;
                        case 'r':
                            result.push_back('\r');
                            break;
                        case 't':
                            result.push_back('\t');
                            break;
                        case 'u':
                            // TODO: 支持 SNBT 的 \uXXXX 转义（当前版本按未实现处理并报错）
                            return Error(ErrorCode::Unsupported, "NBT: unicode escape sequences are not supported yet");
                        default:
                            result.push_back('\\');
                            result.push_back(static_cast<char>(escaped));
                            break;
                    }
                    break;
                }
                default:
                    result.push_back(static_cast<char>(c));
                    break;
            }
        }
    } else {
        std::string result(&first, 1);
        for (;;) {
            i32 c = 0;
            MC_TRY_ASSIGN(c, cheof(input));
            if (is_valid_char(static_cast<char>(c))) {
                result.push_back(static_cast<char>(c));
            } else {
                input.putback(static_cast<char>(c));
                return result;
            }
        }
    }
}

Result<std::string> read_string_bin(std::istream& input, const Context& ctxt)
{
    std::size_t size = 0;
    if (ctxt.format == Context::Format::Zigzag || ctxt.format == Context::Format::Zint) {
        std::int32_t declared = 0;
        MC_TRY_ASSIGN(declared, load_varint(input));
        if (declared < 0) {
            return Error(ErrorCode::InvalidData, "NBT string: negative length declared");
        }
        size = static_cast<std::size_t>(declared);
    } else {
        std::uint16_t declared = 0;
        MC_TRY_ASSIGN(declared, load_flat<std::uint16_t>(input, ctxt.order));
        size = static_cast<std::size_t>(declared);
    }
    // Java 格式下长度是 uint16（≤65535，天然有界），但基岩格式（Zigzag/Zint）用 VarInt，
    // 可声明至 4GB。统一按"剩余可读字节数"校验，避免畸形输入诱导巨大分配。
    MC_TRY(validateBinaryElementCount(input, size, 1, "string"));
    std::string result;
    result.resize(size);
    input.read(result.data(), static_cast<std::streamsize>(size));
    // 读不满（流已耗尽）时 read 只置 failbit、不写满缓冲区，此处返回的是**部分未初始化**
    // 的字符串。必须显式判错：否则畸形输入可静默产出垃圾数据，且失败态会让后续所有
    // remainingStreamBytes 校验失效（见该函数的说明）。
    if (!input) {
        return Error(ErrorCode::InvalidData, "NBT string: stream ended before the declared length was read");
    }
    return result;
}

Result<std::string> read_string(std::istream& input, const Context& ctxt)
{
    if (ctxt.format == Context::Format::Mojangson) {
        return read_string_text(input);
    }
    return read_string_bin(input, ctxt);
}

void write_string_text(std::ostream& output, const std::string& string)
{
    output << '"';
    for (char c : string) {
        switch (c) {
            case '"':
                output << "\\\"";
                break;
            case '\\':
                output << "\\\\";
                break;
            case '\b':
                output << "\\b";
                break;
            case '\f':
                output << "\\f";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                output << c;
        }
    }
    output << '"';
}

void write_string(std::ostream& output, const std::string& string, const Context& ctxt)
{
    switch (ctxt.format) {
        case Context::Format::Bin:
            dump(output, static_cast<std::uint16_t>(string.size()), ctxt);
            break;
        case Context::Format::Mojangson:
            write_string_text(output, string);
            return;
        case Context::Format::Zigzag:
        case Context::Format::Zint:
        default:
            dump_varint(output, static_cast<std::int32_t>(string.size()));
            break;
    }
    output << string;
}

template <TagId tid>
Result<std::unique_ptr<tag>> read_bridge(TagId id, std::istream& input)
{
    if (id == tid) {
        auto result = read<tid>(input);
        if (result.failed()) {
            return result.error();
        }
        return std::unique_ptr<tag>(std::move(result).value());
    }
    return read_bridge<static_cast<TagId>(static_cast<std::int8_t>(tid) - 1)>(id, input);
}

template <>
Result<std::unique_ptr<tag>> read_bridge<TagId::End>(TagId, std::istream&)
{
    return Error(ErrorCode::InvalidData, "NBT: TAG_End is not a readable tag");
}

Result<std::unique_ptr<tag>> read(TagId tid, std::istream& input)
{
    return read_bridge<TagId::LongArray>(tid, input);
}

template <TagId tid>
Result<std::unique_ptr<list_tag>> read_list_content_bridge(TagId id, std::istream& input)
{
    if (id == tid) {
        auto result = list_by<tid>::read_content(input);
        if (result.failed()) {
            return result.error();
        }
        return std::unique_ptr<list_tag>(std::move(result).value());
    }
    return read_list_content_bridge<static_cast<TagId>(static_cast<std::int8_t>(tid) - 1)>(id, input);
}

template <>
Result<std::unique_ptr<list_tag>> read_list_content_bridge<TagId::End>(TagId id, std::istream& input)
{
    // id == End 是合法情况：NBT 空列表的元素类型字节就是 End(0)。
    if (id == TagId::End) {
        auto result = list_by<TagId::End>::read_content(input);
        if (result.failed()) {
            return result.error();
        }
        return std::unique_ptr<list_tag>(std::move(result).value());
    }
    // 其余情况说明该元素类型字节不是任何合法 TagId（分发自 LongArray 逐级降到 End）。
    // 该字节来自不可信输入，必须以错误返回，绝不能用断言终止进程。
    return Error(ErrorCode::InvalidData, "NBT: invalid list element tag id " + std::to_string(static_cast<i32>(id)));
}

Result<std::unique_ptr<list_tag>> read_list_content(TagId tid, std::istream& input)
{
    return read_list_content_bridge<TagId::LongArray>(tid, input);
}

void end_tag::write(std::ostream& output) const
{
    output.put('\0');
}

std::unique_ptr<tag> end_tag::copy() const
{
    return std::make_unique<end_tag>();
}

string_tag::string_tag(const value_type& string)
    : value(string)
{}

string_tag::string_tag(value_type&& string)
    : value(std::move(string))
{}

Result<std::unique_ptr<string_tag>> string_tag::read(std::istream& input)
{
    const Context& ctxt = Context::get(input);
    std::string value;
    MC_TRY_ASSIGN(value, read_string(input, ctxt));
    return std::make_unique<string_tag>(std::move(value));
}

void string_tag::write(std::ostream& output) const
{
    const Context& ctxt = Context::get(output);
    write_string(output, value, ctxt);
}

std::unique_ptr<tag> string_tag::copy() const
{
    return std::make_unique<string_tag>(value);
}

Result<std::unique_ptr<list_tag>> list_tag::read(std::istream& input)
{
    const Context& ctxt = Context::get(input);
    if (ctxt.format == Context::Format::Mojangson) {
        MC_TRY(skip_space(input));
        i32 openRead = 0;
        MC_TRY_ASSIGN(openRead, cheof(input));
        if (static_cast<char>(openRead) != '[') {
            return Error(ErrorCode::InvalidData, "NBT: failed to open list tag");
        }
        TagId type = TagId::End;
        MC_TRY_ASSIGN(type, deduce_tag(input));
        return read_list_content(type, input);
    }
    i32 typeRead = 0;
    MC_TRY_ASSIGN(typeRead, cheof(input));
    const TagId type = static_cast<TagId>(typeRead);
    return read_list_content(type, input);
}

std::unique_ptr<tag> tag_list_tag::operator[](size_t i) const
{
    return value.at(i)->copy();
}

tag_list_tag::tag_list_tag()
    : eid(TagId::End)
{}

tag_list_tag::tag_list_tag(TagId tid)
    : eid(tid)
{}

tag_list_tag::tag_list_tag(const tag_list_tag& other)
    : eid(other.eid)
{
    value.reserve(other.size());
    for (const auto& each : other.value) {
        value.emplace_back(each->copy());
    }
}

tag_list_tag::tag_list_tag(const value_type& list, TagId tid)
    : eid(tid)
{
    value.reserve(list.size());
    for (const auto& each : list) {
        value.emplace_back(each->copy());
    }
}

tag_list_tag::tag_list_tag(value_type&& list, TagId tid)
    : eid(tid)
    , value(std::move(list))
{}

void tag_list_tag::write(std::ostream& output) const
{
    TagId aid = element_id();
    dump_list(output, aid, value, [&output, aid](const std::unique_ptr<tag>& tag, const Context&) {
        // 列表元素类型与声明的元素 ID 不一致属于容器构造错误（编程错误），断言暴露。
        MC_ASSERT_RELEASE_MSG(aid == tag->id(), "NBT: list element tag id does not match the declared element id");
        tag->write(output);
    });
}

std::unique_ptr<tag> tag_list_tag::copy() const
{
    return std::make_unique<tag_list_tag>(*this);
}

tag_list_tag tag_list_tag::as_tags()
{
    return std::move(*this);
}

std::unique_ptr<tag> end_list_tag::operator[](size_t) const
{
    // 空列表恒无元素，越界取元素属于调用方违约，断言暴露。
    MC_ASSERT_RELEASE_MSG(false, "NBT: end list tag is always empty; indexing it is invalid");
    return nullptr;
}

Result<std::unique_ptr<end_list_tag>> end_list_tag::read_content(std::istream& input)
{
    const Context& ctxt = Context::get(input);
    if (ctxt.format != Context::Format::Mojangson) {
        // NBT 二进制 List 格式恒为 etid(1, 已由 list_tag::read 消费) + size(4) + elements。
        // TAG_End 列表元素类型为 End，规范要求 size==0，但 size 字段本身仍占 4 字节。
        // 此前实现仅读 1 字节并断言为 0，遗留 3 字节使外层 Compound 把它误读为 End 提前结束，
        // 丢失后续键——基岩 .mcstructure 的 structure.entities 为 List[End] n=0 时致 palette 键丢失，
        // 结构模板退化为空，GameTest 按钮识别/方块放置全失效。
        std::size_t size = 0;
        MC_TRY_ASSIGN(size, load_size(input, ctxt));
        if (size != 0) {
            return Error(ErrorCode::InvalidData, "NBT: TAG_End list must declare zero elements");
        }
    }
    return std::make_unique<end_list_tag>();
}

void end_list_tag::write(std::ostream& output) const
{
    const Context& ctxt = Context::get(output);
    if (ctxt.format == Context::Format::Mojangson) {
        output << "[]";
    } else {
        // 与 read_content 对称：etid(1) + size(4)=0。此前只写 1 字节 0，缺 3 字节，
        // 与 read 的同源 bug 配对（读 1 写 1 自洽但不符合 NBT 规范，且与其它 list 实现不一致）。
        output.put(static_cast<char>(eid));
        dump_size(output, ctxt, 0);
    }
}

std::unique_ptr<tag> end_list_tag::copy() const
{
    return std::make_unique<end_list_tag>();
}

tag_list_tag end_list_tag::as_tags()
{
    return tag_list_tag(TagId::End);
}

std::unique_ptr<tag> string_list_tag::operator[](size_t i) const
{
    return std::make_unique<string_tag>(value.at(i));
}

string_list_tag::string_list_tag(const value_type& list)
    : value(list)
{}

string_list_tag::string_list_tag(value_type&& list)
    : value(std::move(list))
{}

Result<std::unique_ptr<string_list_tag>> string_list_tag::read_content(std::istream& input)
{
    return load_list<string_list_tag>(input, [&input](const Context& ctxt) -> Result<std::string> {
        return read_string(input, ctxt);
    });
}

void string_list_tag::write(std::ostream& output) const
{
    TagId aid = element_id();
    dump_list(output, aid, value, [&output](const std::string& string, const Context& ctxt) {
        write_string(output, string, ctxt);
    });
}

std::unique_ptr<tag> string_list_tag::copy() const
{
    return std::make_unique<string_list_tag>(*this);
}

tag_list_tag string_list_tag::as_tags()
{
    tag_list_tag result(eid);
    result.value.reserve(value.size());
    for (auto& each : value) {
        result.value.push_back(std::make_unique<tag_type>(std::move(each)));
    }
    value.clear();
    value.shrink_to_fit();
    return result;
}

std::unique_ptr<tag> list_list_tag::operator[](size_t i) const
{
    return value.at(i)->copy();
}

list_list_tag::list_list_tag(const list_list_tag& other)
{
    value.reserve(other.value.size());
    for (const auto& element : other.value) {
        value.emplace_back(cast<list_tag>(element->copy()));
    }
}

list_list_tag::list_list_tag(const value_type& list)
{
    value.reserve(list.size());
    for (const auto& element : list) {
        value.emplace_back(cast<list_tag>(element->copy()));
    }
}

list_list_tag::list_list_tag(value_type&& list)
    : value(std::move(list))
{}

Result<std::unique_ptr<list_list_tag>> list_list_tag::read_content(std::istream& input)
{
    return load_list<list_list_tag>(input, [&input](const Context&) -> Result<element_type> {
        return list_tag::read(input);
    });
}

void list_list_tag::write(std::ostream& output) const
{
    dump_list(output, eid, value, [&output](const element_type& list, const Context&) {
        list->write(output);
    });
}

std::unique_ptr<tag> list_list_tag::copy() const
{
    return std::make_unique<list_list_tag>(*this);
}

tag_list_tag list_list_tag::as_tags()
{
    tag_list_tag result(eid);
    result.value.reserve(value.size());
    for (auto& each : value) {
        result.value.push_back(std::move(each));
    }
    value.clear();
    value.shrink_to_fit();
    return result;
}

compound_tag::compound_tag(const compound_tag& other)
    : is_root(other.is_root)
{
    for (const auto& pair : other.value) {
        value.emplace(pair.first, pair.second->copy());
    }
}

compound_tag::compound_tag(bool root)
    : is_root(root)
{}

compound_tag::compound_tag(const value_type& map, bool root)
    : is_root(root)
{
    for (const auto& pair : map) {
        value.emplace(pair.first, pair.second->copy());
    }
}

compound_tag::compound_tag(value_type&& map, bool root)
    : is_root(root)
    , value(std::move(map))
{}

Result<std::unique_ptr<compound_tag>> compound_tag::read(std::istream& input)
{
    auto ptr = std::make_unique<compound_tag>();
    MC_TRY(readCompoundInto(input, *ptr));
    return ptr;
}

void compound_write_text(std::ostream& output, const compound_tag& compound, const Context& ctxt)
{
    const auto& value = compound.value;
    output << '{';
    auto iter = value.cbegin();
    auto end = value.cend();
    if (iter != end) {
        auto action = [&](const compound_tag::value_type::value_type& entry) {
            write_string(output, entry.first, ctxt);
            output << ':';
            entry.second->write(output);
        };
        action(*iter);
        for (++iter; iter != end; ++iter) {
            output << ',';
            action(*iter);
        }
    }
    output << '}';
}

void compound_write_bin(std::ostream& output, const compound_tag& compound, const Context& ctxt)
{
    const auto& value = compound.value;
    for (const auto& entry : value) {
        output.put(static_cast<char>(entry.second->id()));
        write_string(output, entry.first, ctxt);
        entry.second->write(output);
    }
    if (!compound.is_root) {
        tags::end.write(output);
    }
}

void compound_tag::write(std::ostream& output) const
{
    const Context& ctxt = Context::get(output);
    if (ctxt.format == Context::Format::Mojangson) {
        compound_write_text(output, *this, ctxt);
    } else {
        compound_write_bin(output, *this, ctxt);
    }
}

std::unique_ptr<tag> compound_tag::copy() const
{
    return std::make_unique<compound_tag>(*this);
}

compound_tag& compound_tag::operator=(const compound_tag& other)
{
    if (this == &other) {
        return *this;
    }

    is_root = other.is_root;
    // 整体拷贝赋值：目标原有的键必须被覆盖（而不是保留旧值），
    // 因此逐键 insert_or_assign 而非 emplace
    value.clear();
    for (const auto& pair : other.value) {
        value.emplace(pair.first, pair.second->copy());
    }
    return *this;
}

void compound_tag::make_heavy()
{
    for (auto& pair : value) {
        tags::tag& each = *pair.second;
        if (each.id() == TagId::List) {
            list_tag& list = dynamic_cast<list_tag&>(each);
            if (!list.heavy()) {
                pair.second = std::make_unique<tag_list_tag>(list.as_tags());
            }
        }
    }
}

std::unique_ptr<tag> compound_list_tag::operator[](size_t i) const
{
    return value.at(i).copy();
}

compound_list_tag::compound_list_tag(const value_type& list)
    : value(list)
{}

compound_list_tag::compound_list_tag(value_type&& list)
    : value(std::move(list))
{}

Result<std::unique_ptr<compound_list_tag>> compound_list_tag::read_content(std::istream& input)
{
    return load_list<compound_list_tag>(input, [&input](const Context&) -> Result<compound_tag> {
        compound_tag element;
        MC_TRY(readCompoundInto(input, element));
        return element;
    });
}

void compound_list_tag::write(std::ostream& output) const
{
    dump_list(output, eid, value, [&output](const element_type& element, const Context&) {
        element.write(output);
    });
}

std::unique_ptr<tag> compound_list_tag::copy() const
{
    return std::make_unique<compound_list_tag>(*this);
}

tag_list_tag compound_list_tag::as_tags()
{
    tag_list_tag result(eid);
    result.value.reserve(value.size());
    for (auto& each : value) {
        each.make_heavy();
        result.value.push_back(std::make_unique<tag_type>(std::move(each)));
    }
    value.clear();
    value.shrink_to_fit();
    return result;
}

bool compound_tag::erase(const std::string& name)
{
    return value.erase(name) > 0;
}

bool list_tag::equals(const tag& other) const
{
    if (other.id() != TagId::List) {
        return false;
    }

    const auto& otherList = static_cast<const list_tag&>(other);

    // 元素类型必须相同
    if (element_id() != otherList.element_id()) {
        return false;
    }

    // 元素数量必须相同
    if (size() != otherList.size()) {
        return false;
    }

    // 按顺序逐一比较元素（与 MC Java 的 ListTag.equals 行为一致）
    for (size_t i = 0; i < size(); ++i) {
        auto thisElem = (*this)[i];
        auto otherElem = otherList[i];
        if (!thisElem->equals(*otherElem)) {
            return false;
        }
    }

    return true;
}

bool compound_tag::equals(const struct tag& other) const
{
    if (other.id() != TagId::Compound) {
        return false;
    }

    const auto& otherCompound = static_cast<const compound_tag&>(other);

    // 键集合大小必须相同
    if (value.size() != otherCompound.value.size()) {
        return false;
    }

    // 逐一比较每个键值对
    for (const auto& [key, val] : value) {
        auto it = otherCompound.value.find(key);
        if (it == otherCompound.value.end()) {
            // 键不存在
            return false;
        }

        // 递归比较值
        if (!val->equals(*it->second)) {
            return false;
        }
    }

    return true;
}

} // namespace tags

Result<void> read_compound_text(std::istream& input, tags::compound_tag& compound, const Context& ctxt)
{
    MC_TRY(skip_space(input));
    i32 openRead = 0;
    MC_TRY_ASSIGN(openRead, cheof(input));
    if (static_cast<char>(openRead) != '{') {
        return Error(ErrorCode::InvalidData, "NBT: failed to open compound tag");
    }
    for (;;) {
        TagId keyType = TagId::End;
        MC_TRY_ASSIGN(keyType, deduce_tag(input));
        switch (keyType) {
            case TagId::End: {
                MC_TRY(skip_space(input));
                i32 closeRead = 0;
                MC_TRY_ASSIGN(closeRead, cheof(input));
                if (static_cast<char>(closeRead) != '}') {
                    return Error(ErrorCode::InvalidData, "NBT: failed to close compound tag");
                }
                return Result<void>::ok();
            }
            case TagId::String:
                break;
            default:
                return Error(ErrorCode::InvalidData,
                    "NBT: invalid compound key type (expected string, got " +
                        std::to_string(static_cast<i32>(keyType)) + ")");
        }
        std::string key;
        MC_TRY_ASSIGN(key, tags::read_string(input, ctxt));
        MC_TRY(skip_space(input));
        i32 delimiterRead = 0;
        MC_TRY_ASSIGN(delimiterRead, cheof(input));
        if (static_cast<char>(delimiterRead) != ':') {
            return Error(ErrorCode::InvalidData,
                std::string("NBT: key-value delimiter expected in compound, got '") + static_cast<char>(delimiterRead) +
                    "'");
        }
        TagId valueType = TagId::End;
        MC_TRY_ASSIGN(valueType, deduce_tag(input));
        if (valueType == TagId::End) {
            return Error(ErrorCode::InvalidData, "NBT: compound value expected");
        }
        std::unique_ptr<tags::tag> value;
        MC_TRY_ASSIGN(value, tags::read(valueType, input));
        compound.value.emplace(std::move(key), std::move(value));
        MC_TRY(skip_space(input));
        i32 nextRead = 0;
        MC_TRY_ASSIGN(nextRead, cheof(input));
        const char next = static_cast<char>(nextRead);
        if (next == '}') {
            return Result<void>::ok();
        }
        if (next != ',' && next != ';') {
            return Error(
                ErrorCode::InvalidData, std::string("NBT: next tag or end expected in compound, got '") + next + "'");
        }
    }
}

Result<void> read_compound_bin(std::istream& input, tags::compound_tag& compound, const Context& ctxt)
{
    for (;;) {
        const auto idNumeric = input.get();
        if (!input) {
            compound.is_root = true;
            break;
        }
        const auto id = static_cast<TagId>(idNumeric);
        if (id > TagId::LongArray) {
            return Error(ErrorCode::InvalidData,
                "NBT: unknown tag id " + std::to_string(static_cast<i32>(idNumeric)) + " in compound");
        }
        if (id == TagId::End) {
            break;
        }
        std::string key;
        MC_TRY_ASSIGN(key, tags::read_string(input, ctxt));
        std::unique_ptr<tags::tag> value;
        MC_TRY_ASSIGN(value, tags::read(id, input));
        compound.value.emplace(std::move(key), std::move(value));
    }
    return Result<void>::ok();
}

Result<void> readCompoundInto(std::istream& input, tags::compound_tag& compound)
{
    const Context& ctxt = Context::get(input);
    if (ctxt.format == Context::Format::Mojangson) {
        return read_compound_text(input, compound, ctxt);
    }
    return read_compound_bin(input, compound, ctxt);
}

std::ostream& operator<<(std::ostream& output, const tags::tag& tag)
{
    tag.write(output);
    return output;
}

// 数组文本格式特化
template <>
Result<std::vector<std::int8_t>> load_array_text<std::int8_t>(std::istream& input)
{
    return load_array_text_impl<std::int8_t>(input);
}

template <>
Result<std::vector<std::int32_t>> load_array_text<std::int32_t>(std::istream& input)
{
    return load_array_text_impl<std::int32_t>(input);
}

template <>
Result<std::vector<std::int64_t>> load_array_text<std::int64_t>(std::istream& input)
{
    return load_array_text_impl<std::int64_t>(input);
}

template <>
void dump_array_text<std::int8_t>(std::ostream& output, const std::vector<std::int8_t>& array)
{
    dump_array_text_impl(output, array);
}

template <>
void dump_array_text<std::int32_t>(std::ostream& output, const std::vector<std::int32_t>& array)
{
    dump_array_text_impl(output, array);
}

template <>
void dump_array_text<std::int64_t>(std::ostream& output, const std::vector<std::int64_t>& array)
{
    dump_array_text_impl(output, array);
}

} // namespace nbt
} // namespace mc

namespace std {

string to_string(mc::nbt::TagId tid)
{
#define TAG_ID_CASE(name)      \
    case mc::nbt::TagId::name: \
        return #name
    switch (tid) {
        TAG_ID_CASE(End);
        TAG_ID_CASE(Byte);
        TAG_ID_CASE(Short);
        TAG_ID_CASE(Int);
        TAG_ID_CASE(Long);
        TAG_ID_CASE(Float);
        TAG_ID_CASE(Double);
        TAG_ID_CASE(ByteArray);
        TAG_ID_CASE(String);
        TAG_ID_CASE(List);
        TAG_ID_CASE(Compound);
        TAG_ID_CASE(IntArray);
        TAG_ID_CASE(LongArray);
        default:
            abort(); // unreachable
    }
#undef TAG_ID_CASE
}

string to_string(const mc::nbt::tags::tag& tag)
{
    using namespace mc::nbt;
    std::stringstream result;
    tag.write(result << Contexts::mojangson);
    return result.str();
}

} // namespace std
