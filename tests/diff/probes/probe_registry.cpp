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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file probe_registry.cpp
 * @brief 差分探针的全局注册表：链接期收集全部 `MC_DIFF_DEFINE_PROBE` 定义。
 *
 * 注册表刻意用「函数内静态 + 懒分配」：各 probe 源文件里的静态 registrar 在 main
 * 之前构造，那时全局 vector 的构造顺序尚无保证；改用局部静态保证首次访问时已就绪。
 */

#include <algorithm>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

#include "probe.hpp"

namespace mc::diff {

namespace {

std::vector<const Probe*>& storage()
{
    static std::vector<const Probe*> entries;
    return entries;
}

bool& sorted()
{
    static bool value = false;
    return value;
}

} // namespace

ProbeRegistrar::ProbeRegistrar(const Probe& probe)
{
    storage().push_back(&probe);
    sorted() = false;
}

const std::vector<const Probe*>& registeredProbes()
{
    std::vector<const Probe*>& entries = storage();
    if (!sorted()) {
        std::sort(entries.begin(), entries.end(), [](const Probe* lhs, const Probe* rhs) {
            return lhs->domain < rhs->domain;
        });
        sorted() = true;
    }
    return entries;
}

const Probe* findProbe(std::string_view domain)
{
    for (const Probe* probe : registeredProbes()) {
        if (probe->domain == domain) {
            return probe;
        }
    }
    return nullptr;
}

} // namespace mc::diff
