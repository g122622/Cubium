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
#include "common/util/Direction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/RotatedPillarBlock.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <string>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {

const BlockState* BlockStateProviderHandle::asSingle() const noexcept
{
    // 仅 Simple 情形可无随机源取单一状态；Weighted/RuleBased 调用方须自行采样。
    return kind == Kind::Simple ? simple : nullptr;
}

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
 * @brief 解析 {fallback, rules} 结构为 RuleBased 句柄（MC 1.21.11 RuleBasedBlockStateProvider）。
 *        parse(type=rule_based_state_provider) 与 parseRuleBased(无 type) 共用此逻辑。
 */
Result<BlockStateProviderHandle> parseRuleBasedData(const nlohmann::json& providerObj)
{
    if (!providerObj.contains("fallback") || !providerObj["fallback"].is_object()) {
        return Error(ErrorCode::InvalidData, "rule_based_state_provider missing 'fallback' object");
    }
    auto fallbackResult = parse(providerObj["fallback"]);
    if (!fallbackResult.success()) {
        return fallbackResult.error();
    }
    auto data = std::make_unique<BlockStateProviderHandle::RuleBasedData>();
    data->fallback = std::make_unique<BlockStateProviderHandle>(std::move(fallbackResult.value()));

    if (!providerObj.contains("rules") || !providerObj["rules"].is_array()) {
        return Error(ErrorCode::InvalidData, "rule_based_state_provider missing 'rules' array");
    }
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
        BlockStateProviderHandle::Rule rule;
        rule.ifTrue = predResult.value();
        rule.then = std::make_unique<BlockStateProviderHandle>(std::move(thenResult.value()));
        data->rules.push_back(std::move(rule));
    }

    BlockStateProviderHandle handle;
    handle.kind = BlockStateProviderHandle::Kind::RuleBased;
    handle.ruleBased = std::move(data);
    return handle;
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

/// 由 seed + NoiseParameters 构造 NormalNoise（对齐 MC NormalNoise.create(WorldgenRandom(seed), params)）。
std::unique_ptr<world::gen::noise::NormalNoise> createNoise(
    u64 seed, const world::gen::noise::NormalNoise::NoiseParameters& params)
{
    return std::make_unique<world::gen::noise::NormalNoise>(seed, params.firstOctave, params.amplitudes);
}

/// MC ExtraCodecs.intervalCodec 的 InclusiveRange<Integer> 三形式解析：
/// 裸整数 N（min=max=N）、数组 [min,max]、对象 {min_inclusive,max_inclusive}。
/// minBound/maxBound 为合法上下界（含），违反则返回错误。
Result<InclusiveRange> parseInclusiveRange(const nlohmann::json& json, i32 minBound, i32 maxBound)
{
    if (json.is_number_integer()) {
        const i32 v = json.get<i32>();
        if (v < minBound || v > maxBound) {
            return Error(ErrorCode::InvalidData,
                "InclusiveRange value " + std::to_string(v) + " out of range [" + std::to_string(minBound) + "," +
                    std::to_string(maxBound) + "]");
        }
        return InclusiveRange{v, v};
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
        return InclusiveRange{lo, hi};
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
        return InclusiveRange{lo, hi};
    }
    return Error(ErrorCode::InvalidData, "InclusiveRange must be int, [min,max] array, or {min_inclusive,max_inclusive}");
}

} // namespace

