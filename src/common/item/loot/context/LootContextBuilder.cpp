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

#include "LootContextBuilder.hpp"
#include "LootContext.hpp"
#include "LootParams.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootParameterSet.hpp"
#include "common/util/math/random/Random.hpp"
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace loot {

// ============================================================================
// LootContextBuilder
// ============================================================================

LootContextBuilder::LootContextBuilder(IWorld& world)
    : m_world(world)
{}

LootContextBuilder& LootContextBuilder::withRandom(math::Random& random)
{
    m_random = &random;
    return *this;
}

LootContextBuilder& LootContextBuilder::withSeed(u64 seed)
{
    m_seed = seed;
    m_hasSeed = true;
    return *this;
}

LootContextBuilder& LootContextBuilder::withLuck(f32 luck)
{
    m_luck = luck;
    return *this;
}

LootContextBuilder& LootContextBuilder::withLootingModifier(i32 level)
{
    m_lootingModifier = level;
    return *this;
}

std::unique_ptr<LootContext> LootContextBuilder::build(const LootParameterSet& paramSet)
{
    // 创建随机数生成器
    static thread_local math::Random defaultRandom(0);

    math::Random* randomToUse = m_random;
    if (randomToUse == nullptr) {
        if (m_hasSeed) {
            defaultRandom.setSeed(m_seed);
        } else {
            defaultRandom.setSeed(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
        }
        randomToUse = &defaultRandom;
    }

    auto context = std::make_unique<LootContext>(m_world, *randomToUse);

    // 设置幸运值
    context->setLuck(m_luck);
    context->setLootingModifier(m_lootingModifier);
    if (m_lootingModifier != 0) {
        context->setOwnedValue(LootParams::LOOTING_MODIFIER, m_lootingModifier);
    }

    // 复制参数
    for (const auto& [id, value] : m_params) {
        context->m_params[id] = value;
    }

    // 转移拥有所有权的值
    for (auto& ownedPtr : m_ownedValues) {
        context->m_ownedValues.push_back(std::move(ownedPtr));
    }

    // 设置掉落表解析器
    if (m_lootTableResolver) {
        context->setLootTableResolver(std::move(m_lootTableResolver));
    }

    // 设置谓词解析器
    if (m_predicateResolver) {
        context->setPredicateResolver(std::move(m_predicateResolver));
    }

    // 参数验证：检查必需参数和不允许的参数
    // Empty 和 Generic 类型的参数集没有必需参数也没有可选参数限制，跳过验证
    if (paramSet.getType() != LootParameterSet::Type::Empty && paramSet.getType() != LootParameterSet::Type::Generic) {
        std::vector<std::string> providedParamIds;
        providedParamIds.reserve(m_params.size());
        for (const auto& [id, _] : m_params) {
            providedParamIds.push_back(id);
        }

        std::vector<std::string> missingParams;
        std::vector<std::string> unexpectedParams;
        if (!paramSet.validate(providedParamIds, missingParams, unexpectedParams)) {
            if (!missingParams.empty()) {
                std::string missingStr;
                for (size_t i = 0; i < missingParams.size(); ++i) {
                    if (i > 0) missingStr += ", ";
                    missingStr += missingParams[i];
                }
                spdlog::warn("LootContextBuilder: missing required parameters [{}] (parameter set type: {})",
                    missingStr,
                    static_cast<int>(paramSet.getType()));
            }
            if (!unexpectedParams.empty()) {
                std::string unexpectedStr;
                for (size_t i = 0; i < unexpectedParams.size(); ++i) {
                    if (i > 0) unexpectedStr += ", ";
                    unexpectedStr += unexpectedParams[i];
                }
                spdlog::warn("LootContextBuilder: unexpected parameters [{}] (parameter set type: {})",
                    unexpectedStr,
                    static_cast<int>(paramSet.getType()));
            }
        }
    }

    return context;
}

} // namespace loot
} // namespace mc
