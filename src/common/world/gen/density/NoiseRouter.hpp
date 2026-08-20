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
 */

#pragma once

#include "common/world/biome/climate/Sampler.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include <array>
#include <memory>

namespace mc::world::gen::density {

/**
 * @brief 噪声路由器
 *
 * MC 1.18+ 的核心地形生成数据结构。
 * 持有 15 个密度函数引用，其中 6 个用于 Climate.Sampler，
 * 1 个用于最终密度计算，其余用于洞穴和矿石生成。
 *
 * 噪声路由器的所有密度函数由外部创建并传入（由 RandomState::create 经 NoiseBindingVisitor
 * 从 DimensionSettings::m_routerDfs 数据驱动模板绑定创建），NoiseRouter 仅持有 unique_ptr 管理生命周期。
 */
class NoiseRouter {
public:
    /**
     * @brief 构造噪声路由器
     *
     * 所有 15 个密度函数必须非空。
     */
    NoiseRouter(std::unique_ptr<DensityFunction> barrierNoise,
        std::unique_ptr<DensityFunction> fluidLevelFloodednessNoise,
        std::unique_ptr<DensityFunction> fluidLevelSpreadNoise,
        std::unique_ptr<DensityFunction> lavaNoise,
        std::unique_ptr<DensityFunction> temperature,
        std::unique_ptr<DensityFunction> vegetation,
        std::unique_ptr<DensityFunction> continents,
        std::unique_ptr<DensityFunction> erosion,
        std::unique_ptr<DensityFunction> depth,
        std::unique_ptr<DensityFunction> ridges,
        std::unique_ptr<DensityFunction> preliminarySurfaceLevel,
        std::unique_ptr<DensityFunction> finalDensity,
        std::unique_ptr<DensityFunction> veinToggle,
        std::unique_ptr<DensityFunction> veinRidged,
        std::unique_ptr<DensityFunction> veinGap);

    ~NoiseRouter() = default;

    NoiseRouter(NoiseRouter&&) noexcept = default;
    NoiseRouter& operator=(NoiseRouter&&) noexcept = default;

    // 禁止拷贝（持有 unique_ptr）
    NoiseRouter(const NoiseRouter&) = delete;
    NoiseRouter& operator=(const NoiseRouter&) = delete;

    // ========== Climate 参数密度函数 ==========

    [[nodiscard]] const DensityFunction& temperature() const { return *m_temperature; }
    [[nodiscard]] const DensityFunction& vegetation() const { return *m_vegetation; }
    [[nodiscard]] const DensityFunction& continents() const { return *m_continents; }
    [[nodiscard]] const DensityFunction& erosion() const { return *m_erosion; }
    [[nodiscard]] const DensityFunction& depth() const { return *m_depth; }
    [[nodiscard]] const DensityFunction& ridges() const { return *m_ridges; }

    // ========== 地形密度函数 ==========

    [[nodiscard]] const DensityFunction& finalDensity() const { return *m_finalDensity; }
    [[nodiscard]] const DensityFunction& preliminarySurfaceLevel() const { return *m_preliminarySurfaceLevel; }

    // ========== 洞穴密度函数 ==========

    [[nodiscard]] const DensityFunction& barrierNoise() const { return *m_barrierNoise; }
    [[nodiscard]] const DensityFunction& fluidLevelFloodednessNoise() const { return *m_fluidLevelFloodednessNoise; }
    [[nodiscard]] const DensityFunction& fluidLevelSpreadNoise() const { return *m_fluidLevelSpreadNoise; }
    [[nodiscard]] const DensityFunction& lavaNoise() const { return *m_lavaNoise; }

    // ========== 矿脉密度函数 ==========

    [[nodiscard]] const DensityFunction& veinToggle() const { return *m_veinToggle; }
    [[nodiscard]] const DensityFunction& veinRidged() const { return *m_veinRidged; }
    [[nodiscard]] const DensityFunction& veinGap() const { return *m_veinGap; }

