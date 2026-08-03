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

#include "RandomizedIntBlockStateProvider.hpp"

#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/feature/state/NoiseStateUtils.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"

#include <memory>
#include <string>
#include <utility>

namespace mc::world::gen::feature::state {

RandomizedIntBlockStateProvider::RandomizedIntBlockStateProvider(std::unique_ptr<BlockStateProvider> source,
    std::string propertyName,
    std::unique_ptr<world::gen::valueprovider::IntProvider> values)
    : m_source(std::move(source))
    , m_propertyName(std::move(propertyName))
    , m_values(std::move(values))
{}

const BlockState* RandomizedIntBlockStateProvider::getState(
    const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const
{
    if (m_source == nullptr) {
        return nullptr;
    }
    const BlockState* blockstate = m_source->getState(world, random, x, y, z);
    if (blockstate == nullptr) {
        return nullptr;
    }
    // 缓存失效（方块可能无该属性）时重新解析：按名查找 IntegerProperty。
    if (m_property == nullptr || !blockstate->hasProperty(*m_property)) {
        const IntegerProperty* found = noise_state_utils::findIntegerProperty(*blockstate, m_propertyName);
        if (found == nullptr) {
            return blockstate; // 属性不存在 → 原样返回
        }
        m_property = found;
    }
    const i32 value = m_values->sample(random);
    return &blockstate->with(*m_property, value);
}

std::unique_ptr<BlockStateProvider> RandomizedIntBlockStateProvider::clone() const
{
    return std::make_unique<RandomizedIntBlockStateProvider>(
        m_source ? m_source->clone() : nullptr, m_propertyName, m_values ? m_values->clone() : nullptr);
}

} // namespace mc::world::gen::feature::state