Result<BlockStateProviderHandle> parse(const nlohmann::json& providerObj)
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
        BlockStateProviderHandle handle;
        handle.kind = BlockStateProviderHandle::Kind::Simple;
        handle.simple = stateResult.value();
        return handle;
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
        BlockStateProviderHandle handle;
        handle.kind = BlockStateProviderHandle::Kind::Weighted;
        handle.weighted = std::move(weighted);
        return handle;
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
        BlockStateProviderHandle handle;
        handle.kind = BlockStateProviderHandle::Kind::Rotated;
        handle.rotatedBlock = &stateResult.value()->getBlock();
        return handle;
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
        auto data = std::make_unique<BlockStateProviderHandle::NoiseData>();
        data->seed = seedResult.value();
        data->scale = readPositiveFloat(providerObj, "scale", 1.0f);
        data->noise = createNoise(data->seed, paramsResult.value());
        data->threshold = getFloat(providerObj, "threshold", 0.0f);
        data->highChance = getFloat(providerObj, "high_chance", 0.0f);
        data->defaultState = defaultResult.value();
        data->lowStates = std::move(lowResult.value());
        data->highStates = std::move(highResult.value());
        BlockStateProviderHandle handle;
        handle.kind = BlockStateProviderHandle::Kind::NoiseThreshold;
        handle.noiseData = std::move(data);
        return handle;
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
        auto data = std::make_unique<BlockStateProviderHandle::NoiseData>();
        data->seed = seedResult.value();
        data->scale = readPositiveFloat(providerObj, "scale", 1.0f);
        data->noise = createNoise(data->seed, paramsResult.value());
        data->states = std::move(statesResult.value());
        BlockStateProviderHandle handle;
        handle.kind = BlockStateProviderHandle::Kind::Noise;
        handle.noiseData = std::move(data);
        return handle;
    }

    if (type == "dual_noise_provider") {
        if (!providerObj.contains("variety")) {
            return Error(ErrorCode::InvalidData, "dual_noise_provider missing 'variety' InclusiveRange");
        }
        auto varietyResult = parseInclusiveRange(providerObj["variety"], 1, 64);
        if (!varietyResult.success()) {
            return varietyResult.error();
        }
        const InclusiveRange variety = varietyResult.value();
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
        auto data = std::make_unique<BlockStateProviderHandle::NoiseData>();
        data->seed = seedResult.value();
        data->scale = readPositiveFloat(providerObj, "scale", 1.0f);
        data->noise = createNoise(data->seed, paramsResult.value());
        data->variety = variety;
        data->slowScale = readPositiveFloat(providerObj, "slow_scale", 1.0f);
        // MC: slowNoise 用同一 seed 构造。
        data->slowNoise = createNoise(data->seed, slowParamsResult.value());
        data->states = std::move(statesResult.value());
        BlockStateProviderHandle handle;
        handle.kind = BlockStateProviderHandle::Kind::DualNoise;
        handle.noiseData = std::move(data);
        return handle;
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
        auto data = std::make_unique<BlockStateProviderHandle::RandomizedIntData>();
        data->source = std::make_unique<BlockStateProviderHandle>(std::move(sourceResult.value()));
        data->propertyName = providerObj["property"].get<std::string>();
        data->values = valuesResult.value();
        BlockStateProviderHandle handle;
        handle.kind = BlockStateProviderHandle::Kind::RandomizedInt;
        handle.randomizedInt = std::move(data);
        return handle;
    }

    return Error(ErrorCode::NotFound, "unsupported block state provider type '" + type + "'");
}

Result<BlockStateProviderHandle> parseRuleBased(const nlohmann::json& providerObj)
{
    if (!providerObj.is_object()) {
        return Error(ErrorCode::InvalidData, "rule_based_state_provider JSON must be an object");
    }
    return parseRuleBasedData(providerObj);
}

