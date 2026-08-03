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

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <string>

namespace mc {
namespace command {

/**
 * @brief LocateCommand - 定位结构
 *
 * 用法: /locate <structure>
 * 权限: 0 (所有玩家)
 */
class LocateCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

    /**
     * @brief 将用户输入的结构名称规范化为 ResourceLocation
     *
     * 支持带命名空间（minecraft:village）、不带命名空间（village）、
     * 以及常见别名（mansion → minecraft:mansion）。
     *
     * @param name 用户输入的结构名称
     * @return 规范化后的 ResourceLocation
     */
    static ResourceLocation _normalizeToResourceLocation(const std::string& name);

private:
    static i32 _locateStructure(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
