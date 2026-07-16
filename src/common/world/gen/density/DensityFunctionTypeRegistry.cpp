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

#include "common/world/gen/density/DensityFunctionTypeRegistry.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"

#include <nlohmann/json.hpp>

namespace mc::world::gen::density {

namespace {

using json = nlohmann::json;

/// 剥离 "minecraft:" 命名空间前缀
std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

/// 读取必填 double 字段
Result<f64> readDouble(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_number()) {
        return Error(ErrorCode::InvalidData, "missing number field '" + std::string(field) + "'");
    }
    return j[field].get<f64>();
}

/// 读取必填整数字段
Result<i32> readInt(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "missing integer field '" + std::string(field) + "'");
    }
    return j[field].get<i32>();
}

/// 解析必填子 DF 字段，空则报错
Result<std::unique_ptr<DensityFunction>> requireChild(
    const json& j, std::string_view field, const ResolveContext& ctx, std::string_view type)
{
    if (!j.contains(field)) {
        return Error(ErrorCode::InvalidData,
            "density_function type '" + std::string(type) + "' missing field '" + std::string(field) + "'");
    }
    return ctx.resolveInline(j[field]);
}

/// 解析噪声名字段（noise RL 字符串，或内联 {firstOctave,amplitudes} 对象）。
/// 内联噪声在原版 1.21.11 数据包中未使用，本项目噪声已全数据驱动，只支持字符串 RL。
Result<std::string> readNoiseName(const json& j, std::string_view field, std::string_view type)
{
    if (!j.contains(field)) {
        return Error(ErrorCode::InvalidData,
            "density_function type '" + std::string(type) + "' missing noise field '" + std::string(field) + "'");
    }
    const auto& node = j[field];
    if (node.is_string()) {
        return node.get<std::string>();
    }
    return Error(ErrorCode::InvalidData,
        "density_function type '" + std::string(type) +
            "' noise field must be a string reference "
            "(inline noise parameters not supported; noises are data-driven)");
}

// ============================================================================
// 工厂
// ============================================================================

Result<std::unique_ptr<DensityFunction>> createConstant(const json& j, const ResolveContext& ctx)
{
    (void)ctx;
    // {"type":"minecraft:constant","argument": <number>} 或裸数字（由 resolveInline 处理）
    if (j.contains("argument")) {
        if (j["argument"].is_number()) {
            return factory::constant(j["argument"].get<f64>());
        }
        return Error(ErrorCode::InvalidData, "constant argument must be a number");
    }
    // {} → 0
    return factory::constant(0.0);
}

Result<std::unique_ptr<DensityFunction>> createYClampedGradient(const json& j, const ResolveContext& ctx)
{
    (void)ctx;
    auto fromY = readInt(j, "from_y");
    if (fromY.failed()) {
        return fromY.error();
    }
    auto toY = readInt(j, "to_y");
    if (toY.failed()) {
        return toY.error();
    }
    auto fromValue = readDouble(j, "from_value");
    if (fromValue.failed()) {
        return fromValue.error();
    }
    auto toValue = readDouble(j, "to_value");
    if (toValue.failed()) {
        return toValue.error();
    }
    return factory::yClampedGradient(fromY.value(), toY.value(), fromValue.value(), toValue.value());
}

Result<std::unique_ptr<DensityFunction>> createClamp(const json& j, const ResolveContext& ctx)
{
    auto input = requireChild(j, "input", ctx, "clamp");
    if (input.failed()) {
        return input.error();
    }
    auto minVal = readDouble(j, "min");
    if (minVal.failed()) {
        return minVal.error();
    }
    auto maxVal = readDouble(j, "max");
    if (maxVal.failed()) {
        return maxVal.error();
    }
    return factory::clamp(input.value(), minVal.value(), maxVal.value());
}