namespace {

/// MC: NoiseBasedStateProvider.getNoiseValue(pos, scale) = noise.getValue(pos*scale, ...)
f64 getNoiseValue(const BlockStateProviderHandle::NoiseData& data, const BlockPos& pos)
{
    const f64 s = static_cast<f64>(data.scale);
    return data.noise->getValue(static_cast<f64>(pos.x) * s, static_cast<f64>(pos.y) * s, static_cast<f64>(pos.z) * s);
}

/// MC: NoiseProvider.getRandomState(list, noiseValue)：clamp((1+d0)/2, 0, 0.9999) * size。
const BlockState* getRandomStateByNoise(const std::vector<const BlockState*>& states, f64 noiseValue)
{
    const f64 d0 = math::clamp((1.0 + noiseValue) / 2.0, 0.0, 0.9999);
    const size_t idx = static_cast<size_t>(d0 * static_cast<f64>(states.size()));
    return states[idx < states.size() ? idx : states.size() - 1];
}

/// MC: NoiseProvider.getRandomState(list, pos, scale)。
const BlockState* getRandomState(const BlockStateProviderHandle::NoiseData& data, const BlockPos& pos)
{
    return getRandomStateByNoise(data.states, getNoiseValue(data, pos));
}

/// MC: NoiseThresholdProvider.getState。
const BlockState* sampleNoiseThreshold(
    const BlockStateProviderHandle& handle, math::IRandom& random, const BlockPos& pos)
{
    if (handle.noiseData == nullptr) {
        return nullptr;
    }
    const auto& data = *handle.noiseData;
    const f64 d0 = getNoiseValue(data, pos);
    if (d0 < static_cast<f64>(data.threshold)) {
        return data.lowStates[static_cast<size_t>(random.nextInt(static_cast<i32>(data.lowStates.size())))];
    }
    if (random.nextFloat() < data.highChance) {
        return data.highStates[static_cast<size_t>(random.nextInt(static_cast<i32>(data.highStates.size())))];
    }
    return data.defaultState;
}

/// MC: NoiseProvider.getState = getRandomState(states, pos, scale)。
const BlockState* sampleNoiseProvider(
    const BlockStateProviderHandle& handle, math::IRandom& /*random*/, const BlockPos& pos)
{
    if (handle.noiseData == nullptr || handle.noiseData->states.empty()) {
        return nullptr;
    }
    return getRandomState(*handle.noiseData, pos);
}

/// MC: DualNoiseProvider.getState。
const BlockState* sampleDualNoiseProvider(
    const BlockStateProviderHandle& handle, math::IRandom& /*random*/, const BlockPos& pos)
{
    if (handle.noiseData == nullptr || handle.noiseData->slowNoise == nullptr || handle.noiseData->states.empty()) {
        return nullptr;
    }
    const auto& data = *handle.noiseData;
    // MC: d0 = getSlowNoiseValue(pos); i = clampedMap(d0, -1, 1, variety.min, variety.max+1)
    const f64 slowScale = static_cast<f64>(data.slowScale);
    const f64 d0 = data.slowNoise->getValue(
        static_cast<f64>(pos.x) * slowScale, static_cast<f64>(pos.y) * slowScale, static_cast<f64>(pos.z) * slowScale);
    const i32 i = static_cast<i32>(math::clampedMap(
        d0, -1.0, 1.0, static_cast<f64>(data.variety.minInclusive), static_cast<f64>(data.variety.maxInclusive + 1)));
    // MC: 构造 i 个状态的临时列表，每个用 getSlowNoiseValue(pos.offset(j*54545,0,j*34234)) 选
    std::vector<const BlockState*> list;
    list.reserve(static_cast<size_t>(i > 0 ? i : 0));
    for (i32 j = 0; j < i; ++j) {
        const BlockPos offsetPos(pos.x + j * 54545, pos.y, pos.z + j * 34234);
        const f64 nv = data.slowNoise->getValue(static_cast<f64>(offsetPos.x) * slowScale,
            static_cast<f64>(offsetPos.y) * slowScale,
            static_cast<f64>(offsetPos.z) * slowScale);
        list.push_back(getRandomStateByNoise(data.states, nv));
    }
    if (list.empty()) {
        return data.states[0];
    }
    // MC: return getRandomState(list, pos, scale)（用快噪声索引）
    return getRandomStateByNoise(list, getNoiseValue(data, pos));
}

/// MC: RandomizedIntStateProvider.findProperty —— 按名查找 IntegerProperty。
const IntegerProperty* findIntegerProperty(const BlockState& state, const std::string& name)
{
    const IProperty* prop = state.getBlock().stateContainer().getProperty(name);
    return prop != nullptr ? dynamic_cast<const IntegerProperty*>(prop) : nullptr;
}

/// MC: RandomizedIntStateProvider.getState。
const BlockState* sampleRandomizedInt(
    const BlockStateProviderHandle& handle, const IWorld& world, math::IRandom& random, const BlockPos& pos)
{
    if (handle.randomizedInt == nullptr || handle.randomizedInt->source == nullptr) {
        return nullptr;
    }
    const auto& data = *handle.randomizedInt;
    const BlockState* blockstate = sampleState(*data.source, world, random, pos);
    if (blockstate == nullptr) {
        return nullptr;
    }
    // MC: if (property == null || !blockstate.hasProperty(property)) 重新解析并缓存
    if (data.property == nullptr || !blockstate->hasProperty(*data.property)) {
        const IntegerProperty* found = findIntegerProperty(*blockstate, data.propertyName);
        if (found == nullptr) {
            return blockstate; // 属性不存在 → 原样返回
        }
        data.property = found;
    }
    const i32 value = data.values->sample(random);
    return &blockstate->with(*data.property, value);
}

} // namespace

