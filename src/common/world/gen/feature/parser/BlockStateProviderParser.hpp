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
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/feature/state/WeightedBlockStateProvider.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {

/**
 * @brief 方块状态提供者解析结果
 *
 * 项目的 SimpleBlockStateProvider 与 WeightedBlockStateProvider 不共享基类，
 * 故本结构用 kind 判别并分别持有其中一种。调用方按 kind 取用：
 * - kind==Simple：simple 持有固定 BlockState*
 * - kind==Weighted：weighted 持有加权提供者
 */
struct BlockStateProviderHandle {
    enum class Kind { Simple, Weighted };
    Kind kind = Kind::Simple;
    const BlockState* simple = nullptr;
    std::unique_ptr<state::WeightedBlockStateProvider> weighted;

    /**
     * @brief 取单一状态：Simple 直接返回 simple；Weighted 当仅一个条目时返回该状态，否则 nullptr。
     */
    [[nodiscard]] const BlockState* asSingle() const noexcept;
};

/**
 * @brief 方块状态提供者 JSON 解析器
 *
 * 解析数据包中的 BlockStateProvider 对象：
 *   {"type":"minecraft:simple_state_provider","state":{"Name":...,"Properties":{...}}}
 *   {"type":"minecraft:weighted_state_provider","entries":[{"data":{...},"weight":N}, ...]}
 *
 * 仅支持 simple_state_provider 与 weighted_state_provider（项目仅实现这两种）。
 * 其它 type（rotated_block_state_provider / noise_provider / randomized_int_state_provider 等）
 * 当前严格报错。
 */
namespace BlockStateProviderParser {

/**
 * @brief 解析方块状态提供者
 */
[[nodiscard]] Result<BlockStateProviderHandle> parse(const nlohmann::json& providerObj);

} // namespace BlockStateProviderParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