/// 通用单参数映射工厂（abs/square/cube/half_negative/quarter_negative/squeeze/invert）
Result<std::unique_ptr<DensityFunction>> createMapped(
    const json& j, const ResolveContext& ctx, MappedType mappedType, std::string_view type)
{
    auto input = requireChild(j, "argument", ctx, type);
    if (input.failed()) {
        return input.error();
    }
    // Mapped IS-A DensityFunction；Result<unique_ptr<DensityFunction>> 要求精确基类，
    // 显式向上转。input.value() 返回 prvalue（按值），不可 std::move（-Wpessimizing-move）。
    return std::unique_ptr<DensityFunction>(std::make_unique<Mapped>(input.value(), mappedType));
}

/// 通用双参数工厂（add/mul/min/max）
Result<std::unique_ptr<DensityFunction>> createTwoArg(
    const json& j, const ResolveContext& ctx, TwoArgumentType argType, std::string_view type)
{
    auto arg1 = requireChild(j, "argument1", ctx, type);
    if (arg1.failed()) {
        return arg1.error();
    }
    auto arg2 = requireChild(j, "argument2", ctx, type);
    if (arg2.failed()) {
        return arg2.error();
    }
    return std::unique_ptr<DensityFunction>(std::make_unique<TwoArgument>(arg1.value(), arg2.value(), argType));
}

Result<std::unique_ptr<DensityFunction>> createLerp(const json& j, const ResolveContext& ctx)
{
    // MC 字段：argument(delta), from(start), to(end)
    auto delta = requireChild(j, "argument", ctx, "lerp");
    if (delta.failed()) {
        return delta.error();
    }
    auto start = requireChild(j, "from", ctx, "lerp");
    if (start.failed()) {
        return start.error();
    }
    auto end = requireChild(j, "to", ctx, "lerp");
    if (end.failed()) {
        return end.error();
    }
    return factory::lerp(delta.value(), start.value(), end.value());
}

Result<std::unique_ptr<DensityFunction>> createNoise(const json& j, const ResolveContext& ctx)
{
    auto noiseName = readNoiseName(j, "noise", "noise");
    if (noiseName.failed()) {
        return noiseName.error();
    }
    auto xzScale = readDouble(j, "xz_scale");
    if (xzScale.failed()) {
        return xzScale.error();
    }
    auto yScale = readDouble(j, "y_scale");
    if (yScale.failed()) {
        return yScale.error();
    }
    return std::unique_ptr<DensityFunction>(std::make_unique<UnboundNoiseLeaf>(noiseName.value(),
        xzScale.value(),
        yScale.value(),
        UnboundNoiseLeaf::Kind::Noise,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        std::nullopt,
        std::nullopt,
        WeirdScaledSamplerType::Type1,
        0.0,
        0.0,
        0.0));
}

Result<std::unique_ptr<DensityFunction>> createShiftedNoise(const json& j, const ResolveContext& ctx)
{
    auto noiseName = readNoiseName(j, "noise", "shifted_noise");
    if (noiseName.failed()) {
        return noiseName.error();
    }
    auto xzScale = readDouble(j, "xz_scale");
    if (xzScale.failed()) {
        return xzScale.error();
    }
    auto yScale = readDouble(j, "y_scale");
    if (yScale.failed()) {
        return yScale.error();
    }
    auto shiftX = requireChild(j, "shift_x", ctx, "shifted_noise");
    if (shiftX.failed()) {
        return shiftX.error();
    }
    auto shiftY = requireChild(j, "shift_y", ctx, "shifted_noise");
    if (shiftY.failed()) {
        return shiftY.error();
    }
    auto shiftZ = requireChild(j, "shift_z", ctx, "shifted_noise");
    if (shiftZ.failed()) {
        return shiftZ.error();
    }
    return std::unique_ptr<DensityFunction>(std::make_unique<UnboundNoiseLeaf>(noiseName.value(),
        xzScale.value(),
        yScale.value(),
        UnboundNoiseLeaf::Kind::ShiftedNoise,
        shiftX.value(),
        shiftY.value(),
        shiftZ.value(),
        nullptr,
        std::nullopt,
        std::nullopt,
        WeirdScaledSamplerType::Type1,
        0.0,
        0.0,
        0.0));
}