const BlockState* sampleState(
    const BlockStateProviderHandle& handle, const IWorld& world, math::IRandom& random, const BlockPos& pos)
{
    if (handle.kind == BlockStateProviderHandle::Kind::Simple) {
        return handle.simple;
    }
    if (handle.kind == BlockStateProviderHandle::Kind::Weighted) {
        return (handle.weighted != nullptr) ? handle.weighted->getState(random) : nullptr;
    }
    // RuleBased：按 rules 顺序找第一个命中的谓词，取其 then；否则 fallback。
    if (handle.kind == BlockStateProviderHandle::Kind::RuleBased) {
        if (handle.ruleBased != nullptr) {
            for (const auto& rule : handle.ruleBased->rules) {
                if (rule.ifTrue != nullptr && rule.ifTrue->test(world, pos)) {
                    return (rule.then != nullptr) ? sampleState(*rule.then, world, random, pos) : nullptr;
                }
            }
            if (handle.ruleBased->fallback != nullptr) {
                return sampleState(*handle.ruleBased->fallback, world, random, pos);
            }
        }
        return nullptr;
    }
    // Rotated：defaultState + 随机 axis（trySetValue 语义：无 AXIS 属性则原样返回）。
    if (handle.kind == BlockStateProviderHandle::Kind::Rotated) {
        if (handle.rotatedBlock == nullptr) {
            return nullptr;
        }
        const BlockState& def = handle.rotatedBlock->defaultState();
        // MC: Direction.Axis.getRandom(random) = VALUES[nextInt(3)]
        const Axis axis = Axes::all()[static_cast<size_t>(random.nextInt(3))];
        if (def.hasProperty(RotatedPillarBlock::AXIS())) {
            return &def.with(RotatedPillarBlock::AXIS(), axis);
        }
        return &def;
    }
    // Noise 驱动提供者共用 NoiseData。
    if (handle.kind == BlockStateProviderHandle::Kind::NoiseThreshold) {
        return sampleNoiseThreshold(handle, random, pos);
    }
    if (handle.kind == BlockStateProviderHandle::Kind::Noise) {
        return sampleNoiseProvider(handle, random, pos);
    }
    if (handle.kind == BlockStateProviderHandle::Kind::DualNoise) {
        return sampleDualNoiseProvider(handle, random, pos);
    }
    if (handle.kind == BlockStateProviderHandle::Kind::RandomizedInt) {
        return sampleRandomizedInt(handle, world, random, pos);
    }
    return nullptr;
}

} // namespace BlockStateProviderParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
