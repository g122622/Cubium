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

#include "common/command/StringReader.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include <cfloat>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::command {

// 前向声明
template <typename S>
class CommandContext;

/**
 * @brief 参数类型基类
 *
 * 所有命令参数类型的基类，定义解析和自动补全接口。
 */
template <typename T>
class ArgumentType {
public:
    virtual ~ArgumentType() = default;

    /**
     * @brief 解析参数值
     * @param reader 字符串读取器
     * @return 解析后的值
     * @throws CommandException 如果解析失败
     */
    [[nodiscard]] virtual T parse(StringReader& reader) = 0;

    /**
     * @brief 获取参数类型名称（用于帮助信息）
     */
    [[nodiscard]] virtual std::string getTypeName() const = 0;

    /**
     * @brief 获取示例值列表
     */
    [[nodiscard]] virtual std::vector<std::string> getExamples() const
    {
        return {};
    }

    /**
     * @brief 序列化参数类型元数据
     * @return 用于命令树同步的类型元数据
     */
    [[nodiscard]] virtual nlohmann::json serializeMetadata() const
    {
        return nlohmann::json::object();
    }
};

// ========== 字符串参数 ==========

/**
 * @brief 字符串参数类型
 *
 * 支持三种模式：
 * - SingleWord: 单个单词（无空格）
 * - QuotablePhrase: 可引号短语
 * - GreedyPhrase: 贪婪短语（读取剩余所有内容）
 */
class StringArgumentType : public ArgumentType<std::string> {
public:
    enum class StringType {
        SingleWord,     // 单个单词
        QuotablePhrase, // 可引号短语
        GreedyPhrase    // 贪婪短语
    };

    explicit StringArgumentType(StringType type = StringType::QuotablePhrase)
        : m_type(type)
    {}

    [[nodiscard]] std::string parse(StringReader& reader) override
    {
        switch (m_type) {
            case StringType::SingleWord:
                return reader.readUnquotedString();
            case StringType::QuotablePhrase:
                return reader.readString();
            case StringType::GreedyPhrase: {
                std::string remaining = reader.getRemaining();
                reader.setCursor(reader.getTotalLength());
                return remaining;
            }
        }
        return "";
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        switch (m_type) {
            case StringType::SingleWord:
                return "word";
            case StringType::QuotablePhrase:
                return "phrase";
            case StringType::GreedyPhrase:
                return "greedy_string";
        }
        return "string";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        switch (m_type) {
            case StringType::SingleWord:
                return {"word", "123"};
            case StringType::QuotablePhrase:
                return {"word", "\"quoted phrase\"", "word"};
            case StringType::GreedyPhrase:
                return {"word", "\"quoted phrase\"", "multiple words"};
        }
        return {};
    }

    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"mode", getTypeName()}};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<StringArgumentType> word()
    {
        return std::make_shared<StringArgumentType>(StringType::SingleWord);
    }

    static std::shared_ptr<StringArgumentType> string()
    {
        return std::make_shared<StringArgumentType>(StringType::QuotablePhrase);
    }

    static std::shared_ptr<StringArgumentType> greedyString()
    {
        return std::make_shared<StringArgumentType>(StringType::GreedyPhrase);
    }

    // ========== 静态获取方法 ==========

    static std::string getString(StringReader& reader)
    {
        return reader.readString();
    }

private:
    StringType m_type;
};

/**
 * @brief 读「数字单词」（brigadier {@code isAllowedNumber}）
 *
 * 可选 '-' 开头，之后是数字与至多一个小数点；遇到 `+`、字母、空白立即停下。
 * 所以 `1a` 只读 "1"、`-1.5` 读成 "-1.5"、`~5` 一个字符都不读，`1.5.5` 读成 "1.5.5"。
 */
static std::string readNumberWord(StringReader& reader)
{
    std::string result;
    if (reader.canRead() && reader.peek() == '-') {
        result += reader.read();
    }
    while (reader.canRead() && StringReader::isAllowedNumber(reader.peek())) {
        result += reader.read();
    }
    return result;
}

