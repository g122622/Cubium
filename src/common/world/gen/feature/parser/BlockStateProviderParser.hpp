/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including without limitation the rights
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

#include "common/core/Result.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/feature/state/WeightedBlockStateProvider.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// 前向声明
class IWorld;
class Block;
class IntegerProperty;

namespace world {
namespace gen {
namespace feature {
namespace parser {

/// MC 1.21.11 InclusiveRange<Integer>（仅用于 DualNoiseProvider.variety，1..64）。
struct InclusiveRange {
    i32 minInclusive;
    i32 maxInclusive;
};

/**
 * @brief 方块状态提供者解析结果
 *
 * 项目的 SimpleBlockStateProvider 与 WeightedBlockStateProvider 不共享基类，
 * 故本结构用 kind 判别并分别持有其中一种。调用方按 kind 取用：
 * - kind==Simple：simple 持有固定 BlockState*
 * - kind==Weighted：weighted 持有加权提供者
 * - kind==RuleBased：ruleBased 持有 fallback + 规则列表（每条规则 = 谓词 + 子提供者）
 * - kind==Rotated：rotatedBlock 持有方块（每次取默认状态并随机轴）
 * - kind==NoiseThreshold/Noise/DualNoise：noiseData 持有噪声驱动配置
 * - kind==RandomizedInt：randomizedInt 持有子提供者 + 属性名 + IntProvider
 */
struct BlockStateProviderHandle {
    enum class Kind { Simple, Weighted, RuleBased, Rotated, NoiseThreshold, Noise, DualNoise, RandomizedInt };
    Kind kind = Kind::Simple;
    const BlockState* simple = nullptr;
    std::unique_ptr<state::WeightedBlockStateProvider> weighted;

    /// rule_based 的一条规则：if_true 谓词命中时取 then 提供者的状态。
    struct Rule {
        std::unique_ptr<predicate::BlockPredicate> ifTrue;
        std::unique_ptr<BlockStateProviderHandle> then;
    };
    struct RuleBasedData {
        std::unique_ptr<BlockStateProviderHandle> fallback;
        std::vector<Rule> rules;
    };
    std::unique_ptr<RuleBasedData> ruleBased;

    /// rotated_block_provider：仅持有方块（丢弃 JSON Properties）。
    const Block* rotatedBlock = nullptr;

    /// 噪声驱动提供者共用配置（NoiseThreshold/Noise/DualNoise）。
    struct NoiseData {
        u64 seed = 0;
        f32 scale = 1.0f;
        std::unique_ptr<world::gen::noise::NormalNoise> noise; ///< 快噪声（getNoiseValue）
        std::vector<const BlockState*> states;                 ///< Noise/DualNoise 的 states 列表
        // NoiseThreshold 专用
        f32 threshold = 0.0f;
        f32 highChance = 0.0f;
        const BlockState* defaultState = nullptr;
        std::vector<const BlockState*> lowStates;
        std::vector<const BlockState*> highStates;
        // DualNoise 专用
        InclusiveRange variety{1, 1};
        f32 slowScale = 1.0f;
        std::unique_ptr<world::gen::noise::NormalNoise> slowNoise; ///< 慢噪声
    };
    std::unique_ptr<NoiseData> noiseData;

    /// randomized_int_state_provider：source 提供者 + 属性名 + IntProvider。
    struct RandomizedIntData {
        std::unique_ptr<BlockStateProviderHandle> source;
        std::string propertyName;
        std::unique_ptr<valueprovider::IntProvider> values;
        mutable const IntegerProperty* property = nullptr; ///< 懒解析缓存
    };
    std::unique_ptr<RandomizedIntData> randomizedInt;

    /**
     * @brief 取单一状态：Simple 直接返回 simple；其余返回 nullptr（需随机源/世界）。
     */
    [[nodiscard]] const BlockState* asSingle() const noexcept;
};

/**
 * @brief 方块状态提供者 JSON 解析器
 *
 * 解析数据包中的 BlockStateProvider 对象：
 *   {"type":"minecraft:simple_state_provider","state":{"Name":...,"Properties":{...}}}
 *   {"type":"minecraft:weighted_state_provider","entries":[{"data":{...},"weight":N}, ...]}
 *   {"type":"minecraft:rule_based_state_provider",
 *    "fallback":<BlockStateProvider>, "rules":[{"if_true":<BlockPredicate>, "then":<BlockStateProvider>}, ...]}
 *   {"type":"minecraft:rotated_block_provider","state":{...}}（取 Block，随机 axis）
 *   {"type":"minecraft:noise_threshold_provider","seed":L,"noise":{...},"scale":F,
 *    "threshold":F,"high_chance":F,"default_state":{...},"low_states":[...],"high_states":[...]}
 *   {"type":"minecraft:noise_provider","seed":L,"noise":{...},"scale":F,"states":[...]}
 *   {"type":"minecraft:dual_noise_provider","variety":{"min_inclusive":1,"max_inclusive":64},
 *    "slow_noise":{...},"slow_scale":F,"seed":L,"noise":{...},"scale":F,"states":[...]}
 *   {"type":"minecraft:randomized_int_state_provider","source":{...},"property":"age",
 *    "values":{...IntProvider...}}
 *
 * 另有 parseRuleBased() 解析无 type 字段的 {fallback,rules}（MC 1.21.11
 * RuleBasedBlockStateProvider 为独立 record 类型，DiskConfiguration 直接持有）。
 */
namespace BlockStateProviderParser {

/**
 * @brief 解析方块状态提供者
 */
[[nodiscard]] Result<BlockStateProviderHandle> parse(const nlohmann::json& providerObj);

/**
 * @brief 解析 rule_based_state_provider（无 type 字段）
 *
 * 对应 MC 1.21.11 RuleBasedBlockStateProvider.CODEC：仅 {fallback, rules}，
 * 无 "type" 字段（与多态 BlockStateProvider.CODEC 的带 type 派发不同）。
 * DiskConfiguration.stateProvider 等直接持有此类型，JSON 不写 type。
 * 解析结果 kind 恒为 RuleBased。
 */
[[nodiscard]] Result<BlockStateProviderHandle> parseRuleBased(const nlohmann::json& providerObj);

/**
 * @brief 从提供者采样一个方块状态
 *
 * - Simple：返回 simple；
 * - Weighted：按权重采样（仅需随机源）；
 * - RuleBased：按 rules 顺序，第一个 if_true.test(world, pos) 命中的取其 then 采样，
 *   否则取 fallback 采样。对齐 MC RuleBasedBlockStateProvider.getState。
 * - Rotated：defaultState + 随机 axis（trySetValue 语义）。
 * - NoiseThreshold/Noise/DualNoise：基于 NormalNoise 采样，对齐 MC 算法。
 * - RandomizedInt：source.getState 后按属性名查找 IntegerProperty 并 setValue(IntProvider.sample)。
 *
 * @param handle 提供者句柄（非 null）
 * @param world 世界（用于 RuleBased 谓词测试）
 * @param random 随机源（用于 Weighted/噪声分支选择）
 * @param pos 测试位置（RuleBased 谓词在此处测试；噪声在此处采样）
 * @return 采样的方块状态；提供者无可用状态时返回 nullptr
 */
[[nodiscard]] const BlockState* sampleState(
    const BlockStateProviderHandle& handle, const IWorld& world, math::IRandom& random, const BlockPos& pos);

} // namespace BlockStateProviderParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
