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

#include "BlockStateProviderParser.hpp"

#include "BlockPredicateParser.hpp"
#include "BlockStateParser.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/feature/state/DualNoiseBlockStateProvider.hpp"
#include "common/world/gen/feature/state/NoiseBlockStateProvider.hpp"
#include "common/world/gen/feature/state/NoiseStateUtils.hpp"
#include "common/world/gen/feature/state/NoiseThresholdBlockStateProvider.hpp"
#include "common/world/gen/feature/state/RandomizedIntBlockStateProvider.hpp"
#include "common/world/gen/feature/state/RotatedBlockStateProvider.hpp"
#include "common/world/gen/feature/state/RuleBasedBlockStateProvider.hpp"
#include "common/world/gen/feature/state/WeightedBlockStateProvider.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::world::gen::feature::parser {
namespace BlockStateProviderParser {

namespace {

std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

f32 getFloat(const nlohmann::json& obj, const char* key, f32 fallback)
{
    return (obj.contains(key) && obj[key].is_number()) ? obj[key].get<f32>() : fallback;
}

/**
 * @brief 解析 {fallback, rules} 结构为 RuleBased 提供者（MC 1.21.11 RuleBasedBlockStateProvider）。
 *        parse(type=rule_based_state_provider) 与 parseRuleBased(无 type) 共用此逻辑。
 */
Result<std::unique_ptr<state::BlockStateProvider>> parseRuleBasedData(const nlohmann::json& providerObj)
{
    if (!providerObj.contains("fallback") || !providerObj["fallback"].is_object()) {
        return Error(ErrorCode::InvalidData, "rule_based_state_provider missing 'fallback' object");
    }
    auto fallbackResult = parse(providerObj["fallback"]);
    if (!fallbackResult.success()) {
        return fallbackResult.error();
    }

    if (!providerObj.contains("rules") || !providerObj["rules"].is_array()) {
        return Error(ErrorCode::InvalidData, "rule_based_state_provider missing 'rules' array");
    }
    std::vector<state::RuleBasedBlockStateProvider::Rule> rules;
    rules.reserve(providerObj["rules"].size());
    for (size_t i = 0; i < providerObj["rules"].size(); ++i) {
        const auto& ruleObj = providerObj["rules"][i];
        if (!ruleObj.is_object() || !ruleObj.contains("if_true") || !ruleObj.contains("then")) {
            return Error(ErrorCode::InvalidData,
                "rule_based_state_provider rule[" + std::to_string(i) + "] missing 'if_true'/'then'");
        }
        auto predResult = BlockPredicateParser::parse(ruleObj["if_true"]);
        if (!predResult.success()) {
            return Error(predResult.error().code(),
                "rule_based_state_provider rule[" + std::to_string(i) + "] if_true: " + predResult.error().message());
        }
        auto thenResult = parse(ruleObj["then"]);
        if (!thenResult.success()) {
            return Error(thenResult.error().code(),
                "rule_based_state_provider rule[" + std::to_string(i) + "] then: " + thenResult.error().message());
        }
        rules.emplace_back(predResult.value(), thenResult.value());
    }

    return Result<std::unique_ptr<state::BlockStateProvider>>(
        std::make_unique<state::RuleBasedBlockStateProvider>(fallbackResult.value(), std::move(rules)));
}

/// MC NormalNoise.NoiseParameters.DIRECT_CODEC：{"firstOctave":N,"amplitudes":[...]}。
Result<world::gen::noise::NormalNoise::NoiseParameters> parseNoiseParameters(const nlohmann::json& json)
{
    if (!json.is_object() || !json.contains("firstOctave") || !json["firstOctave"].is_number_integer() ||
        !json.contains("amplitudes") || !json["amplitudes"].is_array()) {
        return Error(ErrorCode::InvalidData, "noise parameters missing 'firstOctave'/'amplitudes'");
    }
    world::gen::noise::NormalNoise::NoiseParameters params;
    params.firstOctave = json["firstOctave"].get<i32>();
    for (const auto& amp : json["amplitudes"]) {
        if (!amp.is_number()) {
            return Error(ErrorCode::InvalidData, "noise parameters amplitudes must be numbers");
        }
        params.amplitudes.push_back(amp.get<f64>());
    }
    if (params.amplitudes.empty()) {
        return Error(ErrorCode::InvalidData, "noise parameters amplitudes must be non-empty");
    }
    return params;
}

/// 解析非空 BlockState 列表（MC ExtraCodecs.nonEmptyList(BlockState.CODEC.listOf())）。
Result<std::vector<const BlockState*>> parseStateList(const nlohmann::json& json)
{
    if (!json.is_array() || json.empty()) {
        return Error(ErrorCode::InvalidData, "block state list must be a non-empty array");
    }
    std::vector<const BlockState*> states;
    states.reserve(json.size());
    for (size_t i = 0; i < json.size(); ++i) {
        auto r = BlockStateParser::parse(json[i]);
        if (!r.success()) {
            return Error(r.error().code(), "block state list[" + std::to_string(i) + "]: " + r.error().message());
        }
        states.push_back(r.value());
    }
    return states;
}

/// 读取 seed（u64）与 scale（正 float）通用字段。
Result<u64> readSeed(const nlohmann::json& obj)
{
    if (!obj.contains("seed") || !obj["seed"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "noise state provider missing 'seed' integer");
    }
    return static_cast<u64>(obj["seed"].get<i64>());
}

f32 readPositiveFloat(const nlohmann::json& obj, const char* key, f32 fallback)
{
    if (obj.contains(key) && obj[key].is_number()) {
        const f32 v = obj[key].get<f32>();
        return v > 0.0f ? v : fallback;
    }
    return fallback;
}

/// 由 seed + NoiseParameters 构造 NormalNoise。
std::unique_ptr<world::gen::noise::NormalNoise> createNoise(
    u64 seed, const world::gen::noise::NormalNoise::NoiseParameters& params)
{
    return std::make_unique<world::gen::noise::NormalNoise>(seed, params.firstOctave, params.amplitudes);
}

/// MC ExtraCodecs.intervalCodec 的 InclusiveRange<Integer> 三形式解析：
/// 裸整数 N（min=max=N）、数组 [min,max]、对象 {min_inclusive,max_inclusive}。
/// minBound/maxBound 为合法上下界（含），违反则返回错误。
Result<state::InclusiveRange> parseInclusiveRange(const nlohmann::json& json, i32 minBound, i32 maxBound)
{
    if (json.is_number_integer()) {
        const i32 v = json.get<i32>();
        if (v < minBound || v > maxBound) {
            return Error(ErrorCode::InvalidData,
                "InclusiveRange value " + std::to_string(v) + " out of range [" + std::to_string(minBound) + "," +
                    std::to_string(maxBound) + "]");
        }
        return state::InclusiveRange{v, v};
    }
    if (json.is_array()) {
        if (json.size() != 2 || !json[0].is_number_integer() || !json[1].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "InclusiveRange array must be [min,max] integers");
        }
        const i32 lo = json[0].get<i32>();
        const i32 hi = json[1].get<i32>();
        if (lo > hi || lo < minBound || hi > maxBound) {
            return Error(ErrorCode::InvalidData,
                "InclusiveRange [" + std::to_string(lo) + "," + std::to_string(hi) +
                    "] invalid (require minBound<=min<=max<=maxBound)");
        }
        return state::InclusiveRange{lo, hi};
    }
    if (json.is_object()) {
        if (!json.contains("min_inclusive") || !json.contains("max_inclusive") ||
            !json["min_inclusive"].is_number_integer() || !json["max_inclusive"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "InclusiveRange object missing 'min_inclusive'/'max_inclusive'");
        }
        const i32 lo = json["min_inclusive"].get<i32>();
        const i32 hi = json["max_inclusive"].get<i32>();
        if (lo > hi || lo < minBound || hi > maxBound) {
            return Error(ErrorCode::InvalidData,
                "InclusiveRange {min=" + std::to_string(lo) + ",max=" + std::to_string(hi) +
                    "} invalid (require minBound<=min<=max<=maxBound)");
        }
        return state::InclusiveRange{lo, hi};
    }
    return Error(
        ErrorCode::InvalidData, "InclusiveRange must be int, [min,max] array, or {min_inclusive,max_inclusive}");
}

} // namespace