    /**
     * @brief 创建 Climate.Sampler
     *
     * 使用此路由器的 6 个气候密度函数创建采样器。
     * 采样器用于在指定位置采样气候参数值。
     */
    [[nodiscard]] mc::world::biome::climate::Sampler createClimateSampler() const;

    /**
     * @brief 对所有 15 个密度函数递归应用 visitor
     *
     * MC 1.21 对应 NoiseRouter.mapAll(Visitor)。
     * 对每个密度函数调用 mapAll(visitor)，将结果写回路由器。
     * 用于 NoiseChunk 构造时将 Marker 类型替换为特定实现。
     *
     * @param visitor 访问者
     */
    void mapAll(DensityFunction::Visitor& visitor);

    /**
     * @brief 对所有 15 个密度函数递归应用 visitor，返回新路由器（不改 this）
     *
     * 性能优化路径：RandomState::createRouterCopy 用此方法从已共享化的 m_router
     * 派生每区块独立副本。与 mapAll 的区别：mapAll 是非 const、applyInPlace 就地替换
     * 成员（用于 NoiseChunk 在自己的副本上替换 Marker）；mapAllCopy 是 const、对每个
     * 成员调用 mapAll(visitor)（const，返回新 unique_ptr）组装成新 NoiseRouter，this 不变。
     *
     * 共享语义：m_router 经 extractShareableTopology 处理后，纯拓扑子树已包装为
     * SharedTopology。SharedTopology::mapAll 返回持同一 shared_ptr 的新包装（零深拷贝），
     * 故 mapAllCopy 对纯拓扑子树零深拷贝，仅含 Marker / per-chunk 可变节点的路径每区块
     * 深拷贝。这是 createRouterCopy 不再每区块整树深拷贝的核心。
     *
     * @param visitor 访问者（RandomState::createRouterCopy 传入 NoiseBindingVisitor，
     *                对已绑定树表现为透传 + Marker 路径深拷贝）
     * @return 新的独立 NoiseRouter（纯拓扑子树共享，Marker 路径独立）
     */
    [[nodiscard]] NoiseRouter mapAllCopy(DensityFunction::Visitor& visitor) const;

    /**
     * @brief 提取 finalDensity 密度函数（移出所有权）
     *
     * 用于 NoiseChunk 构造时提取 finalDensity，叠加 BeardifierMarker 后替换回去。
     * 调用后 finalDensity() 将返回空指针，必须立即调用 replaceFinalDensity()。
     */
    [[nodiscard]] std::unique_ptr<DensityFunction> extractFinalDensity();

    /**
     * @brief 替换 finalDensity 密度函数
     *
     * 用于将叠加了 BeardifierMarker 并包装在 CacheAllInCell 中的组合密度函数
     * 设置回路由器。
     */
    void replaceFinalDensity(std::unique_ptr<DensityFunction> density);

private:
    // 洞穴
    std::unique_ptr<DensityFunction> m_barrierNoise;
    std::unique_ptr<DensityFunction> m_fluidLevelFloodednessNoise;
    std::unique_ptr<DensityFunction> m_fluidLevelSpreadNoise;
    std::unique_ptr<DensityFunction> m_lavaNoise;

    // 气候参数
    std::unique_ptr<DensityFunction> m_temperature;
    std::unique_ptr<DensityFunction> m_vegetation;
    std::unique_ptr<DensityFunction> m_continents;
    std::unique_ptr<DensityFunction> m_erosion;
    std::unique_ptr<DensityFunction> m_depth;
    std::unique_ptr<DensityFunction> m_ridges;

    // 地形
    std::unique_ptr<DensityFunction> m_preliminarySurfaceLevel;
    std::unique_ptr<DensityFunction> m_finalDensity;

    // 矿脉
    std::unique_ptr<DensityFunction> m_veinToggle;
    std::unique_ptr<DensityFunction> m_veinRidged;
    std::unique_ptr<DensityFunction> m_veinGap;
};

} // namespace mc::world::gen::density
