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

#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <unordered_map>

namespace mc::world::gen::density {

class DensityFunction;

/**
 * @brief 密度函数注册表（MC 1.21.11 worldgen/density_function）
 *
 * name → shared_ptr<DensityFunction>。DensityFunctionLoader 从数据包加载 35 个
 * density_function JSON，经 Holder 引用解析后注册到本表。noise_settings JSON 的
 * noise_router 15 字段是 DF Holder（字符串 RL 或内联对象），由 NoiseSettingsLoader
 * 查本表解析。
 *
 * 进程级单例。共享子图：多个父引用同一 name 时返回同一 shared_ptr（DensityFunctionLoader
 * 用 factory::sharedHolder 包装），与原版 Holder<DensityFunction> 语义一致。
 */
class DensityFunctionRegistry {
public:
    static DensityFunctionRegistry& instance();

    /**
     * @brief 注册命名密度函数
     *
     * DensityFunctionLoader 解析完一个 density_function JSON（含 Holder 引用展开）后调用。
     *
     * @param name 资源位置（如 "minecraft:overworld/sloped_cheese"）
     * @param function 密度函数（共享所有权，多父引用复用）
     */
    void registerFunction(const ResourceLocation& name, std::shared_ptr<DensityFunction> function);

    /**
     * @brief 按名取密度函数
     * @return 密度函数，或 nullptr（未注册）
     */
    [[nodiscard]] std::shared_ptr<DensityFunction> get(const ResourceLocation& name) const;

    /** 是否已注册 */
    [[nodiscard]] bool has(const ResourceLocation& name) const;

    /** 清空注册表（数据驱动加载前调用） */
    void clear() noexcept;

    /**
     * @brief 标记数据驱动加载状态
     *
     * true=已由 DensityFunctionLoader 注入；false=重置（恢复兜底可用）。
     */
    void markLoadedFromDatapack(bool loaded) noexcept;

    /** 是否已由数据驱动加载 */
    [[nodiscard]] bool isLoadedFromDatapack() const noexcept;

private:
    DensityFunctionRegistry() = default;
    ~DensityFunctionRegistry() = default;
    DensityFunctionRegistry(const DensityFunctionRegistry&) = delete;
    DensityFunctionRegistry& operator=(const DensityFunctionRegistry&) = delete;

    std::unordered_map<ResourceLocation, std::shared_ptr<DensityFunction>> m_functions;
    bool m_loadedFromDatapack = false;
};

} // namespace mc::world::gen::density