Result<std::unique_ptr<state::BlockStateProvider>> parse(const nlohmann::json& providerObj)
{
    if (!providerObj.is_object() || !providerObj.contains("type") || !providerObj["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "block state provider JSON missing 'type' string");
    }

    const std::string type = stripNamespace(providerObj["type"].get<std::string>());

    if (type == "simple_state_provider") {
        if (!providerObj.contains("state")) {
            return Error(ErrorCode::InvalidData, "simple_state_provider missing 'state' field");
        }
        auto stateResult = BlockStateParser::parse(providerObj["state"]);
        if (!stateResult.success()) {
            return stateResult.error();
        }
        return Result<std::unique_ptr<state::BlockStateProvider>>(
            std::make_unique<state::SimpleBlockStateProvider>(stateResult.value()));
    }

    if (type == "weighted_state_provider") {
        if (!providerObj.contains("entries") || !providerObj["entries"].is_array()) {
            return Error(ErrorCode::InvalidData, "weighted_state_provider missing 'entries' array");
        }
        auto weighted = std::make_unique<state::WeightedBlockStateProvider>();
        for (size_t i = 0; i < providerObj["entries"].size(); ++i) {
            const auto& entry = providerObj["entries"][i];
            if (!entry.contains("data")) {
                return Error(
                    ErrorCode::InvalidData, "weighted_state_provider entry[" + std::to_string(i) + "] missing 'data'");
            }
            auto entryState = BlockStateParser::parse(entry["data"]);
            if (!entryState.success()) {
                return entryState.error();
            }
            if (!entry.contains("weight") || !entry["weight"].is_number_integer()) {
                return Error(ErrorCode::InvalidData,
                    "weighted_state_provider entry[" + std::to_string(i) + "] missing 'weight'");
            }
            weighted->add(entryState.value(), entry["weight"].get<i32>());
        }
        return Result<std::unique_ptr<state::BlockStateProvider>>(std::move(weighted));
    }

    if (type == "rule_based_state_provider") {
        return parseRuleBasedData(providerObj);
    }

    if (type == "rotated_block_provider") {
        if (!providerObj.contains("state")) {
            return Error(ErrorCode::InvalidData, "rotated_block_provider missing 'state' field");
        }
        auto stateResult = BlockStateParser::parse(providerObj["state"]);
        if (!stateResult.success()) {
            return stateResult.error();
        }
        // MC: xmapped via getBlock → 仅保留 Block，丢弃 Properties。
        return Result<std::unique_ptr<state::BlockStateProvider>>(
            std::make_unique<state::RotatedBlockStateProvider>(&stateResult.value()->getBlock()));
    }

    if (type == "noise_threshold_provider") {
        auto seedResult = readSeed(providerObj);
        if (!seedResult.success()) {
            return seedResult.error();
        }
        if (!providerObj.contains("noise")) {
            return Error(ErrorCode::InvalidData, "noise_threshold_provider missing 'noise' parameters");
        }
        auto paramsResult = parseNoiseParameters(providerObj["noise"]);
        if (!paramsResult.success()) {
            return paramsResult.error();
        }
        if (!providerObj.contains("default_state")) {
            return Error(ErrorCode::InvalidData, "noise_threshold_provider missing 'default_state'");
        }
        auto defaultResult = BlockStateParser::parse(providerObj["default_state"]);
        if (!defaultResult.success()) {
            return defaultResult.error();
        }
        if (!providerObj.contains("low_states") || !providerObj.contains("high_states")) {
            return Error(ErrorCode::InvalidData, "noise_threshold_provider missing 'low_states'/'high_states'");
        }
        auto lowResult = parseStateList(providerObj["low_states"]);
        if (!lowResult.success()) {
            return lowResult.error();
        }
        auto highResult = parseStateList(providerObj["high_states"]);
        if (!highResult.success()) {
            return highResult.error();
        }
        const u64 seed = seedResult.value();
        const f32 scale = readPositiveFloat(providerObj, "scale", 1.0f);
        auto noise = createNoise(seed, paramsResult.value());
        const f32 threshold = getFloat(providerObj, "threshold", 0.0f);
        const f32 highChance = getFloat(providerObj, "high_chance", 0.0f);
        return Result<std::unique_ptr<state::BlockStateProvider>>(
            std::make_unique<state::NoiseThresholdBlockStateProvider>(seed,
                scale,
                std::move(noise),
                threshold,
                highChance,
                defaultResult.value(),
                std::move(lowResult.value()),
                std::move(highResult.value())));
    }

    if (type == "noise_provider") {
        auto seedResult = readSeed(providerObj);
        if (!seedResult.success()) {
            return seedResult.error();
        }
        if (!providerObj.contains("noise")) {
            return Error(ErrorCode::InvalidData, "noise_provider missing 'noise' parameters");
        }
        auto paramsResult = parseNoiseParameters(providerObj["noise"]);
        if (!paramsResult.success()) {
            return paramsResult.error();
        }
        if (!providerObj.contains("states")) {
            return Error(ErrorCode::InvalidData, "noise_provider missing 'states' list");
        }
        auto statesResult = parseStateList(providerObj["states"]);
        if (!statesResult.success()) {
            return statesResult.error();
        }
        const u64 seed = seedResult.value();
        const f32 scale = readPositiveFloat(providerObj, "scale", 1.0f);
        auto noise = createNoise(seed, paramsResult.value());
        return Result<std::unique_ptr<state::BlockStateProvider>>(std::make_unique<state::NoiseBlockStateProvider>(
            seed, scale, std::move(noise), std::move(statesResult.value())));
    }

    if (type == "dual_noise_provider") {
        if (!providerObj.contains("variety")) {
            return Error(ErrorCode::InvalidData, "dual_noise_provider missing 'variety' InclusiveRange");
        }
        auto varietyResult = parseInclusiveRange(providerObj["variety"], 1, 64);
        if (!varietyResult.success()) {
            return varietyResult.error();
        }
        const state::InclusiveRange variety = varietyResult.value();
        if (!providerObj.contains("slow_noise")) {
            return Error(ErrorCode::InvalidData, "dual_noise_provider missing 'slow_noise' parameters");
        }
        auto slowParamsResult = parseNoiseParameters(providerObj["slow_noise"]);
        if (!slowParamsResult.success()) {
            return slowParamsResult.error();
        }
        auto seedResult = readSeed(providerObj);
        if (!seedResult.success()) {
            return seedResult.error();
        }
        if (!providerObj.contains("noise")) {
            return Error(ErrorCode::InvalidData, "dual_noise_provider missing 'noise' parameters");
        }
        auto paramsResult = parseNoiseParameters(providerObj["noise"]);
        if (!paramsResult.success()) {
            return paramsResult.error();
        }
        if (!providerObj.contains("states")) {
            return Error(ErrorCode::InvalidData, "dual_noise_provider missing 'states' list");
        }
        auto statesResult = parseStateList(providerObj["states"]);
        if (!statesResult.success()) {
            return statesResult.error();
        }
        const u64 seed = seedResult.value();
        const f32 scale = readPositiveFloat(providerObj, "scale", 1.0f);
        auto noise = createNoise(seed, paramsResult.value());
        const f32 slowScale = readPositiveFloat(providerObj, "slow_scale", 1.0f);
        // MC: slowNoise 用同一 seed 构造。
        auto slowNoise = createNoise(seed, slowParamsResult.value());
        return Result<std::unique_ptr<state::BlockStateProvider>>(std::make_unique<state::DualNoiseBlockStateProvider>(
            seed, scale, std::move(noise), variety, slowScale, std::move(slowNoise), std::move(statesResult.value())));
    }

    if (type == "randomized_int_state_provider") {
        if (!providerObj.contains("source")) {
            return Error(ErrorCode::InvalidData, "randomized_int_state_provider missing 'source' provider");
        }
        auto sourceResult = parse(providerObj["source"]);
        if (!sourceResult.success()) {
            return sourceResult.error();
        }
        if (!providerObj.contains("property") || !providerObj["property"].is_string()) {
            return Error(ErrorCode::InvalidData, "randomized_int_state_provider missing 'property' string");
        }
        if (!providerObj.contains("values")) {
            return Error(ErrorCode::InvalidData, "randomized_int_state_provider missing 'values' IntProvider");
        }
        auto valuesResult = valueprovider::IntProviderParser::parse(providerObj["values"]);
        if (!valuesResult.success()) {
            return valuesResult.error();
        }
        return Result<std::unique_ptr<state::BlockStateProvider>>(
            std::make_unique<state::RandomizedIntBlockStateProvider>(
                sourceResult.value(), providerObj["property"].get<std::string>(), valuesResult.value()));
    }

    return Error(ErrorCode::NotFound, "unsupported block state provider type '" + type + "'");
}

Result<std::unique_ptr<state::BlockStateProvider>> parseRuleBased(const nlohmann::json& providerObj)
{
    if (!providerObj.is_object()) {
        return Error(ErrorCode::InvalidData, "rule_based_state_provider JSON must be an object");
    }
    return parseRuleBasedData(providerObj);
}

} // namespace BlockStateProviderParser
} // namespace mc::world::gen::feature::parser
