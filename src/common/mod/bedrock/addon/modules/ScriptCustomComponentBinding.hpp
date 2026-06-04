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

#pragma once

#include "common/core/Types.hpp"

#include <string>

namespace mc::mod::bedrock::addon {

class NativeModuleBuilder;
class IScriptBindingContext;

/**
 * @brief 注册方块自定义组件到BlockComponentRegistry
 *
 * 从JS回调中解析组件对象，将每个回调函数注册到C++ BlockComponentRegistry。
 * JS API: blockComponentRegistry.registerCustomComponent(typeId, component)
 *
 * @param typeId 方块类型ID（如"minecraft:stone"）
 * @param componentObj JS组件对象句柄，包含onStepOn/onPlace等回调
 * @param ctx 绑定上下文
 * @return 是否注册成功
 */
bool registerBlockCustomComponentFromJS(const std::string& typeId, void* componentObj, IScriptBindingContext& ctx);

/**
 * @brief 注册物品自定义组件到ItemComponentRegistry
 *
 * 从JS回调中解析组件对象，将每个回调函数注册到C++ ItemComponentRegistry。
 * JS API: itemComponentRegistry.registerCustomComponent(typeId, component)
 *
 * @param typeId 物品类型ID（如"minecraft:diamond_sword"）
 * @param componentObj JS组件对象句柄，包含onUse/onHitEntity等回调
 * @param ctx 绑定上下文
 * @return 是否注册成功
 */
bool registerItemCustomComponentFromJS(const std::string& typeId, void* componentObj, IScriptBindingContext& ctx);

/**
 * @brief 向@minecraft/server模块注册自定义组件绑定
 *
 * 导出blockComponentRegistry和itemComponentRegistry全局对象到JS模块。
 *
 * @param builder 模块构建器
 */
void registerCustomComponentBindings(NativeModuleBuilder& builder);

} // namespace mc::mod::bedrock::addon
