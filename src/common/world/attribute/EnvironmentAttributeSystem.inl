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

#pragma once

// 本文件由 EnvironmentAttributeSystem.hpp 在类定义之后包含，依赖其中的模板声明。

namespace mc {
namespace world {
namespace attribute {

template <typename Value>
EnvironmentAttributeSystemBuilder& EnvironmentAttributeSystemBuilder::addConstantLayer(
    const EnvironmentAttribute<Value>& attribute, const EnvironmentAttributeMap& map)
{
    AttributeLayers& spec = m_layers[&attribute];
    spec.layers.push_back([&attribute, map](const std::any& input, const _EvalContext&) -> std::any {
        Value value = std::any_cast<const Value&>(input);
        map.applyModifier(&attribute, &value);
        return std::any(std::in_place_type<Value>, std::move(value));
    });
    return *this;
}

template <typename Value>
EnvironmentAttributeSystemBuilder& EnvironmentAttributeSystemBuilder::addPositionalLayer(
    const EnvironmentAttribute<Value>& attribute, std::function<Value(const Value&, const Vector3&)> apply)
{
    AttributeLayers& spec = m_layers[&attribute];
    spec.affectedByPosition = true;
    spec.layers.push_back([apply = std::move(apply)](const std::any& input, const _EvalContext& context) -> std::any {
        const Value& value = std::any_cast<const Value&>(input);
        // 非位置查询跳过空间计算（与 vanilla computeValueNotPositional 一致）
        if (!context.positional) {
            return std::any(std::in_place_type<Value>, value);
        }
        return std::any(std::in_place_type<Value>, apply(value, context.pos));
    });
    return *this;
}

template <typename Value, typename Layer>
    requires requires(Layer& layer) {
        { layer.applyConstant(std::declval<Value>()) } -> std::same_as<Value>;
    }
EnvironmentAttributeSystemBuilder& EnvironmentAttributeSystemBuilder::addLayer(
    const EnvironmentAttribute<Value>& attribute, std::shared_ptr<const Layer> layer)
{
    AttributeLayers& spec = m_layers[&attribute];
    spec.layers.push_back([layer](const std::any& input, const _EvalContext&) -> std::any {
        const Value& value = std::any_cast<const Value&>(input);
        return std::any(std::in_place_type<Value>, layer->applyConstant(value));
    });
    return *this;
}

template <typename Value, typename Layer>
    requires requires(Layer& layer) {
        { layer.applyTimeBased(std::declval<Value>(), i64{}) } -> std::same_as<Value>;
    }
EnvironmentAttributeSystemBuilder& EnvironmentAttributeSystemBuilder::addLayer(
    const EnvironmentAttribute<Value>& attribute, std::shared_ptr<const Layer> layer)
{
    AttributeLayers& spec = m_layers[&attribute];
    spec.layers.push_back([layer](const std::any& input, const _EvalContext& context) -> std::any {
        const Value& value = std::any_cast<const Value&>(input);
        // 负序号表示一次性求值：层的 tick 比上下文小 1，必然重新采样，且不回写层缓存
        const i64 layerTickId = context.tickId < 0 ? context.tickId : context.tickId - 1;
        return std::any(std::in_place_type<Value>, layer->applyTimeBased(value, layerTickId));
    });
    // 层自身的缓存以 tick 序号判定：系统序号递增（invalidateTickCache）后，
    // 必须显式刷新层缓存，下一次求值才会按新的 dayTime 重新采样
    spec.resetTickCaches.push_back([layer] { layer->invalidateTickCache(); });
    return *this;
}

template <typename Value, typename Layer>
    requires requires(Layer& layer) {
        { layer.applyPositional(std::declval<Value>(), std::declval<const Vector3&>()) } -> std::same_as<Value>;
    }
EnvironmentAttributeSystemBuilder& EnvironmentAttributeSystemBuilder::addLayer(
    const EnvironmentAttribute<Value>& attribute, std::shared_ptr<const Layer> layer)
{
    AttributeLayers& spec = m_layers[&attribute];
    spec.affectedByPosition = true;
    spec.layers.push_back([layer](const std::any& input, const _EvalContext& context) -> std::any {
        const Value& value = std::any_cast<const Value&>(input);
        // 非位置查询跳过空间计算（与 vanilla computeValueNotPositional 一致）
        if (!context.positional) {
            return std::any(std::in_place_type<Value>, value);
        }
        return std::any(std::in_place_type<Value>, layer->applyPositional(value, context.pos));
    });
    return *this;
}

template <typename Value, typename Argument>
EnvironmentAttributeSystemBuilder& EnvironmentAttributeSystemBuilder::addTimeBasedLayer(
    const EnvironmentAttribute<Value>& attribute,
    std::unique_ptr<timeline::AttributeTrackSampler<Value, Argument>> sampler)
{
    // 采样器作为 `EnvironmentAttributeLayer<Value>::TimeBased` 子接口持有：
    // 系统只通过 TimeBased 的虚调用使用它，并与 Argument 无关。
    return addLayer<Value>(attribute,
        std::shared_ptr<const typename EnvironmentAttributeLayer<Value>::TimeBased>(std::move(sampler)));
}

template <typename Value>
Value EnvironmentAttributeSystem::getDimensionValue(const EnvironmentAttribute<Value>& attribute) const
{
    const auto it = m_attributes.find(&attribute);
    if (it == m_attributes.end()) {
        return attribute.defaultValue();
    }

    const AttributeEntry& entry = it->second;
    if (entry.cachedTickValue.has_value()) {
        return std::any_cast<Value>(entry.cachedTickValue);
    }

    const EnvironmentAttributeSystemBuilder::_EvalContext context{false, m_tickId, Vector3::zero()};
    std::any value = entry.eval(std::any(std::in_place_type<Value>, attribute.defaultValue()), context);
    Value result = attribute.sanitizeValue(std::any_cast<Value>(std::move(value)));
    entry.cachedTickValue = result;
    return result;
}

template <typename Value>
Value EnvironmentAttributeSystem::getValue(const EnvironmentAttribute<Value>& attribute, const Vector3& pos) const
{
    const auto it = m_attributes.find(&attribute);
    if (it == m_attributes.end()) {
        return attribute.defaultValue();
    }

    const AttributeEntry& entry = it->second;
    if (!entry.affectedByPosition) {
        return getDimensionValue(attribute);
    }

    const EnvironmentAttributeSystemBuilder::_EvalContext context{true, m_tickId, pos};
    std::any value = entry.eval(std::any(std::in_place_type<Value>, attribute.defaultValue()), context);
    return attribute.sanitizeValue(std::any_cast<Value>(std::move(value)));
}

template <typename Value>
bool EnvironmentAttributeSystem::isAffectedByPosition(const EnvironmentAttribute<Value>& attribute) const
{
    const auto it = m_attributes.find(&attribute);
    return it != m_attributes.end() && it->second.affectedByPosition;
}

} // namespace attribute
} // namespace world
} // namespace mc
