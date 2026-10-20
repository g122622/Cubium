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

#include "common/world/attribute/EnvironmentAttributeSystem.hpp"

namespace mc {
namespace world {
namespace attribute {

::mc::world::attribute::EnvironmentAttributeSystem EnvironmentAttributeSystemBuilder::build() const
{
    EnvironmentAttributeSystem system;

    for (const auto& [attribute, spec] : m_layers) {
        const std::vector<std::function<std::any(const std::any&, const _EvalContext&)>> layers = spec.layers;
        const auto resetTickCaches = std::make_shared<std::vector<std::function<void()>>>(spec.resetTickCaches);

        // 求值函数由 std::function 承载，各层返回值类型与属性值类型一致（由构建期约束保证），
        // 因此 std::bad_any_cast 只会在层与属性类型不匹配的编程错误时抛出；此处不吞掉该异常。
        EnvironmentAttributeSystem::AttributeEntry entry{// NOLINTNEXTLINE(bugprone-exception-escape)
            [layers](const std::any& base, const _EvalContext& context) -> std::any {
                std::any value = base;
                for (const auto& layer : layers) {
                    value = layer(value, context);
                }
                return value;
            },
            spec.affectedByPosition,
            std::any{},
            resetTickCaches};

        system.m_attributes.emplace(attribute, std::move(entry));
    }

    return system;
}

EnvironmentAttributeSystem::EnvironmentAttributeSystem(const EnvironmentAttributeSystem& other)
    : m_attributes(other.m_attributes)
    , m_tickId(other.m_tickId)
{}

EnvironmentAttributeSystem& EnvironmentAttributeSystem::operator=(const EnvironmentAttributeSystem& other)
{
    if (this == &other) {
        return *this;
    }

    EnvironmentAttributeSystem copy(other);
    m_attributes = std::move(copy.m_attributes);
    m_tickId = copy.m_tickId;
    return *this;
}

void EnvironmentAttributeSystem::invalidateTickCache() const
{
    ++m_tickId;
    for (const auto& [attribute, entry] : m_attributes) {
        entry.cachedTickValue.reset();
        // 层（时间线采样器等）以 tick 序号自行缓存采样结果，序号因刷新而自增，
        // 必须同步清空，否则层继续命中旧缓存（与 vanilla invalidateTickCache 一致）
        for (const auto& reset : *entry.resetTickCaches) {
            reset();
        }
    }
}

} // namespace attribute
} // namespace world
} // namespace mc