// ========== 整数参数 ==========

/**
 * @brief 整数参数类型
 *
 * 支持范围检查
 */
class IntegerArgumentType : public ArgumentType<i32> {
public:
    explicit IntegerArgumentType(i32 min = INT32_MIN, i32 max = INT32_MAX)
        : m_min(min)
        , m_max(max)
    {}

    /**
     * @brief 解析整数（brigadier IntegerArgumentType 语义）
     *
     * 原版先用 {@code readInt()} 读数字（`+` 非法、溢出自然回绕、读不到时游标回退），
     * 再要求「后面不能再有非空白字符」——`1a`、`1.0` 都因此整体失败，且报错位置是
     * 读到的位置，不是起点。因此这里先「读完 → 校验 → 失败则回退」，成功时不回退。
     */
    [[nodiscard]] i32 parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        // 与 brigadier 逐行对应：readInt 读数字（读不到时自己回退到起点并抛错），
        // 之后只要还有非空白字符就整体失败——两者抛出的错游标都在起点。
        // 整数与浮点数共用同一条「合法数字」规则读整段单词（可选 '-' + 数字 + 至多一个小数点）。
        const std::string text = readNumberWord(reader);
        i64 number = 0;
        if (!parseJavaInt(text, number)) {
            reader.setCursor(start);
            throw CommandException(
                CommandErrorType::IntegerExpected, "Expected integer at position " + std::to_string(start), start);
        }

        const i32 end = reader.getCursor();
        if (number < m_min) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::IntegerTooLow, "Integer too low", start);
        }
        if (number > m_max) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::IntegerTooHigh, "Integer too high", start);
        }
        return static_cast<i32>(number);
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "integer";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0", "123", "-123"};
    }

    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"min", m_min}, {"max", m_max}};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<IntegerArgumentType> integer()
    {
        return std::make_shared<IntegerArgumentType>();
    }

    static std::shared_ptr<IntegerArgumentType> integer(i32 min)
    {
        return std::make_shared<IntegerArgumentType>(min, INT32_MAX);
    }

    static std::shared_ptr<IntegerArgumentType> integer(i32 min, i32 max)
    {
        return std::make_shared<IntegerArgumentType>(min, max);
    }

    // ========== 静态获取方法 ==========

    static i32 getInteger(StringReader& reader)
    {
        return reader.readInt();
    }

private:
    /**
     * @brief 按 Java {@code Integer.parseInt} 的规则解析整段文本
     *
     * 可选单个 '-' 或 '+'，其后至少一位 ASCII 数字，且结果必须在 i32 范围内；
     * 文本里只要有一个别的字符就整体失败（原版正是整段交给 parseInt）。
     *
     * @param text 已读到的单词
     * @param out 解析结果（仅在返回 true 时有意义）
     * @return 是否解析成功
     */
    static bool parseJavaInt(const std::string& text, i64& out)
    {
        size_t index = 0;
        bool negative = false;
        if (index < text.size() && (text[index] == '-' || text[index] == '+')) {
            negative = text[index] == '-';
            ++index;
        }
        if (index == text.size()) {
            return false;
        }
        i64 value = 0;
        for (; index < text.size(); ++index) {
            const char c = text[index];
            if (c < '0' || c > '9') {
                return false;
            }
            value = value * 10 + (c - '0');
            if (value > 2147483648LL) {
                return false; // 超出 i32 范围时 parseInt 报错
            }
        }
        value = negative ? -value : value;
        if (value < INT32_MIN || value > INT32_MAX) {
            return false;
        }
        out = value;
        return true;
    }

    i32 m_min;
    i32 m_max;
};

// ========== 浮点数参数 ==========

/**
 * @brief 浮点数参数类型
 */
