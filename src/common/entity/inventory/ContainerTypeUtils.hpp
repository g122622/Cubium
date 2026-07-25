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

#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"

namespace mc {

/// @brief 容器类型工具函数
///
/// 从旧 packet/ContainerPacketHandler 迁出。仅依赖 ContainerTypes.hpp 的枚举，
/// 与网络层解耦——ClickAction↔ClickType 映射是容器菜单的纯业务逻辑。
namespace ContainerTypes {

/// @brief 获取容器类型的槽位数
/// @param type 容器类型
/// @return 槽位数
[[nodiscard]] i32 getSlotCount(ContainerType type);

/// @brief 获取容器类型的标题
/// @param type 容器类型
/// @return 默认标题
[[nodiscard]] const char* getDefaultTitle(ContainerType type);

/// @brief 将容器类型转换为网络传输的 u8 值
/// @param type 容器类型
/// @return 网络值
[[nodiscard]] u8 toNetworkType(ContainerType type);

/// @brief 将网络点击动作转换为菜单点击类型
/// @param action 网络点击动作
/// @param button 鼠标按钮或快捷栏索引
/// @return 对应的菜单点击类型
[[nodiscard]] ClickType toClickType(ClickAction action, i32 button);

/// @brief 将菜单点击类型转换为网络点击动作
/// @param clickType 菜单点击类型
/// @return 对应的网络点击动作
[[nodiscard]] ClickAction toClickAction(ClickType clickType);

} // namespace ContainerTypes

} // namespace mc
