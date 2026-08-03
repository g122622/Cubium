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

#include "EnumProperty.hpp"
#include "common/util/Direction.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc {

/**
 * @brief 方向属性
 *
 * 专门用于表示方向的枚举属性，提供便捷的方向特定功能。
 *
 * 参考: net.minecraft.state.DirectionProperty
 *
 * 用法示例:
 * @code
 * // 创建包含所有方向的方向属性
 * auto facing = DirectionProperty::create("facing");
 *
 * // 创建只包含水平方向的方向属性
 * auto horizontalFacing = DirectionProperty::createHorizontal("facing");
 *
 * // 创建特定方向集合的属性
 * std::vector<Direction> dirs = {Direction::North, Direction::South};
 * auto facing = DirectionProperty::create("facing", dirs);
 * @endcode
 */
class DirectionProperty : public EnumProperty<Direction> {
public:
    /**
     * @brief 创建包含所有方向的方向属性
     * @param name 属性名称
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<DirectionProperty> create(const std::string& name)
    {
        auto all = Directions::all();
        return create(name, std::vector<Direction>(all.begin(), all.end()));
    }

    /**
     * @brief 创建包含特定方向的方向属性
     * @param name 属性名称
     * @param values 方向列表
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<DirectionProperty> create(
        const std::string& name, const std::vector<Direction>& values)
    {
        return std::unique_ptr<DirectionProperty>(new DirectionProperty(name, values));
    }

    /**
     * @brief 创建只包含水平方向的方向属性 (NORTH, EAST, SOUTH, WEST)
     * @param name 属性名称
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<DirectionProperty> createHorizontal(const std::string& name)
    {
        auto horiz = Directions::horizontal();
        return create(name, std::vector<Direction>(horiz.begin(), horiz.end()));
    }

    /**
     * @brief 使用过滤器创建方向属性
     * @param name 属性名称
     * @param filter 过滤函数
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<DirectionProperty> create(
        const std::string& name, std::function<bool(Direction)> filter)
    {
        std::vector<Direction> values;
        for (Direction dir : Directions::all()) {
            if (filter(dir)) {
                values.push_back(dir);
            }
        }
        return create(name, values);
    }

    /**
     * @brief 获取类型名称
     */
    [[nodiscard]] const char* typeName() const override { return "DirectionProperty"; }

private:
    DirectionProperty(const std::string& name, const std::vector<Direction>& values)
        : EnumProperty<Direction>(name, values)
    {}
};

/**
 * @brief Axis属性工具类
 *
 * 用于创建包含所有轴的属性。
 */
class AxisProperty {
public:
    /**
     * @brief 创建包含所有轴的属性
     * @param name 属性名称
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<EnumProperty<Axis>> create(const std::string& name)
    {
        auto all = Axes::all();
        return EnumProperty<Axis>::create(name, std::vector<Axis>(all.begin(), all.end()));
    }
};

} // namespace mc