class FloatArgumentType : public ArgumentType<f32> {
public:
    explicit FloatArgumentType(f32 min = -FLT_MAX, f32 max = FLT_MAX)
        : m_min(min)
        , m_max(max)
    {}

    /**
     * @brief 解析浮点数（brigadier FloatArgumentType 语义）
     *
     * 与 {@link IntegerArgumentType::parse} 同样的两步：先读「数字单词」（可选 '-'、数字、
     * 至多一个小数点），再要求后面不能有非空白字符；成功后用 {@code readDouble()} 取值，
     * 它只读数字部分，后面的 `e3`、`d` 之类留给调用方（时间单位、坐标后缀），
     * 所以 `1e3` 读出 1.0 而游标停在 1。失败时游标一律回退到起点。
     */
    [[nodiscard]] f32 parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        const std::string number = readNumberWord(reader);
        if (number.empty() || number.find('.') != number.rfind('.')) {
            reader.setCursor(start);
            throw CommandException(
                CommandErrorType::FloatExpected, "Expected float at position " + std::to_string(start), start);
        }
        reader.setCursor(start);

        const f64 value = reader.readDouble();
        const i32 end = reader.getCursor();
        if (value < m_min) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::FloatTooLow, "Float too low", start);
        }
        if (value > m_max) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::FloatTooHigh, "Float too high", start);
        }
        return static_cast<f32>(value);
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "float";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0", "1.5", "-2.5"};
    }

    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"min", m_min}, {"max", m_max}};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<FloatArgumentType> floatArg()
    {
        return std::make_shared<FloatArgumentType>();
    }

    static std::shared_ptr<FloatArgumentType> floatArg(f32 min)
    {
        return std::make_shared<FloatArgumentType>(min, FLT_MAX);
    }

    static std::shared_ptr<FloatArgumentType> floatArg(f32 min, f32 max)
    {
        return std::make_shared<FloatArgumentType>(min, max);
    }

    static f32 getFloat(StringReader& reader)
    {
        return static_cast<f32>(reader.readDouble());
    }

private:
    f32 m_min;
    f32 m_max;
};

// ========== 布尔参数 ==========

/**
 * @brief 布尔参数类型
 */
class BoolArgumentType : public ArgumentType<bool> {
public:
    [[nodiscard]] bool parse(StringReader& reader) override
    {
        return reader.readBool();
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "bool";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"true", "false"};
    }

    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json::object();
    }

    static std::shared_ptr<BoolArgumentType> boolArg()
    {
        return std::make_shared<BoolArgumentType>();
    }

    static bool getBool(StringReader& reader)
    {
        return reader.readBool();
    }
};

// ========== 枚举参数模板 ==========

/**
 * @brief 枚举参数类型模板
 *
 * 用于定义枚举类型的命令参数
 *
 * @tparam T 枚举类型
 *
 * 使用示例：
 * @code
 * enum class GameMode { Survival, Creative, Adventure, Spectator };
 *
 * auto gameModeArg = EnumArgument<GameMode>::create()
 *     .add("survival", GameMode::Survival)
 *     .add("creative", GameMode::Creative)
 *     .add("adventure", GameMode::Adventure)
 *     .add("spectator", GameMode::Spectator);
 * @endcode
 */
template <typename T>
class EnumArgumentType : public ArgumentType<T> {
public:
    EnumArgumentType& add(const std::string& name, T value)
    {
        m_names.push_back(name);
        m_values[name] = value;
        return *this;
    }

    [[nodiscard]] T parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();
        std::string name = reader.readUnquotedString();

        auto it = m_values.find(name);
        if (it == m_values.end()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::Unknown, "Unknown value: " + name, start);
        }

        return it->second;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "enum";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return m_names;
    }

    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"values", m_names}};
    }

    // ========== 静态方法 ==========

    template <typename S>
    static T getEnum(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<T>(name);
    }

private:
    std::vector<std::string> m_names;
    std::unordered_map<std::string, T> m_values;
};

} // namespace mc::command
