#include "common/util/math/MathUtils.hpp"
#include "probe.hpp"
#include "server/interaction/DigSpeed.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;
using server::interaction::digSpeed;
using server::interaction::DigSpeedInputs;

/**
 * @brief 与探针协议一致的浮点输出
 *
 * 挖掘速度是 f32，两侧必须按同一套位模式比较，否则最后一位的差异会被十进制打印抹掉。
 */
std::string floatBits(f32 value)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", std::bit_cast<u32>(value));
    return buffer;
}

/**
 * @brief 输入场景
 *
 * 每个场景只固定一组离散的输入组合，输出是它们代入公式后的结果。之所以用固定的场景表
 * 而不是让外部传任意输入，是因为这条链的语义主要由「哪些因子同时在场」决定：
 * 单独测每一环都会通过，出问题的永远是组合（比如效率附魔遇上空手、急迫遇上疲劳）。
 */
struct Scenario {
    const char* name;
    DigSpeedInputs inputs;
};

const std::vector<Scenario>& scenarios()
{
    // 效率附魔的四档取值：0（无）、2（I 级，1²+1）、5（II 级，2²+1）、26（V 级，5²+1）
    constexpr f32 kEff0 = 0.0f;
    constexpr f32 kEff2 = 2.0f;
    constexpr f32 kEff5 = 5.0f;
    constexpr f32 kEff26 = 26.0f;

    // toolSpeed 的三档：1.0 空手、8.0 钻石镐对石头、0.2 木剑对石头（比手还慢）
    constexpr f32 kHand = 1.0f;
    constexpr f32 kPickaxe = 8.0f;
    constexpr f32 kSlowTool = 0.2f;

    constexpr f32 kBreak1 = 1.0f;
    constexpr f32 kBreak2 = 2.0f;

    constexpr f32 kSubmerged = 0.2f;
    constexpr f32 kAquaAffinity = 1.0f;

    static const std::vector<Scenario> table = {
        // —— 基线：只有工具速度 ——
        {"hand_air", {kHand, kEff0, -1, -1, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_air", {kPickaxe, kEff0, -1, -1, -1, kBreak1, kSubmerged, false, true}},
        {"slowtool_air", {kSlowTool, kEff0, -1, -1, -1, kBreak1, kSubmerged, false, true}},

        // —— 效率附魔：只有 toolSpeed > 1 才生效 ——
        {"hand_eff2", {kHand, kEff2, -1, -1, -1, kBreak1, kSubmerged, false, true}},
        {"hand_eff26", {kHand, kEff26, -1, -1, -1, kBreak1, kSubmerged, false, true}},
        {"slowtool_eff5", {kSlowTool, kEff5, -1, -1, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_eff2", {kPickaxe, kEff2, -1, -1, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_eff5", {kPickaxe, kEff5, -1, -1, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_eff26", {kPickaxe, kEff26, -1, -1, -1, kBreak1, kSubmerged, false, true}},

        // —— 急迫：每级 +20%，amplifier 从 0 起 ——
        {"pickaxe_haste0", {kPickaxe, kEff0, 0, -1, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_haste1", {kPickaxe, kEff0, 1, -1, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_haste2", {kPickaxe, kEff0, 2, -1, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_haste9", {kPickaxe, kEff0, 9, -1, -1, kBreak1, kSubmerged, false, true}},

        // —— 潮涌能量：与急迫取等级较高者，不叠加 ——
        {"pickaxe_conduit0", {kPickaxe, kEff0, -1, 0, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_conduit2", {kPickaxe, kEff0, -1, 2, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_haste1_conduit2", {kPickaxe, kEff0, 1, 2, -1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_haste3_conduit1", {kPickaxe, kEff0, 3, 1, -1, kBreak1, kSubmerged, false, true}},

        // —— 挖掘疲劳：四档精确常量，不是几何级数 ——
        {"pickaxe_fatigue0", {kPickaxe, kEff0, -1, -1, 0, kBreak1, kSubmerged, false, true}},
        {"pickaxe_fatigue1", {kPickaxe, kEff0, -1, -1, 1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_fatigue2", {kPickaxe, kEff0, -1, -1, 2, kBreak1, kSubmerged, false, true}},
        {"pickaxe_fatigue3", {kPickaxe, kEff0, -1, -1, 3, kBreak1, kSubmerged, false, true}},
        {"pickaxe_fatigue7", {kPickaxe, kEff0, -1, -1, 7, kBreak1, kSubmerged, false, true}},

        // —— 急迫 + 疲劳同时在场（两者相乘） ——
        {"pickaxe_haste2_fatigue1", {kPickaxe, kEff0, 2, -1, 1, kBreak1, kSubmerged, false, true}},
        {"pickaxe_eff5_haste2_fatigue2", {kPickaxe, kEff5, 2, -1, 2, kBreak1, kSubmerged, false, true}},

        // —— block_break_speed 属性 ——
        {"pickaxe_break2", {kPickaxe, kEff0, -1, -1, -1, kBreak2, kSubmerged, false, true}},
        {"pickaxe_break2_eff5_haste1", {kPickaxe, kEff5, 1, -1, -1, kBreak2, kSubmerged, false, true}},

        // —— 水下：默认降到 1/5，水下速掘把它拉回 1 ——
        {"pickaxe_wet", {kPickaxe, kEff0, -1, -1, -1, kBreak1, kSubmerged, true, true}},
        {"pickaxe_wet_aqua", {kPickaxe, kEff0, -1, -1, -1, kBreak1, kAquaAffinity, true, true}},
        {"hand_wet", {kHand, kEff0, -1, -1, -1, kBreak1, kSubmerged, true, true}},

        // —— 空中：速度除以 5 ——
        {"pickaxe_falling", {kPickaxe, kEff0, -1, -1, -1, kBreak1, kSubmerged, false, false}},
        {"pickaxe_wet_falling", {kPickaxe, kEff0, -1, -1, -1, kBreak1, kSubmerged, true, false}},

        // —— 全因子叠加 ——
        {"all_factors", {kPickaxe, kEff5, 2, 1, 1, kBreak2, kAquaAffinity, true, false}},
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
            return json{{"unit", requested}, {"speed", floatBits(digSpeed(scenario.inputs))}}.dump();
        }
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(dig_speed, &writeList, &runLine)

} // namespace mc::diff
