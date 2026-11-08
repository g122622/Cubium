#include "probe.hpp"

#include "common/world/time/timeline/KeyframeTrack.hpp"
#include "common/world/time/timeline/KeyframeTrackSampler.hpp"

#include <nlohmann/json.hpp>

#include <bit>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;
using world::timeline::EasingType;
using world::timeline::Keyframe;
using world::timeline::KeyframeTrack;
using world::timeline::KeyframeTrackSampler;

std::string floatBits(f32 value)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", std::bit_cast<u32>(value));
    return buffer;
}

/**
 * @brief 一条待测轨道
 *
 * 每条轨道只声明关键帧、缓动与周期，采样点由外面统一给出。这样两侧拿到的是**同一份
 * 轨道定义**，不会因为一侧少写一个关键帧而误判。
 */
struct TrackCase {
    const char* id;
    std::vector<Keyframe<f32>> keyframes;
    EasingType easing;
    std::optional<i32> period;
};

const std::vector<TrackCase>& trackCases()
{
    return *[] {
        static const std::vector<TrackCase> table = {
            // —— 单关键帧：任何时刻都返回该值 ——
            {"single", {{0, 5.0f}}, EasingType::LINEAR, std::nullopt},

            // —— 两关键帧、无周期：区间外两端钳位 ——
            {"two_linear", {{0, 0.0f}, {100, 10.0f}}, EasingType::LINEAR, std::nullopt},
            {"two_eased", {{0, 0.0f}, {100, 10.0f}}, EasingType::IN_OUT_SINE, std::nullopt},

            // —— 三关键帧：中间点必须精确命中，不允许出现插值误差 ——
            {"three_linear", {{0, 0.0f}, {50, 10.0f}, {100, 0.0f}}, EasingType::LINEAR, std::nullopt},
            {"three_out_bounce", {{0, 0.0f}, {50, 10.0f}, {100, 0.0f}}, EasingType::OUT_BOUNCE, std::nullopt},

            // —— 带周期：末尾与开头之间要能回绕 ——
            {"period_24000", {{0, 0.0f}, {12000, 1.0f}}, EasingType::LINEAR, 24000},
            {"period_24000_eased", {{0, 0.0f}, {12000, 1.0f}}, EasingType::IN_OUT_QUAD, 24000},
            {"period_24000_three", {{0, 0.0f}, {6000, 1.0f}, {18000, 0.0f}}, EasingType::LINEAR, 24000},

            // —— 非零起点的关键帧：回绕段从 last.ticks - period 起算 ——
            {"period_offset", {{1000, 0.0f}, {13000, 1.0f}}, EasingType::LINEAR, 24000},

            // —— 玩家时刻表里真实出现的形状：三段常量回绕 ——
            {"villager_schedule", {{0, 0.0f}, {10, 1.0f}, {100, 1.0f}}, EasingType::CONSTANT, 24000},
        };
        return &table;
    }();
}

/**
 * @brief 采样时刻
 *
 * 特意包含负数与超出周期的大值：`loopTicks` 用 `Math.floorMod` 而不是取余，
 * 负数必须回绕到周期末尾而不是 0；超过一个周期要正确折回。
 */
const std::vector<i64>& sampleTicks()
{
    static const std::vector<i64> table = {
        -24001,
        -24000,
        -12000,
        -1,
        0,
        1,
        25,
        49,
        50,
        51,
        75,
        99,
        100,
        101,
        6000,
        12000,
        18000,
        23999,
        24000,
        24001,
        48000,
        100000,
    };
    return table;
}

std::unique_ptr<KeyframeTrackSampler<f32>> buildSampler(const TrackCase& trackCase)
{
    const KeyframeTrack<f32> track(trackCase.keyframes, trackCase.easing);
    // 用生产代码自己的插值函数：差分测试要覆盖真实路径，而不是探针另写一份。
    const world::attribute::LerpFunction<f32> lerp = world::attribute::LerpFunction<f32>::ofFloat();
    return std::make_unique<KeyframeTrackSampler<f32>>(track.bakeSampler(trackCase.period, lerp));
}

void writeList(std::FILE* out)
{
    for (const auto& trackCase : trackCases()) {
        for (size_t i = 0; i < sampleTicks().size(); ++i) {
            std::fprintf(out, "%s@%zu\n", trackCase.id, i);
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
    const std::string trackId = requested.substr(0, at);
    const size_t tickIndex = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (tickIndex >= sampleTicks().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    for (const auto& trackCase : trackCases()) {
        if (trackId == trackCase.id) {
            const auto sampler = buildSampler(trackCase);
            const f32 value = sampler->sample(sampleTicks()[tickIndex]);
            return json{{"unit", requested}, {"value", floatBits(value)}}.dump();
        }
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(timeline_sample, &writeList, &runLine)

} // namespace mc::diff
