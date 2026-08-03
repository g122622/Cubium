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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND OF EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/command/CommandContext.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::command {

/**
 * @brief 时间参数类型
 *
 * 支持 Minecraft 原版的时间字符串格式，解析数字加可选后缀并转换为 tick 数。
 *
 * 支持的后缀：
 * - "t" 或无后缀: tick（乘数 1）
 * - "s": 秒（乘数 20，1 秒 = 20 tick）
 * - "d": 天（乘数 24000，1 天 = 24000 tick）
 *
 * 示例：
 * - "100"   -> 100 tick
 * - "100t"  -> 100 tick
 * - "5s"    -> 100 tick（5 * 20）
 * - "1d"    -> 24000 tick（1 * 24000）
 * - "1.5d"  -> 36000 tick（Math.round(1.5 * 24000)）
 *
 * 对齐 MC Java 版 net.minecraft.commands.arguments.TimeArgument
 */
class TimeArgumentType : public ArgumentType<i32> {
public:
    /**
     * @brief 构造时间参数类型
     * @param minimum 允许的最小 tick 数（默认 0）
     */
    explicit TimeArgumentType(i32 minimum = 0)
        : m_minimum(minimum)
    {}

    [[nodiscard]] i32 parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();

        // 读取浮点数部分（MC 原版使用 readFloat，支持如 "1.5d" 的输入）
        f64 value = 0.0;
        try {
            value = reader.readDouble();
        }
        catch (const CommandException&) {
            reader.setCursor(start);
            throw;
        }

        // 读取后缀单位字符串
        std::string unit = reader.readUnquotedString();

        // 查找单位对应的 tick 乘数
        auto it = s_units.find(unit);
        if (it == s_units.end()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::Unknown,
                "Invalid time unit: '" + unit + "'. Expected 's', 'd', 't', or no unit",
                start);
        }

        i32 multiplier = it->second;

        // 计算总 tick 数（四舍五入，与 MC Java 的 Math.round 一致）
        f64 rawTicks = value * static_cast<f64>(multiplier);
        i32 ticks = static_cast<i32>(std::round(rawTicks));

        // 检查最小值
        if (ticks < m_minimum) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::IntegerTooLow,
                "Time value must be at least " + std::to_string(m_minimum) + " ticks (got " + std::to_string(ticks) +
                    ")",
                start);
        }

        return ticks;
    }

    [[nodiscard]] std::string getTypeName() const override { return "time"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override { return {"0d", "0s", "0t", "0"}; }

    [[nodiscard]] nlohmann::json serializeMetadata() const override { return nlohmann::json{{"min", m_minimum}}; }

    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建默认时间参数类型（最小值 0）
     *
     * 适用于 /schedule、/time set、/time add、/title times、/worldborder 等命令
     */
    static std::shared_ptr<TimeArgumentType> time() { return std::make_shared<TimeArgumentType>(0); }

    /**
     * @brief 创建指定最小值的时间参数类型
     * @param minimum 允许的最小 tick 数
     *
     * 适用于 /weather（minimum=1）、/tick step、/tick sprint 等命令
     */
    static std::shared_ptr<TimeArgumentType> time(i32 minimum) { return std::make_shared<TimeArgumentType>(minimum); }

    // ========== 静态获取方法 ==========

    /**
     * @brief 从命令上下文中获取时间参数值
     * @tparam S 命令源类型
     * @param context 命令上下文
     * @param name 参数名
     * @return tick 数
     */
    template <typename S>
    static i32 getTime(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<i32>(name);
    }

private:
    i32 m_minimum;

    /// 单位字符串到 tick 乘数的映射表（与 MC Java TimeArgument.UNITS 一致）
    static const std::unordered_map<std::string, i32> s_units;
};

// 静态成员定义
inline const std::unordered_map<std::string, i32> TimeArgumentType::s_units = {
    {"d", 24000}, // 天：1 天 = 24000 tick
    {"s", 20},    // 秒：1 秒 = 20 tick
    {"t", 1},     // tick：1 tick = 1 tick
    {"", 1},      // 无后缀：默认为 tick
};

} // namespace mc::command
