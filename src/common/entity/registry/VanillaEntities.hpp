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

#include "common/entity/core/EntityRegistry.hpp"
#include <string>

namespace mc {
namespace entity {

/**
 * @brief 原版实体初始化器
 *
 * 注册所有原版实体类型到实体注册表。
 * 必须在服务器启动时或客户端初始化时调用。
 *
 * 注册实现位于 VanillaEntities.cpp，本头文件仅暴露声明，避免下游包含被迫拉入
 * 全部实体子类头文件。
 */
class VanillaEntities {
public:
    /**
     * @brief 注册所有原版实体类型
     *
     * 包括动物、怪物和其他实体类型。
     * 此方法线程安全且幂等：若注册表已包含原版实体（如 minecraft:pig）则跳过；
     * 若注册表被清空（例如测试中调用了 EntityRegistry::clear()）则重新注册。
     * 必须在服务器启动时或客户端初始化时调用。
     */
    static void registerAll();

    /**
     * @brief 获取实体类型的本地化名称
     * @param typeId 实体类型ID
     * @return 本地化名称键（如 entity.minecraft.pig）
     */
    static std::string getLocalizedNameKey(const std::string& typeId);

private:
    /**
     * @brief 实际执行所有原版实体类型的注册
     *
     * 由 registerAll() 在哨兵检查通过后调用，调用方负责互斥与幂等保护。
     */
    static void doRegisterAll();
};

} // namespace entity
} // namespace mc
