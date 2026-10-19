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

#include "common/world/attribute/AttributeTypes.hpp"

#include "common/world/attribute/EnvironmentAttributeRegistry.hpp"

#include <map>
#include <string_view>

namespace mc {
namespace world {
namespace attribute {
namespace {

/// 浮点类修改器库（ADD / SUBTRACT / MULTIPLY / MINIMUM / MAXIMUM）
AttributeType<f32>::ModifierLibrary _makeFloatModifierLibrary()
{
    return {
        {AttributeModifierOperation::Add, &FloatModifier::ADD},
        {AttributeModifierOperation::Subtract, &FloatModifier::SUBTRACT},
        {AttributeModifierOperation::Multiply, &FloatModifier::MULTIPLY},
        {AttributeModifierOperation::Minimum, &FloatModifier::MINIMUM},
        {AttributeModifierOperation::Maximum, &FloatModifier::MAXIMUM},
    };
}

/// 布尔修改器库（AND / NAND / OR / NOR / XOR / XNOR）
AttributeType<bool>::ModifierLibrary _makeBooleanModifierLibrary()
{
    return {
        {AttributeModifierOperation::And, &BooleanModifier::AND},
        {AttributeModifierOperation::Nand, &BooleanModifier::NAND},
        {AttributeModifierOperation::Or, &BooleanModifier::OR},
        {AttributeModifierOperation::Nor, &BooleanModifier::NOR},
        {AttributeModifierOperation::Xor, &BooleanModifier::XOR},
        {AttributeModifierOperation::Xnor, &BooleanModifier::XNOR},
    };
}

/// 颜色修改器库（ALPHA_BLEND / ADD / SUBTRACT / MULTIPLY / BLEND_TO_GRAY）
AttributeType<i32>::ModifierLibrary _makeColorModifierLibrary()
{
    return {
        {AttributeModifierOperation::AlphaBlend, &RgbColorModifier::ALPHA_BLEND},
        {AttributeModifierOperation::Add, &RgbColorModifier::ADD},
        {AttributeModifierOperation::Subtract, &RgbColorModifier::SUBTRACT},
        {AttributeModifierOperation::Multiply, &RgbColorModifier::MULTIPLY},
        {AttributeModifierOperation::BlendToGray, nullptr},
    };
}

} // namespace

const AttributeType<bool>& AttributeTypes::BOOLEAN()
{
    static const AttributeType<bool> instance = AttributeType<bool>::ofNotInterpolated(_makeBooleanModifierLibrary());
    return instance;
}

const AttributeType<util::TriState>& AttributeTypes::TRI_STATE()
{
    static const AttributeType<util::TriState> instance = AttributeType<util::TriState>::ofNotInterpolated();
    return instance;
}

const AttributeType<f32>& AttributeTypes::FLOAT()
{
    static const AttributeType<f32> instance = AttributeType<f32>::ofInterpolated(
        LerpFunction<f32>::ofFloat(), LerpFunction<f32>::ofFloat(), _makeFloatModifierLibrary());
    return instance;
}

const AttributeType<f32>& AttributeTypes::ANGLE_DEGREES()
{
    static const AttributeType<f32> instance = AttributeType<f32>::ofInterpolated(
        LerpFunction<f32>::ofDegrees(90.0f), LerpFunction<f32>::ofFloat(), _makeFloatModifierLibrary());
    return instance;
}

const AttributeType<i32>& AttributeTypes::RGB_COLOR()
{
    static const AttributeType<i32> instance = AttributeType<i32>::ofInterpolated(
        LerpFunction<i32>::ofColor(), LerpFunction<i32>::ofColor(), _makeColorModifierLibrary());
    return instance;
}

const AttributeType<i32>& AttributeTypes::ARGB_COLOR()
{
    static const AttributeType<i32> instance = AttributeType<i32>::ofInterpolated(
        LerpFunction<i32>::ofColor(), LerpFunction<i32>::ofColor(), _makeColorModifierLibrary());
    return instance;
}

const AttributeType<MoonPhase>& AttributeTypes::MOON_PHASE()
{
    static const AttributeType<MoonPhase> instance = AttributeType<MoonPhase>::ofNotInterpolated();
    return instance;
}

const AttributeType<entity::ai::brain::schedule::Activity>& AttributeTypes::ACTIVITY()
{
    static const AttributeType<entity::ai::brain::schedule::Activity> instance =
        AttributeType<entity::ai::brain::schedule::Activity>::ofNotInterpolated();
    return instance;
}

Result<size_t> AttributeTypes::registerAll()
{
    size_t registered = 0;
    const auto registerOne = [&registered](std::string_view name, const auto& type) {
        const auto result = AttributeTypeRegistry::registerType(name, type);
        if (result.success()) {
            ++registered;
        }
    };

    registerOne("boolean", BOOLEAN());
    registerOne("tri_state", TRI_STATE());
    registerOne("float", FLOAT());
    registerOne("angle_degrees", ANGLE_DEGREES());
    registerOne("rgb_color", RGB_COLOR());
    registerOne("argb_color", ARGB_COLOR());
    registerOne("moon_phase", MOON_PHASE());
    registerOne("activity", ACTIVITY());
    return registered;
}

} // namespace attribute
} // namespace world
} // namespace mc
