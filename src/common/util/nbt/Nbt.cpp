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
#include <cstdint>
#include <cstdlib>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace nbt {

void skip_space(std::istream& input)
{
    for (;;) {
        int next = cheof(input);
        if (!std::isspace(next)) {
            input.putback(next);
            return;
        }
    }
}

int context_id()
{
    static const int i = std::ios_base::xalloc();
    return i;
}

Context*& context_storage(std::ios_base& ios)
{
    int i = context_id();
    return reinterpret_cast<Context*&>(ios.pword(i));
}

const Context& Context::get(std::ios_base& ios)
{
    auto ctxt = context_storage(ios);
    if (ctxt == nullptr)
        return Contexts::java;
    else
        return *ctxt;
}

void ios_callback(std::ios_base::event event, std::ios_base& ios, int)
{
    if (event == std::ios_base::event::erase_event) delete context_storage(ios);
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

std::int32_t load_varint(std::istream& input)
{
    return load_varnum<std::int32_t>(input);
}

void dump_varint(std::ostream& output, std::int32_t value)
{
    dump_varnum(output, value);
}

std::int64_t load_varlong(std::istream& input)
{
    return load_varnum<std::int64_t>(input);
}

void dump_varlong(std::ostream& output, std::int64_t value)
{
    dump_varnum(output, value);
}

template <typename number_t, char suffix>
number_t load_text_simple(std::istream& input)
{
    number_t value;
    input >> value;
    int next = cheof(input);
    if (next != suffix) input.putback(next);
    return value;
}

template <>
std::int8_t load_text<std::int8_t>(std::istream& input)
{
    int value;
    input >> value;
    int next = cheof(input);
    if (next != 'b') input.putback(next);
    return static_cast<std::int8_t>(value);
}

template <>
std::int16_t load_text<std::int16_t>(std::istream& input)
{
    return load_text_simple<std::int16_t, 's'>(input);
}

template <>
std::int32_t load_text<std::int32_t>(std::istream& input)
{
    std::int32_t value;
    input >> value;
    return value;
}

template <>
std::int64_t load_text<std::int64_t>(std::istream& input)
{
    return load_text_simple<std::int64_t, 'l'>(input);
}

template <>
float load_text<float>(std::istream& input)
{
    return load_text_simple<float, 'f'>(input);
}

template <>
double load_text<double>(std::istream& input)
{
    return load_text_simple<double, 'd'>(input);
}

template <>
std::int32_t load<std::int32_t>(std::istream& input, const Context& ctxt)
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
std::int64_t load<std::int64_t>(std::istream& input, const Context& ctxt)
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
    output << int(number) << 'b';
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
void dump_text<float>(std::ostream& output, float number)
{
    output << number << 'f';
}

template <>
void dump_text<double>(std::ostream& output, double number)
{
    output << number;
}

std::size_t load_size(std::istream& input, const Context& ctxt)
{
    if (ctxt.format == Context::Format::Bin)
        return load_flat<std::uint32_t>(input, ctxt.order);
    else
        return static_cast<std::size_t>(load_varint(input));
}

void dump_size(std::ostream& output, const Context& ctxt, std::size_t size)
{
    auto sz = static_cast<std::uint32_t>(size);
    if (ctxt.format == Context::Format::Bin)
        dump_flat(output, sz, ctxt.order);
    else
        dump_varint(output, sz);
}

std::ostream& operator<<(std::ostream& output, TagId tid)
{
    return output << std::to_string(tid);
}

inline bool is_valid_char(char c)
{
    return std::isalnum(c) || c == '-' || c == '_' || c == '+' || c == '.';
}

TagId deduce_tag(std::istream& input)
{
    skip_space(input);
    char a = cheof(input);
    if (a == '}' || a == ']') {
        input.putback(a);
        return TagId::End;
    }
    if (a == '[') {
        char b = cheof(input);
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
            char c = input.peek();
            if (c != ';') id = TagId::List;
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
            char b = cheof(input);
            buffer.push_back(b);
            if (std::isdigit(b)) {
                continue;
            } else if (b == 'b') {
                deduced = TagId::Byte;
            } else if (b == 's') {
                deduced = TagId::Short;
            } else if (b == 'l') {
                deduced = TagId::Long;
            } else if (b == 'f') {
                deduced = TagId::Float;
            } else if (b == 'd') {
                deduced = TagId::Double;
            } else if (b == 'e') {
                char c = cheof(input);
                buffer.push_back(c);
                if (std::isdigit(c) || c == '-' || c == '+') {
                    for (;;) {
                        char d = cheof(input);
                        buffer.push_back(d);
                        if (std::isdigit(d)) {
                            continue;
                        } else if (d == 'f') {
                            deduced = TagId::Float;
                        } else if (d == 'd') {
                            deduced = TagId::Double;
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
                    char c = cheof(input);
                    buffer.push_back(c);
                    if (std::isdigit(c)) {
                        continue;
                    } else if (c == 'e' || c == 'E') {
                        char d = cheof(input);
                        buffer.push_back(d);
                        if (std::isdigit(d) || d == '-' || d == '+') continue;
                    } else if (c == 'f') {
                        deduced = TagId::Float;
                    } else if (c == 'd') {
                        deduced = TagId::Double;
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
        for (auto iter = buffer.crbegin(), end = buffer.crend(); iter != end; ++iter)
            input.putback(*iter);
        return deduced;
    }
    if (a == '"' || a == '\'' || is_valid_char(a)) {
        input.putback(a);
        return TagId::String;
    }
    input.putback(a);
    return TagId::End;
}

namespace tags {

std::string read_string_text(std::istream& input)
{
    skip_space(input);
    char first = cheof(input);
    if (first == '"' || first == '\'') {
        char quote = first;
        std::string result;
        for (;;) {
            int c = cheof(input);
            switch (c) {
                case '\'':
                    if (quote == '\'') {
                        return result;
                    }
                    result.push_back(c);
                    break;
                case '"':
                    if (quote == '"') {
                        return result;
                    }
                    result.push_back(c);
                    break;
                case '\\': {
                    int s = cheof(input);
                    switch (s) {
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
                            throw std::runtime_error("Unicode escape sequences not yet supported");
                        default:
                            result.push_back('\\');
                            result.push_back(s);
                            break;
                    }
                    break;
                }
                default:
                    result.push_back(c);
                    break;
            }
        }
    } else {
        std::string result(&first, 1);
        for (;;) {
            int c = cheof(input);
            if (is_valid_char(c)) {
                result.push_back(c);
            } else {
                input.putback(c);
                return result;
            }
        }
    }
}

std::string read_string_bin(std::istream& input, const Context& ctxt)
{
    std::uint32_t size = (ctxt.format == Context::Format::Zigzag || ctxt.format == Context::Format::Zint)
        ? load_varint(input)
        : load_flat<std::uint16_t>(input, ctxt.order);
    std::string result;
    result.resize(size);
    input.read(result.data(), size);
    return result;
}

std::string read_string(std::istream& input, const Context& ctxt)
{
    if (ctxt.format == Context::Format::Mojangson)
        return read_string_text(input);
    else
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
std::unique_ptr<tag> read_bridge(TagId id, std::istream& input)
{
    if (id == tid)
        return read<tid>(input);
    else
        return read_bridge<static_cast<TagId>(static_cast<std::int8_t>(tid) - 1)>(id, input);
}

template <>
std::unique_ptr<tag> read_bridge<TagId::End>(TagId, std::istream&)
{
    throw std::runtime_error("end tag not for reading");
}

std::unique_ptr<tag> read(TagId tid, std::istream& input)
{
    return read_bridge<TagId::LongArray>(tid, input);
}

template <TagId tid>
std::unique_ptr<list_tag> read_list_content_bridge(TagId id, std::istream& input)
{
    if (id == tid)
        return list_by<tid>::read_content(input);
    else
        return read_list_content_bridge<static_cast<TagId>(static_cast<std::int8_t>(tid) - 1)>(id, input);
}

template <>
std::unique_ptr<list_tag> read_list_content_bridge<TagId::End>(TagId id, std::istream& input)
{
    MC_ASSERT_RELEASE_MSG(id == TagId::End, "Expected End tag id in list content bridge");
    return list_by<TagId::End>::read_content(input);
}

std::unique_ptr<list_tag> read_list_content(TagId tid, std::istream& input)
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

std::unique_ptr<string_tag> string_tag::read(std::istream& input)
{
    const Context& ctxt = Context::get(input);
    return std::make_unique<string_tag>(read_string(input, ctxt));
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

std::unique_ptr<list_tag> list_tag::read(std::istream& input)
{
    const Context& ctxt = Context::get(input);
    if (ctxt.format == Context::Format::Mojangson) {
        skip_space(input);
        char a = cheof(input);
        if (a != '[') throw std::runtime_error("failed to open list tag");
        TagId type = deduce_tag(input);
        return read_list_content(type, input);
    } else {
        TagId type = static_cast<TagId>(cheof(input));
        return read_list_content(type, input);
    }
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
    for (const auto& each : other.value)
        value.emplace_back(each->copy());
}

tag_list_tag::tag_list_tag(const value_type& list, TagId tid)
    : eid(tid)
{
    value.reserve(list.size());
    for (const auto& each : list)
        value.emplace_back(each->copy());
}

tag_list_tag::tag_list_tag(value_type&& list, TagId tid)
    : eid(tid)
    , value(std::move(list))
{}

void tag_list_tag::write(std::ostream& output) const
{
    TagId aid = element_id();
    dump_list(output, aid, value, [&output, aid](const std::unique_ptr<tag>& tag, const Context&) {
        if (aid != tag->id()) throw std::runtime_error("wrong tag id");
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
    throw std::out_of_range("end list tag always empty");
}

std::unique_ptr<end_list_tag> end_list_tag::read_content(std::istream& input)
{
    const Context& ctxt = Context::get(input);
    if (ctxt.format != Context::Format::Mojangson) {
        // NBT 二进制 List 格式恒为 etid(1, 已由 list_tag::read 消费) + size(4) + elements。
        // TAG_End 列表元素类型为 End，规范要求 size==0，但 size 字段本身仍占 4 字节。
        // 此前实现仅读 1 字节并断言为 0，遗留 3 字节使外层 Compound 把它误读为 End 提前结束，
        // 丢失后续键——基岩 .mcstructure 的 structure.entities 为 List[End] n=0 时致 palette 键丢失，
        // 结构模板退化为空，GameTest 按钮识别/方块放置全失效。
        const std::size_t size = load_size(input, ctxt);
        if (size != 0) throw std::runtime_error("end list tag should be empty");
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

std::unique_ptr<string_list_tag> string_list_tag::read_content(std::istream& input)
{
    return load_list<string_list_tag>(
        input, [&input](const Context& ctxt) -> std::string { return read_string(input, ctxt); });
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
    for (auto& each : value)
        result.value.push_back(std::make_unique<tag_type>(std::move(each)));
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
    for (const auto& element : other.value)
        value.emplace_back(cast<list_tag>(element->copy()));
}

list_list_tag::list_list_tag(const value_type& list)
{
    value.reserve(list.size());
    for (const auto& element : list)
        value.emplace_back(cast<list_tag>(element->copy()));
}

list_list_tag::list_list_tag(value_type&& list)
    : value(std::move(list))
{}

std::unique_ptr<list_list_tag> list_list_tag::read_content(std::istream& input)
{
    return load_list<list_list_tag>(input, [&input](const Context&) -> element_type { return list_tag::read(input); });
}

void list_list_tag::write(std::ostream& output) const
{
    dump_list(output, eid, value, [&output](const element_type& list, const Context&) { list->write(output); });
}

std::unique_ptr<tag> list_list_tag::copy() const
{
    return std::make_unique<list_list_tag>(*this);
}

tag_list_tag list_list_tag::as_tags()
{
    tag_list_tag result(eid);
    result.value.reserve(value.size());
    for (auto& each : value)
        result.value.push_back(std::move(each));
    value.clear();
    value.shrink_to_fit();
    return result;
}

compound_tag::compound_tag(const compound_tag& other)
    : is_root(other.is_root)
{
    for (const auto& pair : other.value)
        value.emplace(pair.first, pair.second->copy());
}

compound_tag::compound_tag(bool root)
    : is_root(root)
{}

compound_tag::compound_tag(const value_type& map, bool root)
    : is_root(root)
{
    for (const auto& pair : map)
        value.emplace(pair.first, pair.second->copy());
}

compound_tag::compound_tag(value_type&& map, bool root)
    : is_root(root)
    , value(std::move(map))
{}

std::unique_ptr<compound_tag> compound_tag::read(std::istream& input)
{
    auto ptr = std::make_unique<compound_tag>();
    input >> *ptr;
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
    if (!compound.is_root) tags::end.write(output);
}

void compound_tag::write(std::ostream& output) const
{
    const Context& ctxt = Context::get(output);
    if (ctxt.format == Context::Format::Mojangson)
        compound_write_text(output, *this, ctxt);
    else
        compound_write_bin(output, *this, ctxt);
}

std::unique_ptr<tag> compound_tag::copy() const
{
    return std::make_unique<compound_tag>(*this);
}

compound_tag& compound_tag::operator=(const compound_tag& other)
{
    is_root = other.is_root;
    for (const auto& pair : other.value)
        value.emplace(pair.first, pair.second->copy());
    return *this;
}

void compound_tag::make_heavy()
{
    for (auto& pair : value) {
        tags::tag& each = *pair.second;
        if (each.id() == TagId::List) {
            list_tag& list = dynamic_cast<list_tag&>(each);
            if (!list.heavy()) pair.second = std::make_unique<tag_list_tag>(list.as_tags());
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

std::unique_ptr<compound_list_tag> compound_list_tag::read_content(std::istream& input)
{
    return load_list<compound_list_tag>(input, [&input](const Context&) -> compound_tag {
        compound_tag element;
        input >> element;
        return element;
    });
}

void compound_list_tag::write(std::ostream& output) const
{
    dump_list(output, eid, value, [&output](const element_type& element, const Context&) { element.write(output); });
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

void read_compound_text(std::istream& input, tags::compound_tag& compound, const Context& ctxt)
{
    skip_space(input);
    char a = cheof(input);
    if (a != '{') throw std::runtime_error("failed to open compound");
    for (;;) {
        TagId key_type = deduce_tag(input);
        switch (key_type) {
            case TagId::End:
                skip_space(input);
                if (cheof(input) != '}') throw std::runtime_error("failed to close compound tag");
                return;
            case TagId::String:
                break;
            default:
                throw std::runtime_error("invalid key tag (tag_string expected, got " + std::to_string(key_type) + ')');
        }
        std::string key = tags::read_string(input, ctxt);
        skip_space(input);
        char a = cheof(input);
        if (a != ':') throw std::runtime_error(std::string("key-value delimiter expected, got ") + a);
        TagId value_type = deduce_tag(input);
        if (value_type == TagId::End) throw std::runtime_error(std::string("value expected"));
        compound.value.emplace(std::move(key), tags::read(value_type, input));
        skip_space(input);
        a = cheof(input);
        if (a == '}') return;
        if (a != ',' && a != ';') throw std::runtime_error(std::string("next tag or end expected, got ") + a);
    }
}

void read_compound_bin(std::istream& input, tags::compound_tag& compound, const Context& ctxt)
{
    for (;;) {
        auto id_numeric = input.get();
        if (!input) {
            compound.is_root = true;
            break;
        }
        auto id = static_cast<TagId>(id_numeric);
        if (id > TagId::LongArray) throw std::out_of_range("unknown tag id: " + std::to_string(id_numeric));
        if (id == TagId::End) break;
        std::string key = tags::read_string(input, ctxt);
        compound.value.emplace(std::move(key), tags::read(id, input));
    }
}

std::istream& operator>>(std::istream& input, tags::compound_tag& compound)
{
    const Context& ctxt = Context::get(input);
    if (ctxt.format == Context::Format::Mojangson)
        read_compound_text(input, compound, ctxt);
    else
        read_compound_bin(input, compound, ctxt);
    return input;
}

std::ostream& operator<<(std::ostream& output, const tags::tag& tag)
{
    tag.write(output);
    return output;
}

// 数组文本格式特化
template <>
std::vector<std::int8_t> load_array_text<std::int8_t>(std::istream& input)
{
    return load_array_text_impl<std::int8_t>(input);
}

template <>
std::vector<std::int32_t> load_array_text<std::int32_t>(std::istream& input)
{
    return load_array_text_impl<std::int32_t>(input);
}

template <>
std::vector<std::int64_t> load_array_text<std::int64_t>(std::istream& input)
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