/// shift_a/shift_b/shift：argument 字段是噪声 RL
Result<std::unique_ptr<DensityFunction>> createShift(
    const json& j, const ResolveContext& ctx, UnboundNoiseLeaf::Kind kind, std::string_view type)
{
    (void)ctx;
    auto noiseName = readNoiseName(j, "argument", type);
    if (noiseName.failed()) {
        return noiseName.error();
    }
    return std::unique_ptr<DensityFunction>(std::make_unique<UnboundNoiseLeaf>(noiseName.value(),
        0.0,
        0.0,
        kind,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        std::nullopt,
        std::nullopt,
        WeirdScaledSamplerType::Type1,
        0.0,
        0.0,
        0.0));
}

Result<std::unique_ptr<DensityFunction>> createWeirdScaledSampler(const json& j, const ResolveContext& ctx)
{
    auto input = requireChild(j, "input", ctx, "weird_scaled_sampler");
    if (input.failed()) {
        return input.error();
    }
    auto noiseName = readNoiseName(j, "noise", "weird_scaled_sampler");
    if (noiseName.failed()) {
        return noiseName.error();
    }
    // rarity_value_mapper: "type_1" | "type_2"
    WeirdScaledSamplerType weirdType = WeirdScaledSamplerType::Type1;
    if (j.contains("rarity_value_mapper")) {
        const auto& rvm = j["rarity_value_mapper"];
        if (rvm.is_string()) {
            const auto s = rvm.get<std::string>();
            const auto key = stripNamespace(s);
            if (key == "type_2") {
                weirdType = WeirdScaledSamplerType::Type2;
            } else if (key != "type_1") {
                return Error(ErrorCode::InvalidData, "weird_scaled_sampler rarity_value_mapper must be type_1/type_2");
            }
        } else {
            return Error(ErrorCode::InvalidData, "weird_scaled_sampler rarity_value_mapper must be a string");
        }
    }
    return std::unique_ptr<DensityFunction>(std::make_unique<UnboundNoiseLeaf>(noiseName.value(),
        0.0,
        0.0,
        UnboundNoiseLeaf::Kind::WeirdScaledSampler,
        nullptr,
        nullptr,
        nullptr,
        input.value(),
        std::nullopt,
        std::nullopt,
        weirdType,
        0.0,
        0.0,
        0.0));
}

Result<std::unique_ptr<DensityFunction>> createOldBlendedNoise(const json& j, const ResolveContext& ctx)
{
    (void)ctx;
    auto xzScale = readDouble(j, "xz_scale");
    if (xzScale.failed()) {
        return xzScale.error();
    }
    auto yScale = readDouble(j, "y_scale");
    if (yScale.failed()) {
        return yScale.error();
    }
    auto xzFactor = readDouble(j, "xz_factor");
    if (xzFactor.failed()) {
        return xzFactor.error();
    }
    auto yFactor = readDouble(j, "y_factor");
    if (yFactor.failed()) {
        return yFactor.error();
    }
    auto smear = readDouble(j, "smear_scale_multiplier");
    if (smear.failed()) {
        return smear.error();
    }
    // old_blended_noise 的种子原版走 fromHashOf("minecraft:terrain")（name-hash），
    // 构造真实 BlendedNoise 需 RandomState，解析期存占位，NoiseBindingVisitor 替换。
    // noiseName 固定 "minecraft:terrain"，与原版 RandomState.NoiseWiringHelper.wrapNew 一致。
    return std::unique_ptr<DensityFunction>(std::make_unique<UnboundNoiseLeaf>(std::string("minecraft:terrain"),
        xzScale.value(),
        yScale.value(),
        UnboundNoiseLeaf::Kind::OldBlendedNoise,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        std::nullopt,
        std::nullopt,
        WeirdScaledSamplerType::Type1,
        xzFactor.value(),
        yFactor.value(),
        smear.value()));
}

