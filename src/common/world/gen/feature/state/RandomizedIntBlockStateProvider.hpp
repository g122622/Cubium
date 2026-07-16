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

#include "common/util/property/IntegerProperty.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <string>

namespace mc::world::gen::feature::state {

/**
 * @brief 随机整数属性方块状态提供者
 *
 * 先由 source 提供者取一个方块状态，再按 property 名查找其 IntegerProperty，
 * 用 IntProvider 采样一个整数值设置到该属性上。属性查找结果懒缓存。
 */
class RandomizedIntBlockStateProvider : public BlockStateProvider {
public:
    RandomizedIntBlockStateProvider(std::unique_ptr<BlockStateProvider> source,
        std::string propertyName,
        std::unique_ptr<world::gen::valueprovider::IntProvider> values);

    [[nodiscard]] const BlockState* getState(
        const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const override;

    [[nodiscard]] std::unique_ptr<BlockStateProvider> clone() const override;

private:
    std::unique_ptr<BlockStateProvider> m_source;
    std::string m_propertyName;
    std::unique_ptr<world::gen::valueprovider::IntProvider> m_values;
    /// 懒解析缓存：首次采样时按 m_propertyName 查找并缓存，之后复用。
    mutable const IntegerProperty* m_property = nullptr;
};

} // namespace mc::world::gen::feature::state
