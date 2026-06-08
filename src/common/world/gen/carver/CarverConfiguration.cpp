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

#include "CarverConfiguration.hpp"
#include "common/world/block/BlockTags.hpp"

namespace mc {
namespace world::gen::carver {

namespace ConfiguredCarvers {

CaveCarverConfiguration createOverworldCaveConfig(const BlockTag* replaceable)
{
    return CaveCarverConfiguration(0.15f,
        valueprovider::UniformHeight::create(
            surface::VerticalAnchor::aboveBottom(8), surface::VerticalAnchor::absolute(180)),
        valueprovider::UniformFloat::create(0.1f, 0.9f),
        surface::VerticalAnchor::aboveBottom(8),
        replaceable,
        valueprovider::UniformFloat::create(0.7f, 1.4f),
        valueprovider::UniformFloat::create(0.8f, 1.3f),
        valueprovider::UniformFloat::create(-1.0f, -0.4f));
}

CaveCarverConfiguration createOverworldCaveExtraConfig(const BlockTag* replaceable)
{
    return CaveCarverConfiguration(0.07f,
        valueprovider::UniformHeight::create(
            surface::VerticalAnchor::aboveBottom(8), surface::VerticalAnchor::absolute(47)),
        valueprovider::UniformFloat::create(0.1f, 0.9f),
        surface::VerticalAnchor::aboveBottom(8),
        replaceable,
        valueprovider::UniformFloat::create(0.7f, 1.4f),
        valueprovider::UniformFloat::create(0.8f, 1.3f),
        valueprovider::UniformFloat::create(-1.0f, -0.4f));
}

CanyonCarverConfiguration createOverworldCanyonConfig(const BlockTag* replaceable)
{
    return CanyonCarverConfiguration(0.01f,
        valueprovider::UniformHeight::create(
            surface::VerticalAnchor::absolute(10), surface::VerticalAnchor::absolute(67)),
        valueprovider::ConstantFloat::create(3.0f),
        surface::VerticalAnchor::aboveBottom(8),
        replaceable,
        valueprovider::UniformFloat::create(-0.125f, 0.125f),
        CanyonShapeConfiguration(valueprovider::UniformFloat::create(0.75f, 1.0f),
            valueprovider::TrapezoidFloat::create(0.0f, 6.0f, 2.0f),
            3,
            valueprovider::UniformFloat::create(0.75f, 1.0f),
            1.0f,
            0.0f));
}

CaveCarverConfiguration createNetherCaveConfig(const BlockTag* replaceable)
{
    return CaveCarverConfiguration(0.2f,
        valueprovider::UniformHeight::create(
            surface::VerticalAnchor::absolute(0), surface::VerticalAnchor::belowTop(1)),
        valueprovider::ConstantFloat::create(0.5f),
        surface::VerticalAnchor::aboveBottom(10),
        replaceable,
        valueprovider::ConstantFloat::create(1.0f),
        valueprovider::ConstantFloat::create(1.0f),
        valueprovider::ConstantFloat::create(-0.7f));
}

} // namespace ConfiguredCarvers

} // namespace world::gen::carver
} // namespace mc