Result<std::unique_ptr<DensityFunction>> createRangeChoice(const json& j, const ResolveContext& ctx)
{
    auto input = requireChild(j, "input", ctx, "range_choice");
    if (input.failed()) {
        return input.error();
    }
    auto minVal = readDouble(j, "min_inclusive");
    if (minVal.failed()) {
        return minVal.error();
    }
    auto maxVal = readDouble(j, "max_exclusive");
    if (maxVal.failed()) {
        return maxVal.error();
    }
    auto whenIn = requireChild(j, "when_in_range", ctx, "range_choice");
    if (whenIn.failed()) {
        return whenIn.error();
    }
    auto whenOut = requireChild(j, "when_out_of_range", ctx, "range_choice");
    if (whenOut.failed()) {
        return whenOut.error();
    }
    return factory::rangeChoice(input.value(), minVal.value(), maxVal.value(), whenIn.value(), whenOut.value());
}

/// 前向声明：parseSplinePoint 的 value 为嵌套样条对象时回调 parseSpline（见下方定义）
[[nodiscard]] Result<std::unique_ptr<CubicSpline>> parseSpline(const json& j, const ResolveContext& ctx);

/// 递归解析 spline point 的 value：number→f64，对象→嵌套 CubicSpline
Result<SplinePoint> parseSplinePoint(const json& point, const ResolveContext& ctx)
{
    if (!point.is_object()) {
        return Error(ErrorCode::InvalidData, "spline point must be an object");
    }
    auto location = readDouble(point, "location");
    if (location.failed()) {
        return location.error();
    }
    auto derivative = readDouble(point, "derivative");
    if (derivative.failed()) {
        return derivative.error();
    }
    SplinePoint sp;
    sp.location = location.value();
    sp.derivative = derivative.value();

    if (!point.contains("value")) {
        return Error(ErrorCode::InvalidData, "spline point missing 'value'");
    }
    const auto& valueNode = point["value"];
    if (valueNode.is_number()) {
        sp.value = valueNode.get<f64>();
        return sp;
    }
    if (valueNode.is_object()) {
        // 嵌套样条：{coordinate, points:[...]}
        auto nested = parseSpline(valueNode, ctx);
        if (nested.failed()) {
            return nested.error();
        }
        sp.value = std::shared_ptr<CubicSpline>(nested.value().release());
        return sp;
    }
    return Error(ErrorCode::InvalidData, "spline point value must be number or nested spline object");
}

/// 解析 spline 节点 {coordinate: <DF>, points: [{location, derivative, value}]}。
/// 返回 CubicSpline（ DensityFunction 子类），供外层 createSpline 包成 unique_ptr<DensityFunction>，
/// 或供嵌套 parseSplinePoint 取 shared_ptr<CubicSpline> 作 point.value。
Result<std::unique_ptr<CubicSpline>> parseSpline(const json& j, const ResolveContext& ctx)
{
    auto coordinate = requireChild(j, "coordinate", ctx, "spline");
    if (coordinate.failed()) {
        return coordinate.error();
    }
    if (!j.contains("points") || !j["points"].is_array()) {
        return Error(ErrorCode::InvalidData, "spline missing 'points' array");
    }
    std::vector<SplinePoint> points;
    points.reserve(j["points"].size());
    for (const auto& point : j["points"]) {
        auto sp = parseSplinePoint(point, ctx);
        if (sp.failed()) {
            return sp.error();
        }
        points.push_back(sp.value());
    }
    auto sharedInput = std::shared_ptr<DensityFunction>(coordinate.value());
    return std::make_unique<CubicSpline>(std::move(sharedInput), std::move(points));
}

Result<std::unique_ptr<DensityFunction>> createSpline(const json& j, const ResolveContext& ctx)
{
    // 外层 {"type":"minecraft:spline","spline":{coordinate,points}}
    const json& splineNode = (j.contains("spline") && j["spline"].is_object()) ? j["spline"] : j;
    auto nested = parseSpline(splineNode, ctx);
    if (nested.failed()) {
        return nested.error();
    }
    // CubicSpline IS-A DensityFunction，向上转 unique_ptr<DensityFunction>
    return std::unique_ptr<DensityFunction>(nested.value().release());
}

