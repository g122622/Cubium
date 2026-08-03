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

#include "Biome.hpp"
#include "BiomeFactory.hpp"
#include "BiomeIds.hpp"
#include "common/core/Types.hpp"
#include <vector>

namespace mc {
namespace world {
namespace biome {

/**
 * @brief 生物群系注册表
 *
 * 管理所有注册的生物群系定义。
 *
 * 使用方法：
 * @code
 * const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
 * @endcode
 */
class BiomeRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static BiomeRegistry& instance();

    /**
     * @brief 初始化注册表（注册所有默认生物群系）
     */
    void initialize();

    /**
     * @brief 注册生物群系
     * @param biome 生物群系定义
     */
    void registerBiome(Biome biome);

    /**
     * @brief 获取生物群系定义
     * @param id 生物群系ID
     * @return 生物群系定义，如果不存在返回默认生物群系
     */
    [[nodiscard]] const Biome& get(BiomeId id) const;

    /**
     * @brief 获取可变生物群系引用
     *
     * 用于 BiomeLoader 等数据驱动加载器在已注册的 Biome 上叠加 JSON 字段
     * （气候、效果、生成设置、刷怪信息）。调用方需先通过 hasBiome 确认 id 已注册；
     * 未注册时返回内部默认生物群系（Plains）的可变引用——此时写入会污染默认值，
     * 因此调用方有责任先检查 hasBiome。
     *
     * @param id 生物群系ID
     * @return 生物群系可变引用
     */
    [[nodiscard]] Biome& getMutable(BiomeId id);

    /**
     * @brief 检查生物群系是否已注册
     */
    [[nodiscard]] bool hasBiome(BiomeId id) const;

    /**
     * @brief 获取所有已注册的生物群系
     */
    [[nodiscard]] const std::vector<Biome>& allBiomes() const { return m_biomes; }

private:
    BiomeRegistry();
    std::vector<Biome> m_biomes;
    std::vector<bool> m_registered;
    Biome m_defaultBiome;

    void _registerDefaultBiomes();
};

} // namespace biome
} // namespace world
} // namespace mc

// 旧命名空间兼容别名
namespace mc {
using BiomeRegistry = ::mc::world::biome::BiomeRegistry;
} // namespace mc
