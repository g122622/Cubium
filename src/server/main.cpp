/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "application/ServerApplicationEntry.hpp"
#include "common/core/Types.hpp"
#include "common/util/PathEncoding.hpp"

#include <string>
#include <vector>

mc::i32 main(mc::i32 argc, char* argv[])
{
    // Windows 上 CRT 会把宽命令行按**当前 ANSI 代码页**压成窄 argv，代码页外的字符
    // （例如 --config 指向含中文的路径）在进入 main 之前就已经变成 '?'，无法还原。
    // 因此这里重新从 GetCommandLineW() 取原始宽字符命令行并转成 UTF-8，让 gflags 与
    // 后续所有路径构造都消费 UTF-8；非 Windows 平台与解析失败时退回 CRT 的 argv。
    std::vector<std::string> utf8Storage = mc::util::commandLineArgumentsUtf8();
    if (utf8Storage.empty()) {
        mc::server::ServerApplicationEntry entry;
        return entry.run(argc, argv);
    }

    // gflags 需要 char* 形式的可写参数数组，字符串本体由 utf8Storage 持有。
    std::vector<char*> utf8Argv;
    utf8Argv.reserve(utf8Storage.size() + 1);
    for (std::string& argument : utf8Storage) {
        utf8Argv.push_back(argument.data());
    }
    utf8Argv.push_back(nullptr);

    mc::server::ServerApplicationEntry entry;
    return entry.run(static_cast<mc::i32>(utf8Storage.size()), utf8Argv.data());
}