Result<std::unique_ptr<DensityFunction>> createEndIslands(const json& j, const ResolveContext& ctx)
{
    (void)j;
    (void)ctx;
    // end_islands 无参，seed 固定 0，解析期占位，NoiseBindingVisitor 替换
    return std::unique_ptr<DensityFunction>(std::make_unique<UnboundEndIslands>());
}

Result<std::unique_ptr<DensityFunction>> createBeardifier(const json& j, const ResolveContext& ctx)
{
    (void)j;
    (void)ctx;
    return factory::beardifierMarker();
}

/// 通用 Marker 工厂（interpolated/cache_once/cache_all_in_cell/flat_cache/cache_2d）
Result<std::unique_ptr<DensityFunction>> createMarker(
    const json& j, const ResolveContext& ctx, MarkerType markerType, std::string_view type)
{
    auto wrapped = requireChild(j, "argument", ctx, type);
    if (wrapped.failed()) {
        return wrapped.error();
    }
    return std::unique_ptr<DensityFunction>(std::make_unique<Marker>(markerType, wrapped.value()));
}

Result<std::unique_ptr<DensityFunction>> createBlendAlpha(const json& j, const ResolveContext& ctx)
{
    (void)j;
    (void)ctx;
    // 原版 blend_alpha 恒返回 1.0（Blender 系统已移除，新世界恒等）
    return factory::constant(1.0);
}

Result<std::unique_ptr<DensityFunction>> createBlendOffset(const json& j, const ResolveContext& ctx)
{
    (void)j;
    (void)ctx;
    // 原版 blend_offset 恒返回 0.0
    return factory::constant(0.0);
}

Result<std::unique_ptr<DensityFunction>> createBlendDensity(const json& j, const ResolveContext& ctx)
{
    // 原版 blend_density 包装 input，新世界 Blender.empty() 直通 → 即返回 input 本身
    auto input = requireChild(j, "argument", ctx, "blend_density");
    if (input.failed()) {
        return input.error();
    }
    return input.value();
}

Result<std::unique_ptr<DensityFunction>> createFindTopSurface(const json& j, const ResolveContext& ctx)
{
    auto density = requireChild(j, "density", ctx, "find_top_surface");
    if (density.failed()) {
        return density.error();
    }
    auto upperBound = requireChild(j, "upper_bound", ctx, "find_top_surface");
    if (upperBound.failed()) {
        return upperBound.error();
    }
    auto lowerBound = readInt(j, "lower_bound");
    if (lowerBound.failed()) {
        return lowerBound.error();
    }
    auto cellHeight = readInt(j, "cell_height");
    if (cellHeight.failed()) {
        return cellHeight.error();
    }
    return factory::findTopSurface(density.value(), upperBound.value(), lowerBound.value(), cellHeight.value());
}

} // namespace

DensityFunctionTypeRegistry& DensityFunctionTypeRegistry::instance()
{
    static DensityFunctionTypeRegistry s_instance;
    return s_instance;
}

void DensityFunctionTypeRegistry::registerType(const std::string& type, Factory factory)
{
    m_factories[type] = std::move(factory);
}

Result<std::unique_ptr<DensityFunction>> DensityFunctionTypeRegistry::create(
    const std::string& type, const nlohmann::json& json, const ResolveContext& ctx) const
{
    const std::string key = stripNamespace(type);
    const auto it = m_factories.find(key);
    if (it == m_factories.end()) {
        return Error(ErrorCode::NotFound,
            "Unregistered density_function type: '" + type +
                "'. Implement it and register in DensityFunctionTypeRegistry.");
    }
    return it->second(json, ctx);
}

bool DensityFunctionTypeRegistry::has(const std::string& type) const noexcept
{
    return m_factories.contains(stripNamespace(type));
}

void DensityFunctionTypeRegistry::clear() noexcept
{
    m_factories.clear();
}

