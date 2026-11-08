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
 */

#include "common/world/block/registry/vanilla/BlockStatePredicates.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/state/BlockState.hpp"

namespace mc {
namespace block_registry {

namespace {

bool _never(const BlockState& state, const BlockPos& pos)
{
    MC_UNUSED(state);
    MC_UNUSED(pos);
    return false;
}

bool _always(const BlockState& state, const BlockPos& pos)
{
    MC_UNUSED(state);
    MC_UNUSED(pos);
    return true;
}

/// 原版 `Blocks.lambda$static$1`：活塞未伸出时闷人。
bool _notExtendedPiston(const BlockState& state, const BlockPos& pos)
{
    MC_UNUSED(pos);
    return !state.get(BlockStateProperties::EXTENDED());
}

/// 原版 `Blocks.lambda$static$447`：雪层满 8 层才挡视线。
bool _snowFullLayers(const BlockState& state, const BlockPos& pos)
{
    MC_UNUSED(pos);
    return state.get(BlockStateProperties::LAYERS_1_8()) >= 8;
}

} // namespace

const StatePredicate& BlockStatePredicates::never()
{
    static constexpr StatePredicate s_predicate{&_never, "never"};
    return s_predicate;
}

const StatePredicate& BlockStatePredicates::always()
{
    static constexpr StatePredicate s_predicate{&_always, "always"};
    return s_predicate;
}

const StatePredicate& BlockStatePredicates::notExtendedPiston()
{
    static constexpr StatePredicate s_predicate{&_notExtendedPiston, "not_extended_piston"};
    return s_predicate;
}

const StatePredicate& BlockStatePredicates::snowFullLayersViewBlocking()
{
    static constexpr StatePredicate s_predicate{&_snowFullLayers, "snow_full_layers_view_blocking"};
    return s_predicate;
}

const StatePredicate& BlockStatePredicates::alwaysBlocking()
{
    return always();
}

const StatePredicate& BlockStatePredicates::dirtPathBlocking()
{
    return always();
}

const StatePredicate& BlockStatePredicates::soulSandBlocking()
{
    return always();
}

const StatePredicate& BlockStatePredicates::mudBlocking()
{
    return always();
}

const StatePredicate& BlockStatePredicates::movingPistonNever()
{
    return never();
}

const StatePredicate& BlockStatePredicates::trialSpawnerNeverView()
{
    return never();
}

const StatePredicate& BlockStatePredicates::testInstanceNeverView()
{
    return never();
}

} // namespace block_registry
} // namespace mc
