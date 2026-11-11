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

#include "common/entity/core/Entity.hpp"
#include <optional>
#include <string_view>

namespace mc::mod::bedrock::addon {
/** @brief 查询官方基础定义中的稳定原版家族；缺失或依赖动态组时返回空值。 */
[[nodiscard]] std::optional<bool> vanillaEntityFamilyMatch(std::string_view typeId, std::string_view family);

/**
 * @brief 判定实体是否属于某个基岩版家族
 *
 * 比 vanillaEntityFamilyMatch 多一层回退：先看 addon
 * 实体自己声明的 minecraft:type_family，再查原版家族表，
 * 最后处理 "player" / "mob" 两个通用家族。
 *
 * @param entity 待判定的实体
 * @param family 家族名
 * @return 是否属于该家族；无法判定时返回 false
 */
[[nodiscard]] bool entityHasFamily(const mc::Entity& entity, std::string_view family);
} // namespace mc::mod::bedrock::addon