void initializeBuiltinDensityFunctionTypes()
{
    auto& r = DensityFunctionTypeRegistry::instance();

    r.registerType("constant", createConstant);
    r.registerType("y_clamped_gradient", createYClampedGradient);

    r.registerType("clamp", createClamp);

    // 单参数映射（7）
    r.registerType(
        "abs", [](const json& j, const ResolveContext& ctx) { return createMapped(j, ctx, MappedType::Abs, "abs"); });
    r.registerType("square",
        [](const json& j, const ResolveContext& ctx) { return createMapped(j, ctx, MappedType::Square, "square"); });
    r.registerType("cube",
        [](const json& j, const ResolveContext& ctx) { return createMapped(j, ctx, MappedType::Cube, "cube"); });
    r.registerType("half_negative", [](const json& j, const ResolveContext& ctx) {
        return createMapped(j, ctx, MappedType::HalfNegative, "half_negative");
    });
    r.registerType("quarter_negative", [](const json& j, const ResolveContext& ctx) {
        return createMapped(j, ctx, MappedType::QuarterNegative, "quarter_negative");
    });
    r.registerType("squeeze",
        [](const json& j, const ResolveContext& ctx) { return createMapped(j, ctx, MappedType::Squeeze, "squeeze"); });
    r.registerType("invert",
        [](const json& j, const ResolveContext& ctx) { return createMapped(j, ctx, MappedType::Invert, "invert"); });

    // 双参数（4）
    r.registerType("add",
        [](const json& j, const ResolveContext& ctx) { return createTwoArg(j, ctx, TwoArgumentType::Add, "add"); });
    r.registerType("mul",
        [](const json& j, const ResolveContext& ctx) { return createTwoArg(j, ctx, TwoArgumentType::Mul, "mul"); });
    r.registerType("min",
        [](const json& j, const ResolveContext& ctx) { return createTwoArg(j, ctx, TwoArgumentType::Min, "min"); });
    r.registerType("max",
        [](const json& j, const ResolveContext& ctx) { return createTwoArg(j, ctx, TwoArgumentType::Max, "max"); });

    r.registerType("lerp", createLerp);
    r.registerType("noise", createNoise);
    r.registerType("shifted_noise", createShiftedNoise);

    r.registerType("shift_a", [](const json& j, const ResolveContext& ctx) {
        return createShift(j, ctx, UnboundNoiseLeaf::Kind::ShiftA, "shift_a");
    });
    r.registerType("shift_b", [](const json& j, const ResolveContext& ctx) {
        return createShift(j, ctx, UnboundNoiseLeaf::Kind::ShiftB, "shift_b");
    });
    r.registerType("shift", [](const json& j, const ResolveContext& ctx) {
        return createShift(j, ctx, UnboundNoiseLeaf::Kind::Shift, "shift");
    });

    r.registerType("weird_scaled_sampler", createWeirdScaledSampler);
    r.registerType("old_blended_noise", createOldBlendedNoise);
    r.registerType("range_choice", createRangeChoice);
    r.registerType("spline", createSpline);
    r.registerType("end_islands", createEndIslands);
    r.registerType("beardifier", createBeardifier);

    // Marker（5）
    r.registerType("interpolated", [](const json& j, const ResolveContext& ctx) {
        return createMarker(j, ctx, MarkerType::Interpolated, "interpolated");
    });
    r.registerType("cache_once", [](const json& j, const ResolveContext& ctx) {
        return createMarker(j, ctx, MarkerType::CacheOnce, "cache_once");
    });
    r.registerType("cache_all_in_cell", [](const json& j, const ResolveContext& ctx) {
        return createMarker(j, ctx, MarkerType::CacheAllInCell, "cache_all_in_cell");
    });
    r.registerType("flat_cache", [](const json& j, const ResolveContext& ctx) {
        return createMarker(j, ctx, MarkerType::FlatCache, "flat_cache");
    });
    r.registerType("cache_2d",
        [](const json& j, const ResolveContext& ctx) { return createMarker(j, ctx, MarkerType::Cache2D, "cache_2d"); });

    // blend（3，Blender 已移除，新世界恒等）
    r.registerType("blend_alpha", createBlendAlpha);
    r.registerType("blend_offset", createBlendOffset);
    r.registerType("blend_density", createBlendDensity);

    r.registerType("find_top_surface", createFindTopSurface);
}

} // namespace mc::world::gen::density
