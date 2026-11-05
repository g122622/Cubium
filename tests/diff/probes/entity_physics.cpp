#include "probe.hpp"

#include "server/physics/TravelTick.hpp"

#include <nlohmann/json.hpp>

#include <bit>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;
using server::physics::travelInAir;
using server::physics::TravelTickInputs;

std::string floatBits(f32 value)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", std::bit_cast<u32>(value));
    return buffer;
}

/**
 * @brief 一个移动 tick 的场景
 *
 * 只固定离散的输入组合。这条链的语义主要由「哪些因子同时在场」决定：单独测摩擦或
 * 单独测重力都会通过，出问题的永远是组合（悬浮遇上缓降、地面滑度遇上跳跃因子）。
 */
struct Scenario {
    const char* name;
    TravelTickInputs inputs;
};

const std::vector<Scenario>& scenarios()
{
    constexpr f32 kWalkSpeed = 0.1f;
    constexpr f32 kSprintSpeed = 0.13f;
    constexpr f32 kFlyingSpeed = 0.05f;

    constexpr f32 kSlipDefault = 0.6f;
    constexpr f32 kSlipIce = 0.98f;
    constexpr f32 kSlipBlueIce = 0.989f;
    constexpr f32 kSlipSlime = 0.8f;
    constexpr f32 kSlipAir = 1.0f;

    constexpr f32 kGravity = 0.08f;
    constexpr f32 kJumpStrength = 0.42f;
    constexpr f32 kJumpFactorNormal = 1.0f;
    constexpr f32 kJumpFactorHoney = 0.5f;

    static const std::vector<Scenario> table = {
        // —— 地面移动因子：滑度进入三次方除数 ——
        {"ground_walk_default",
            {kWalkSpeed, kSlipDefault, 0.0f, kGravity, true, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"ground_walk_ice",
            {kWalkSpeed, kSlipIce, 0.0f, kGravity, true, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"ground_walk_blue_ice",
            {kWalkSpeed, kSlipBlueIce, 0.0f, kGravity, true, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"ground_walk_slime",
            {kWalkSpeed, kSlipSlime, 0.0f, kGravity, true, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"ground_sprint_default",
            {kSprintSpeed, kSlipDefault, 0.0f, kGravity, true, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"ground_flying_default",
            {kFlyingSpeed, kSlipDefault, 0.0f, kGravity, true, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},

        // —— 空中：移动因子固定 0.02，滑度被忽略 ——
        {"air_walk", {kWalkSpeed, kSlipAir, -0.1f, kGravity, false, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"air_sprint",
            {kSprintSpeed, kSlipAir, -0.1f, kGravity, false, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},

        // —— 重力：下落时竖直速度乘 0.98 ——
        {"fall_from_zero",
            {kWalkSpeed, kSlipAir, 0.0f, kGravity, false, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"fall_rising",
            {kWalkSpeed, kSlipAir, 0.42f, kGravity, false, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"fall_terminal",
            {kWalkSpeed, kSlipAir, -3.0f, kGravity, false, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},

        // —— 缓降：只在下落时把重力压到 0.01 ——
        {"slowfall_falling",
            {kWalkSpeed, kSlipAir, -0.1f, kGravity, false, -1, true, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"slowfall_rising",
            {kWalkSpeed, kSlipAir, 0.3f, kGravity, false, -1, true, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"slowfall_apex",
            {kWalkSpeed, kSlipAir, 0.0f, kGravity, false, -1, true, 0.0f, kJumpFactorNormal, kJumpStrength}},

        // —— 悬浮：向目标速度收敛，且不再乘 0.98 ——
        {"levitation_0_falling",
            {kWalkSpeed, kSlipAir, -0.5f, kGravity, false, 0, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"levitation_1_falling",
            {kWalkSpeed, kSlipAir, -0.5f, kGravity, false, 1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"levitation_0_rising",
            {kWalkSpeed, kSlipAir, 0.5f, kGravity, false, 0, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"levitation_2_rising",
            {kWalkSpeed, kSlipAir, 1.0f, kGravity, false, 2, false, 0.0f, kJumpFactorNormal, kJumpStrength}},

        // —— 跳跃初速度：jump_strength 乘方块因子，加跳跃提升 ——
        {"jump_plain",
            {kWalkSpeed, kSlipDefault, 0.0f, kGravity, true, -1, false, 0.0f, kJumpFactorNormal, kJumpStrength}},
        {"jump_honey",
            {kWalkSpeed, kSlipDefault, 0.0f, kGravity, true, -1, false, 0.0f, kJumpFactorHoney, kJumpStrength}},
        {"jump_boost_1",
            {kWalkSpeed, kSlipDefault, 0.0f, kGravity, true, -1, false, 0.1f, kJumpFactorNormal, kJumpStrength}},
        {"jump_boost_2_honey",
            {kWalkSpeed, kSlipDefault, 0.0f, kGravity, true, -1, false, 0.2f, kJumpFactorHoney, kJumpStrength}},
        {"jump_custom_strength",
            {kWalkSpeed, kSlipDefault, 0.0f, kGravity, true, -1, false, 0.0f, kJumpFactorNormal, 0.5f}},
    };
    return table;
}

void writeList(std::FILE* out)
{
    for (const auto& scenario : scenarios()) {
        std::fprintf(out, "%s\n", scenario.name);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    for (const auto& scenario : scenarios()) {
        if (requested == scenario.name) {
            const auto result = travelInAir(scenario.inputs);
            return json{{"unit", requested},
                {"moveFactor", floatBits(result.moveFactor)},
                {"verticalVelocity", floatBits(result.verticalVelocity)},
                {"jumpPower", floatBits(result.jumpPower)}}
                .dump();
        }
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(entity_physics, &writeList, &runLine)

} // namespace mc::diff
