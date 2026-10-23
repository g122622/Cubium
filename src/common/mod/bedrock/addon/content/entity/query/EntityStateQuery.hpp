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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {
class Entity;
}

namespace mc::mod::bedrock::addon {

/** @brief 为过滤器和脚本组件统一读取实体当前有效状态。 */
class EntityStateQuery {
public:
    /** @brief 判断活动组件或原生实体对应的组件是否存在。 */
    [[nodiscard]] static Result<bool> hasComponent(const mc::Entity& entity, const std::string& name);
    /** @brief 读取变种组件；addon 缺少组件时值为零，未映射原生类型明确报错。 */
    [[nodiscard]] static Result<i32> variant(const mc::Entity& entity, const std::string& name);
    /** @brief 比较实际装备和背包内容；参数必须经过过滤器编译校验。 */
    [[nodiscard]] static Result<bool> hasEquipment(
        const mc::Entity& entity, const std::string& domain, const std::string& item);
    /** @brief 判断 subject 是否是 caller 记录的主人，不依赖主人当前所在维度。 */
    [[nodiscard]] static bool isOwner(const mc::Entity& caller, const mc::Entity& subject);
};

} // namespace mc::mod::bedrock::addon
