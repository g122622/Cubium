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
#include <atomic>

namespace mc::entity {

/**
 * @brief 实体类继承链标识
 *
 * 复刻 vanilla 1.21.11 `net.minecraft.util.ClassTreeIdRegistry` 的字段 id 分配语义：
 * 每个实体类持有一个静态 EntityClassInfo，记录类名、父类 classInfo 指针、以及本类
 * 已分配的最高 synched-data id。id 分配时沿父类链查找最高 id，+1 续接——基类字段 id
 * 在所有子类中共享，子类字段从基类最高 id+1 起连续编号，与 vanilla ClassTreeIdRegistry
 * 逐字段对齐。
 *
 * C++ 无 Java 的运行时类反射，故用每个类持有的静态 EntityClassInfo 对象（地址稳定）
 * 显式构建继承链。parent 指针在运行时（registerData 时）解引用，此时所有静态对象
 * 已构造，跨翻译单元静态初始化顺序无关。
 */
struct EntityClassInfo {
    const char* name;                        ///< 类名（调试用）
    const EntityClassInfo* parent;           ///< 父类 classInfo（根类为 nullptr）
    mutable std::atomic<i32> lastAssignedId; ///< 本类已分配的最高 id（初始 -1）

    explicit EntityClassInfo(const char* n, const EntityClassInfo* p = nullptr)
        : name(n)
        , parent(p)
        , lastAssignedId(-1)
    {}
};

} // namespace mc::entity
