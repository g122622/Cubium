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
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/IRandom.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon::detail {
namespace {
Result<f32> _number(const MolangValue& value)
{
    if (const auto* number = std::get_if<f32>(&value)) {
        return *number;
    }
    return Error(ErrorCode::InvalidArgument, "Molang numeric operation received a string");
}

Result<MolangValue> _finite(f32 value)
{
    if (!std::isfinite(value)) {
        return Error(ErrorCode::InvalidArgument, "Molang operation produced a non-finite value");
    }
    return MolangValue(value);
}

Result<MolangValue> _math(
    const std::string& function, const std::vector<MolangValue>& arguments, MolangEvaluationContext& context)
{
    MC_ASSERT_RELEASE(arguments.size() <= 3);
    std::array<f32, 3> values{0, 0, 0};
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        auto value = _number(arguments[index]);
        if (value.failed()) {
            return value.error();
        }
        values[index] = value.value();
    }
    const auto [a, b, c] = values;
    if (function == "math.abs") {
        return _finite(std::abs(a));
    }
    if (function == "math.acos") {
        return _finite(std::acos(a) * mc::math::RAD_TO_DEG);
    }
    if (function == "math.asin") {
        return _finite(std::asin(a) * mc::math::RAD_TO_DEG);
    }
    if (function == "math.atan") {
        return _finite(std::atan(a) * mc::math::RAD_TO_DEG);
    }
    if (function == "math.atan2") {
        return _finite(std::atan2(a, b) * mc::math::RAD_TO_DEG);
    }
    if (function == "math.ceil") {
        return _finite(std::ceil(a));
    }
    if (function == "math.copy_sign") {
        return _finite(std::copysign(a, b));
    }
    if (function == "math.cos") {
        return _finite(std::cos(a * mc::math::DEG_TO_RAD));
    }
    if (function == "math.exp") {
        return _finite(std::exp(a));
    }
    if (function == "math.floor") {
        return _finite(std::floor(a));
    }
    if (function == "math.hermite_blend") {
        return _finite(a * a * (3 - 2 * a));
    }
    if (function == "math.inverse_lerp") {
        return _finite(a == b ? 0.0f : (c - a) / (b - a));
    }
    if (function == "math.lerp") {
        return _finite(a + (b - a) * c);
    }
    if (function == "math.lerprotate") {
        return _finite(a + mc::math::wrapDegrees(b - a) * c);
    }
    if (function == "math.ln") {
        return _finite(std::log(a));
    }
    if (function == "math.max") {
        return _finite(std::max(a, b));
    }
    if (function == "math.min") {
        return _finite(std::min(a, b));
    }
    if (function == "math.min_angle") {
        return _finite(mc::math::wrapDegrees(a));
    }
    if (function == "math.mod") {
        return _finite(b == 0 ? 0.0f : std::fmod(a, b));
    }
    if (function == "math.pow") {
        return _finite(std::pow(a, b));
    }
    if (function == "math.round") {
        return _finite(std::round(a));
    }
    if (function == "math.sign") {
        return _finite(a > 0 ? 1.0f : a < 0 ? -1.0f : 0.0f);
    }
    if (function == "math.sin") {
        return _finite(std::sin(a * mc::math::DEG_TO_RAD));
    }
    if (function == "math.sqrt") {
        return _finite(std::sqrt(a));
    }
    if (function == "math.trunc") {
        return _finite(std::trunc(a));
    }
    if (function == "math.clamp") {
        if (b > c) {
            return Error(ErrorCode::InvalidArgument, "Molang clamp requires ordered bounds");
        }
        return _finite(std::clamp(a, b, c));
    }
    if (function == "math.random") {
        if (a > b) {
            return Error(ErrorCode::InvalidArgument, "Molang random requires ordered bounds");
        }
        // 浮点随机区间含两端点，随机源仍由实体或宿主明确提供。
        const double fraction = static_cast<double>(context.random.nextInt(16777217)) / 16777216.0;
        return _finite(static_cast<f32>(static_cast<double>(a) + (static_cast<double>(b) - a) * fraction));
    }
    if (function == "math.random_integer") {
        const double low = std::ceil(static_cast<double>(a));
        const double high = std::floor(static_cast<double>(b));
        if (low > high || low < std::numeric_limits<i32>::min() || high > std::numeric_limits<i32>::max()) {
            return Error(ErrorCode::InvalidArgument, "Molang random_integer requires a valid integer interval");
        }
        const i64 first = static_cast<i64>(low);
        const u64 span = static_cast<u64>(static_cast<i64>(high) - first + 1);
        // 有符号全区间无法交给 i32 的差值运算，使用现有随机源做无偏区间选择。
        const u64 threshold = (u64(0) - span) % span;
        u64 sample = context.random.nextU64();
        while (sample < threshold) {
            sample = context.random.nextU64();
        }
        return _finite(static_cast<f32>(first + static_cast<i64>(sample % span)));
    }
    return Error(ErrorCode::Unsupported, "Molang math function is not implemented: " + function);
}
} // namespace

