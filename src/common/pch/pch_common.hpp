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

// ============================================================================
// 预编译头文件（PCH）
// ----------------------------------------------------------------------------
// 收录全项目高频且稳定的头文件，由 CMake target_precompile_headers 预编译为
// 单个 .pch 产物，供各 TU 复用，消除重复解析开销。详见 src/common/pch/README.md。
//
// 选型原则：
//   1. 高频被引用（覆盖大量 TU，乘以引用次数才有收益）
//   2. 稳定低频改动（PCH 头改动会触发全量重编译，必须选基础头）
//   3. 干净无警告（PCH 编译单元继承 -Werror，触发警告的头须显式豁免）
//
// 严禁放入业务头（BlockState/ChunkData/网络包定义等高频演进头）。
// ============================================================================

#pragma once

// ----------------------------------------------------------------------------
// 标准库高频头：被全项目绝大多数 TU 引用，重复解析开销大
// ----------------------------------------------------------------------------
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

// ----------------------------------------------------------------------------
// 项目自有基础头：稳定低频改动，且被高频引用
//
// Result.hpp 传递引入 <spdlog/spdlog.h>（重头），是 PCH 的高杠杆点之一。
// spdlog 在 -Wall -Wextra -pedantic 下可能产生警告，且项目 -Werror 默认开启，
// 故在此处用 pragma 局部豁免关键警告，保证 PCH 编译单元创建成功。
// pragma 作用域仅限本 PCH 头的展开区间，使用方 TU 的警告行为不受影响。
// ----------------------------------------------------------------------------
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#pragma clang diagnostic ignored "-Wnewline-eof"
#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif

#include <spdlog/spdlog.h>

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif
