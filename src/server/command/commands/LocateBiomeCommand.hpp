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
#include "server/command/ServerCommandSource.hpp"
#include <optional>
#include <string>

namespace mc {
namespace command {

/**
 * @brief 定位生物群系的实现（原版 `/locate biome` 分支）
 *
 * 原版 1.19 起没有独立的 `/locatebiome` 命令，生物群系定位是 `/locate biome <biome>`
 * 的一个分支；这里保留实现、去掉旧的根命令，由 {@link LocateCommand} 挂到 `biome` 字面量下。
 */
class LocateBiomeCommand {
public:
    /// `/locate biome <biome>` 的处理函数（参数名 `biome`）。
    static i32 locateBiome(CommandContext<ServerCommandSource>& context);

private:
    static std::optional<BiomeId> _parseBiomeId(const std::string& name) noexcept;
};

} // namespace command
} // namespace mc