Result<MolangValue> evaluateMolang(const MolangProgram& program, MolangEvaluationContext& context)
{
    std::size_t steps = 0;
    std::function<Result<MolangValue>(std::size_t, std::size_t)> evaluate;
    evaluate = [&](std::size_t index, std::size_t depth) -> Result<MolangValue> {
        if (depth >= 128 || ++steps > 8192) {
            return Error(ErrorCode::InvalidState, "Molang execution budget exceeded");
        }
        MC_ASSERT_RELEASE(index < program.nodes.size());
        const auto& node = program.nodes[index];
        if (node.kind == MolangNodeKind::Number) {
            return MolangValue(node.number);
        }
        if (node.kind == MolangNodeKind::String) {
            return MolangValue(node.text);
        }
        if (node.kind == MolangNodeKind::Call) {
            std::vector<MolangValue> arguments;
            arguments.reserve(node.inputs.size());
            for (const auto input : node.inputs) {
                auto value = evaluate(input, depth + 1);
                if (value.failed()) {
                    return value.error();
                }
                arguments.push_back(std::move(value.value()));
            }
            if (node.text == "query.property" || node.text == "query.has_property") {
                const auto* name = std::get_if<std::string>(&arguments[0]);
                if (name == nullptr) {
                    return Error(ErrorCode::InvalidArgument, "Molang property query requires a string name");
                }
                if (!context.property) {
                    return Error(ErrorCode::InvalidState, "Molang property query has no host accessor");
                }
                const auto value = context.property(*name);
                if (node.text == "query.has_property") {
                    return MolangValue(value ? 1.0f : 0.0f);
                }
                if (!value) {
                    spdlog::warn("Molang query.property references an undeclared property: {}", *name);
                    return MolangValue(0.0f);
                }
                return *value;
            }
            return _math(node.text, arguments, context);
        }
        auto left = evaluate(node.inputs[0], depth + 1);
        if (left.failed()) {
            return left.error();
        }
        if (node.kind == MolangNodeKind::Conditional) {
            auto condition = _number(left.value());
            if (condition.failed()) {
                return condition.error();
            }
            return evaluate(node.inputs[condition.value() != 0 ? 1 : 2], depth + 1);
        }
        if (node.kind == MolangNodeKind::Unary) {
            auto value = _number(left.value());
            if (value.failed()) {
                return value.error();
            }
            return _finite(node.text == "-" ? -value.value()
                    : node.text == "!"      ? (value.value() == 0 ? 1.0f : 0.0f)
                                            : value.value());
        }
        if (node.text == "&&" || node.text == "||") {
            auto first = _number(left.value());
            if (first.failed()) {
                return first.error();
            }
            if ((node.text == "&&" && first.value() == 0) || (node.text == "||" && first.value() != 0)) {
                return MolangValue(node.text == "||" ? 1.0f : 0.0f);
            }
        }
        auto right = evaluate(node.inputs[1], depth + 1);
        if (right.failed()) {
            return right.error();
        }
        if (node.text == "==" || node.text == "!=") {
            const bool equal = left.value() == right.value();
            return MolangValue((node.text == "==" ? equal : !equal) ? 1.0f : 0.0f);
        }
        auto first = _number(left.value());
        if (first.failed()) {
            return first.error();
        }
        auto second = _number(right.value());
        if (second.failed()) {
            return second.error();
        }
        const f32 a = first.value();
        const f32 b = second.value();
        if (node.text == "+") {
            return _finite(a + b);
        }
        if (node.text == "-") {
            return _finite(a - b);
        }
        if (node.text == "*") {
            return _finite(a * b);
        }
        if (node.text == "/") {
            return _finite(b == 0 ? 0.0f : a / b);
        }
        if (node.text == "%") {
            return _finite(b == 0 ? 0.0f : std::fmod(a, b));
        }
        if (node.text == "<") {
            return MolangValue(a < b ? 1.0f : 0.0f);
        }
        if (node.text == ">") {
            return MolangValue(a > b ? 1.0f : 0.0f);
        }
        if (node.text == "<=") {
            return MolangValue(a <= b ? 1.0f : 0.0f);
        }
        if (node.text == ">=") {
            return MolangValue(a >= b ? 1.0f : 0.0f);
        }
        if (node.text == "&&" || node.text == "||") {
            return MolangValue(b != 0 ? 1.0f : 0.0f);
        }
        return Error(ErrorCode::InvalidState, "Unknown compiled Molang operator");
    };
    return evaluate(program.root, 0);
}
} // namespace mc::mod::bedrock::addon::detail
