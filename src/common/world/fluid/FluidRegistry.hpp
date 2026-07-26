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

#include "Fluid.hpp"
#include <memory>

namespace mc::fluid {

/**
 * @brief 流体注册表
 *
 * 管理所有流体的注册和查找。
 * 单例模式，类似于BlockRegistry。
 *
 * 参考: net.minecraft.fluid.Fluids (注册)
 *       net.minecraftforge.registries.ForgeRegistry (注册表)
 *
 * 用法示例:
 * @code
 * // 注册流体
 * auto& registry = FluidRegistry::instance();
 * registry.registerFluid<WaterFluid::Source>("minecraft:water");
 * registry.registerFluid<WaterFluid::Flowing>("minecraft:flowing_water");
 *
 * // 查找流体
 * Fluid* water = FluidRegistry::getFluid(ResourceLocation("minecraft:water"));
 * @endcode
 */
class FluidRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static FluidRegistry& instance();

    /**
     * @brief 初始化注册表
     *
     * 注册所有内置流体。
     */
    void initialize();

    /**
     * @brief 注册流体
     *
     * @tparam FluidType 流体类型
     * @param id 流体资源位置
     * @return 流体指针
     */
    template <typename FluidType>
    Fluid* registerFluid(const ResourceLocation& id)
    {
        auto fluid = std::make_unique<FluidType>();
        Fluid* ptr = fluid.get();

        u32 fluidId = static_cast<u32>(m_fluids.size());
        _registerFluidInternal(ptr, id, fluidId);

        m_fluids.push_back(std::move(fluid));
        return ptr;
    }

    /**
     * @brief 根据ID获取流体
     */
    [[nodiscard]] Fluid* getFluid(u32 fluidId) const;

    /**
     * @brief 根据资源位置获取流体
     */
    [[nodiscard]] Fluid* getFluid(const ResourceLocation& id) const;

    /**
     * @brief 获取流体数量
     */
    [[nodiscard]] size_t fluidCount() const { return m_fluids.size(); }

    // ========== 内置流体ID ==========

    /// 空流体ID
    static constexpr u32 EMPTY_ID = 0;
    /// 水源头ID
    static constexpr u32 WATER_ID = 1;
    /// 流动水ID
    static constexpr u32 FLOWING_WATER_ID = 2;
    /// 岩浆源头ID
    static constexpr u32 LAVA_ID = 3;
    /// 流动岩浆ID
    static constexpr u32 FLOWING_LAVA_ID = 4;

private:
    FluidRegistry() = default;
    FluidRegistry(const FluidRegistry&) = delete;
    FluidRegistry& operator=(const FluidRegistry&) = delete;

    /**
     * @brief 内部注册流体
     */
    void _registerFluidInternal(Fluid* fluid, const ResourceLocation& id, u32 fluidId);

private:
    std::vector<std::unique_ptr<Fluid>> m_fluids;
    std::unordered_map<ResourceLocation, Fluid*> m_fluidsById;
    std::unordered_map<u32, Fluid*> m_fluidsByNumericId;

    bool m_initialized = false;

    friend class Fluid;
    friend class FluidState;
};

} // namespace mc::fluid
