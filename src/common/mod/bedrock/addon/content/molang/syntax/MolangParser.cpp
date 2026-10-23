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

#include "common/mod/bedrock/addon/content/molang/syntax/MolangProgram.hpp"
#include "common/util/math/MathConstants.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <map>
#include <utility>

namespace mc::mod::bedrock::addon::detail {
namespace {
constexpr std::size_t _MAX_NODES = 4096;
constexpr std::size_t _MAX_DEPTH = 128;

enum class _TokenKind { Number, String, Name, Symbol, End };

struct _Token {
    _TokenKind kind;
    std::string text;
    f32 number;
};

bool _digit(char character)
{
    return character >= '0' && character <= '9';
}

bool _letter(char character)
{
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
}

Result<std::vector<_Token>> _tokenize(std::string_view source)
{
    std::vector<_Token> tokens;
    std::size_t offset = 0;
    while (offset < source.size()) {
        const char character = source[offset];
        if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
            ++offset;
            continue;
        }
        if (tokens.size() >= _MAX_NODES) {
            return Error(ErrorCode::InvalidData, "Molang expression is too large");
        }
        if (_digit(character) || (character == '.' && offset + 1 < source.size() && _digit(source[offset + 1]))) {
            f32 value = 0;
            const char* first = source.data() + offset;
            const auto parsed =
                std::from_chars(first, source.data() + source.size(), value, std::chars_format::general);
            if (parsed.ec != std::errc{} || parsed.ptr == first || !std::isfinite(value)) {
                return Error(ErrorCode::InvalidData, "Invalid finite Molang number");
            }
            tokens.push_back({_TokenKind::Number, "", value});
            offset = static_cast<std::size_t>(parsed.ptr - source.data());
        } else if (_letter(character)) {
            const auto first = offset++;
            while (offset < source.size() &&
                (_letter(source[offset]) || _digit(source[offset]) || source[offset] == '.')) {
                ++offset;
            }
            std::string name(source.substr(first, offset - first));
            std::transform(name.begin(), name.end(), name.begin(), [](char value) {
                return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
            });
            if (name.starts_with("q.")) {
                name = "query." + name.substr(2);
            }
            tokens.push_back({_TokenKind::Name, std::move(name), 0});
        } else if (character == '\'') {
            ++offset;
            std::string value;
            bool closed = false;
            while (offset < source.size()) {
                char next = source[offset++];
                if (next == '\'') {
                    closed = true;
                    break;
                }
                if (next == '\\') {
                    if (offset == source.size() || (source[offset] != '\'' && source[offset] != '\\')) {
                        return Error(ErrorCode::InvalidData, "Unsupported Molang string escape");
                    }
                    next = source[offset++];
                }
                value.push_back(next);
            }
            if (!closed) {
                return Error(ErrorCode::InvalidData, "Unterminated Molang string");
            }
            tokens.push_back({_TokenKind::String, std::move(value), 0});
        } else {
            const auto pair = source.substr(offset, 2);
            if (pair == "&&" || pair == "||" || pair == "==" || pair == "!=" || pair == "<=" || pair == ">=") {
                tokens.push_back({_TokenKind::Symbol, std::string(pair), 0});
                offset += 2;
            } else if (std::string_view("+-*/%!<>()?:,;").find(character) != std::string_view::npos) {
                tokens.push_back({_TokenKind::Symbol, std::string(1, character), 0});
                ++offset;
            } else {
                // TODO: 接入变量赋值、作用域、数组、空合并和循环语法。
                return Error(ErrorCode::Unsupported, "Unsupported Molang syntax at offset " + std::to_string(offset));
            }
        }
    }
    tokens.push_back({_TokenKind::End, "", 0});
    return tokens;
}

int _precedence(std::string_view operation)
{
    if (operation == "||") {
        return 1;
    }
    if (operation == "&&") {
        return 2;
    }
    if (operation == "==" || operation == "!=") {
        return 3;
    }
    if (operation == "<" || operation == ">" || operation == "<=" || operation == ">=") {
        return 4;
    }
    if (operation == "+" || operation == "-") {
        return 5;
    }
    if (operation == "*" || operation == "/" || operation == "%") {
        return 6;
    }
    return -1;
}

std::optional<std::size_t> _arity(const std::string& name)
{
    static const std::map<std::string, std::size_t> functions = {{"query.property", 1},
        {"query.has_property", 1},
        {"math.abs", 1},
        {"math.acos", 1},
        {"math.asin", 1},
        {"math.atan", 1},
        {"math.atan2", 2},
        {"math.ceil", 1},
        {"math.clamp", 3},
        {"math.copy_sign", 2},
        {"math.cos", 1},
        {"math.exp", 1},
        {"math.floor", 1},
        {"math.hermite_blend", 1},
        {"math.inverse_lerp", 3},
        {"math.lerp", 3},
        {"math.lerprotate", 3},
        {"math.ln", 1},
        {"math.max", 2},
        {"math.min", 2},
        {"math.min_angle", 1},
        {"math.mod", 2},
        {"math.pow", 2},
        {"math.random", 2},
        {"math.random_integer", 2},
        {"math.round", 1},
        {"math.sign", 1},
        {"math.sin", 1},
        {"math.sqrt", 1},
        {"math.trunc", 1}};
    const auto found = functions.find(name);
    return found != functions.end() ? std::optional<std::size_t>(found->second) : std::nullopt;
}

class _Parser {
public:
    explicit _Parser(std::vector<_Token> tokens)
        : m_tokens(std::move(tokens))
    {}

