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

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"

namespace mc::test {

/**
 * @brief 注册 @minecraft/server-gametest 的辅助类型绑定（批次5）。
 *
 * 对齐基岩官方 JS 文档，注册四个辅助类/枚举，供 Test.getSculkSpreader/getFenceConnectivity
 * （批次4）与 SimulatedPlayer.lookAtBlock（批次6）等使用：
 *
 * - `SculkSpreader`：opaque 持 `mc::blocks::SculkSpreader*`（owned）。属性 maxCharge（kMaxCharge=1000，
 *   做实）；方法 addCursors(pos,amount)/getNumberOfCursors()/getTotalCharge()/getCursorPosition(index)
 *   做实（从 cursors() 派生），addCursorsWithOffset stub（依赖未就绪偏移扩散体系）。
 * - `FenceConnectivity`：值对象（{north,east,south,west} 四 bool）。本批仅注册类原型作 instanceof
 *   锚点；实例由批次4 getFenceConnectivity 经 ScriptClassRegistry 查 classId/proto 构造。
 * - `NavigationResult`：寻路结果空壳（寻路 stub 故最小空壳）。属性 isFullPath（恒 false）、
 *   getPath()（空 Vector3[]）。登记备用，寻路做实后补。
 * - `LookDuration`：字符串枚举对象（Continuous/Instant/UntilMove），值=自身名。全项目此前无此类型。
 *
 * @param builder 模块构建器（exportClass/exportValue 用）。
 * @param ctx 绑定上下文。
 */
void registerGameTestTypesClasses(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx);

} // namespace mc::test
