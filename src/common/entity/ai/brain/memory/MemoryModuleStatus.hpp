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

#include <cstddef>
#include <functional>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief 内存模块状态枚举
 *
 * 用于检查内存模块的状态
 */
enum class MemoryModuleStatus {
    VALUE_PRESENT, // 内存有值
    VALUE_ABSENT,  // 内存无值
    REGISTERED     // 已注册
};

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc

// std::hash 特化 - 用于 std::unordered_set/std::unordered_map
namespace std {

template <>
struct hash<mc::entity::ai::brain::memory::MemoryModuleStatus> {
    size_t operator()(mc::entity::ai::brain::memory::MemoryModuleStatus status) const noexcept
    {
        return static_cast<size_t>(status);
    }
};

} // namespace std