    Result<MolangProgram> parse()
    {
        if (_peek().kind == _TokenKind::Name && _peek().text == "return") {
            ++m_offset;
        }
        auto root = _expression(0, 0);
        if (root.failed()) {
            return root.error();
        }
        _consume(";");
        if (_peek().kind != _TokenKind::End) {
            return Error(ErrorCode::InvalidData, "Unexpected token after Molang expression: " + _peek().text);
        }
        return MolangProgram{std::move(m_nodes), root.value()};
    }

private:
    const _Token& _peek() const
    {
        return m_tokens[m_offset];
    }

    bool _consume(std::string_view text)
    {
        if (_peek().text == text && _peek().kind != _TokenKind::End) {
            ++m_offset;
            return true;
        }
        return false;
    }

    Result<std::size_t> _node(MolangNode node)
    {
        if (m_nodes.size() >= _MAX_NODES) {
            return Error(ErrorCode::InvalidData, "Molang expression node limit exceeded");
        }
        std::size_t depth = 1;
        for (const auto input : node.inputs) {
            depth = std::max(depth, m_depths[input] + 1);
        }
        if (depth >= _MAX_DEPTH) {
            return Error(ErrorCode::InvalidData, "Molang syntax tree depth limit exceeded");
        }
        m_depths.push_back(depth);
        m_nodes.push_back(std::move(node));
        return m_nodes.size() - 1;
    }

    Result<std::size_t> _primary(std::size_t depth)
    {
        if (depth >= _MAX_DEPTH) {
            return Error(ErrorCode::InvalidData, "Molang nesting limit exceeded");
        }
        const auto token = _peek();
        if (token.kind == _TokenKind::Number || token.kind == _TokenKind::String) {
            ++m_offset;
            return _node({token.kind == _TokenKind::Number ? MolangNodeKind::Number : MolangNodeKind::String,
                token.number,
                token.text,
                {}});
        }
        if (_consume("(")) {
            auto value = _expression(0, depth + 1);
            if (value.failed()) {
                return value.error();
            }
            if (!_consume(")")) {
                return Error(ErrorCode::InvalidData, "Molang expression requires closing parenthesis");
            }
            return value;
        }
        if (token.text == "+" || token.text == "-" || token.text == "!") {
            ++m_offset;
            auto input = _primary(depth + 1);
            if (input.failed()) {
                return input.error();
            }
            return _node({MolangNodeKind::Unary, 0, token.text, {input.value()}});
        }
        if (token.kind != _TokenKind::Name) {
            return Error(ErrorCode::InvalidData, "Molang expression requires a value");
        }
        ++m_offset;
        if (token.text == "true" || token.text == "false" || token.text == "math.pi") {
            return _node({MolangNodeKind::Number,
                token.text == "math.pi"    ? mc::math::PI
                    : token.text == "true" ? 1.0f
                                           : 0.0f,
                "",
                {}});
        }
        const auto arity = _arity(token.text);
        if (!arity) {
            // TODO: 继续接入其他查询、变量和数学函数。
            return Error(ErrorCode::Unsupported, "Molang function or variable is not implemented: " + token.text);
        }
        if (!_consume("(")) {
            return Error(ErrorCode::InvalidData, "Molang function requires parentheses");
        }
        std::vector<std::size_t> inputs;
        if (!_consume(")")) {
            for (;;) {
                auto input = _expression(0, depth + 1);
                if (input.failed()) {
                    return input.error();
                }
                inputs.push_back(input.value());
                if (_consume(")")) {
                    break;
                }
                if (!_consume(",")) {
                    return Error(ErrorCode::InvalidData, "Molang function requires comma or closing parenthesis");
                }
            }
        }
        if (inputs.size() != *arity) {
            return Error(ErrorCode::InvalidData, "Incorrect Molang function argument count: " + token.text);
        }
        return _node({MolangNodeKind::Call, 0, token.text, std::move(inputs)});
    }

    Result<std::size_t> _expression(int minimum, std::size_t depth)
    {
        auto left = _primary(depth);
        if (left.failed()) {
            return left.error();
        }
        for (;;) {
            const auto operation = _peek().text;
            const int precedence = _precedence(operation);
            if (precedence < minimum) {
                break;
            }
            ++m_offset;
            auto right = _expression(precedence + 1, depth + 1);
            if (right.failed()) {
                return right.error();
            }
            left = _node({MolangNodeKind::Binary, 0, operation, {left.value(), right.value()}});
            if (left.failed()) {
                return left.error();
            }
        }
        if (minimum == 0 && _consume("?")) {
            auto yes = _expression(0, depth + 1);
            if (yes.failed()) {
                return yes.error();
            }
            auto no = _node({MolangNodeKind::Number, 0, "", {}});
            if (no.failed()) {
                return no.error();
            }
            if (_consume(":")) {
                no = _expression(0, depth + 1);
                if (no.failed()) {
                    return no.error();
                }
            }
            left = _node({MolangNodeKind::Conditional, 0, "", {left.value(), yes.value(), no.value()}});
        }
        return left;
    }

    std::vector<_Token> m_tokens;
    std::vector<MolangNode> m_nodes;
    std::vector<std::size_t> m_depths;
    std::size_t m_offset = 0;
};
} // namespace

Result<MolangProgram> parseMolang(std::string_view source)
{
    auto tokens = _tokenize(source);
    if (tokens.failed()) {
        return tokens.error();
    }
    return _Parser(std::move(tokens.value())).parse();
}
} // namespace mc::mod::bedrock::addon::detail
