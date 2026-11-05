#include "probe.hpp"

#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "server/world/gen/density/DensityFunctions.hpp"
#include "server/world/gen/noise/NormalNoise.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <bit>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;
using world::gen::density::Constant;
using world::gen::density::DensityFunction;
using world::gen::density::ShiftedNoise;
using world::gen::noise::NormalNoise;

std::string doubleBits(f64 value)
{
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(std::bit_cast<u64>(value)));
    return buffer;
}

/**
 * @brief 坐标平移函数的形态
 *
 * `ShiftedNoise` 的语义是「坐标先缩放再叠加位移，然后把结果喂给噪声」。位移本身是一个
 * 密度函数，这里用两种形态覆盖：
 *
 * - `constant`：位移是常量，覆盖缩放加法的纯算术；
 * - `linear`：位移随坐标线性变化，覆盖非零位移路径以及它带来的逐点差异。
 */
enum class ShiftKind : u8 { Constant, Linear };

/**
 * @brief 线性位移密度函数：value = ax*x + ay*y + az*z + base
 *
 * 只用加乘，不引入噪声，便于把差异定位到 `ShiftedNoise` 自身的坐标运算。
 */
class LinearShift final : public DensityFunction {
public:
    LinearShift(f64 ax, f64 ay, f64 az, f64 base)
        : m_ax(ax)
        , m_ay(ay)
        , m_az(az)
        , m_base(base)
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 x = static_cast<f64>(blockX) * m_ax;
        const f64 y = static_cast<f64>(blockY) * m_ay;
        const f64 z = static_cast<f64>(blockZ) * m_az;
        return m_base + x + y + z;
    }

    [[nodiscard]] f64 minValue() const override
    {
        // 采样范围内按保守估计给出，只影响边界字段，不影响 compute。
        return -1.0e6;
    }

    [[nodiscard]] f64 maxValue() const override
    {
        return 1.0e6;
    }

    DENSITY_FUNCTION_MAP_ALL_LEAF(LinearShift, 0.0, 0.0, 0.0, 0.0)
private:
    f64 m_ax;
    f64 m_ay;
    f64 m_az;
    f64 m_base;
};

/**
 * @brief 一条 ShiftedNoise 用例
 *
 * 噪声参数两侧共用同一份（种子、首个倍频、振幅），位移形态与缩放在这里给出。
 */
struct ShiftCase {
    const char* id;
    i64 seed;
    i32 firstOctave;
    std::vector<f64> amplitudes;
    f64 xzScale;
    f64 yScale;
    ShiftKind shiftKind;
    /// constant 形态使用 base；linear 形态使用 (ax, ay, az, base)
    f64 shiftBase;
    f64 shiftAx;
    f64 shiftAy;
    f64 shiftAz;
};

const std::vector<ShiftCase>& shiftCases()
{
    static const std::vector<ShiftCase> table = {
        // 位移全零：退化成普通 NoiseDensity，用来确认基础路径
        {"shift_zero", 1234567, -8, {1.0, 1.0, 1.0, 1.0}, 0.25, 0.25, ShiftKind::Constant, 0.0, 0.0, 0.0, 0.0},

        // 位移是常量：覆盖「缩放 + 常量」的加法顺序
        {"shift_const", 1234567, -8, {1.0, 1.0, 1.0, 1.0}, 0.25, 0.25, ShiftKind::Constant, 0.5, 0.0, 0.0, 0.0},

        // 位移是常量且很小：与大坐标相乘后仍可能被吞掉，覆盖吸收边界
        {"shift_tiny", 987654321, -6, {1.0, 0.5}, 0.1, 0.1, ShiftKind::Constant, 1.0e-9, 0.0, 0.0, 0.0},

        // 位移随坐标线性变化：三个方向给不同系数
        {"shift_linear", -42, -7, {1.0, 1.0, 1.0}, 0.011, 0.011, ShiftKind::Linear, 0.125, 0.75, -0.5, 0.25},

        // 线性位移且缩放不对称
        {"shift_aniso", 555, -8, {1.0, 1.0, 0.5, 0.25}, 0.5, 0.125, ShiftKind::Linear, -0.375, 1.5, 0.0, -1.0},
    };
    return table;
}

/// 采样坐标：覆盖原点、各轴单独偏移、负坐标与大坐标。
const std::vector<std::array<i32, 3>>& samplePoints()
{
    static const std::vector<std::array<i32, 3>> table = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {-1, -1, -1},
        {16, 64, -16},
        {100, -32, 200},
        {-1000, 5, 1000},
        {12345, 200, -6789},
    };
    return table;
}

std::unique_ptr<DensityFunction> buildShift(const ShiftCase& shiftCase, u8 axis)
{
    if (shiftCase.shiftKind == ShiftKind::Constant) {
        // 只有对应轴的常量位移生效，其余两轴为 0，便于区分每条轴。
        const f64 base = (axis == 0) ? shiftCase.shiftBase : 0.0;
        return std::make_unique<Constant>(base);
    }
    const f64 ax = (axis == 0) ? shiftCase.shiftAx : 0.0;
    const f64 ay = (axis == 1) ? shiftCase.shiftAy : 0.0;
    const f64 az = (axis == 2) ? shiftCase.shiftAz : 0.0;
    const f64 base = shiftCase.shiftBase;
    return std::make_unique<LinearShift>(ax, ay, az, base);
}

std::unique_ptr<ShiftedNoise> buildFunction(const ShiftCase& shiftCase)
{
    math::JavaLegacyRandom rng(shiftCase.seed);
    auto noise = std::make_shared<NormalNoise>(rng, shiftCase.firstOctave, shiftCase.amplitudes);
    return std::make_unique<ShiftedNoise>(noise,
        shiftCase.xzScale,
        shiftCase.yScale,
        buildShift(shiftCase, 0),
        buildShift(shiftCase, 1),
        buildShift(shiftCase, 2));
}

void writeList(std::FILE* out)
{
    for (const auto& shiftCase : shiftCases()) {
        for (size_t i = 0; i < samplePoints().size(); ++i) {
            std::fprintf(out, "%s@%zu\n", shiftCase.id, i);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t at = requested.rfind('@');
    if (at == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const std::string caseId = requested.substr(0, at);
    const size_t index = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (index >= samplePoints().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    for (const auto& shiftCase : shiftCases()) {
        if (caseId != shiftCase.id) {
            continue;
        }
        // 直接构造生产代码的 ShiftedNoise，不在这里重写坐标叠加的算术。
        const auto function = buildFunction(shiftCase);
        const auto& point = samplePoints()[index];
        const f64 value = function->compute(point[0], point[1], point[2]);
        return json{{"unit", requested},
            {"value", doubleBits(value)},
            {"min", doubleBits(function->minValue())},
            {"max", doubleBits(function->maxValue())}}
            .dump();
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(shifted_noise, &writeList, &runLine)

} // namespace mc::diff
