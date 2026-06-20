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

#include "FlatLayerInfo.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockState.hpp"
#include <vector>

namespace mc {

/**
 * @brief 平坦世界生成设置
 *
 * 定义平坦世界的层配置、生物群系和特性标志。
 *
 * MC 默认配置：
 * - 生物群系: Plains
 * - 层: 1x Bedrock + 2x Dirt + 1x Grass Block
 * - decoration: false
 * - addLakes: false
 */
class FlatLevelGeneratorSettings {
public:
    FlatLevelGeneratorSettings() = default;

    /**
     * @brief 构造平坦世界生成设置
     * @param biomeId 生物群系 ID
     * @param decoration 是否添加生物群系装饰特性
     * @param addLakes 是否添加湖泊
     */
    FlatLevelGeneratorSettings(BiomeId biomeId, bool decoration = false, bool addLakes = false)
        : m_biomeId(biomeId)
        , m_decoration(decoration)
        , m_addLakes(addLakes)
    {}

    // === 层配置 ===

    /** 获取层信息列表（可修改） */
    std::vector<FlatLayerInfo>& layersInfo() { return m_layersInfo; }

    /** 获取层信息列表（只读） */
    [[nodiscard]] const std::vector<FlatLayerInfo>& layersInfo() const { return m_layersInfo; }

    /**
     * @brief 获取展开后的层列表（每个 Y 级别一个 BlockState）
     *
     * 从底部（minY）开始，逐层展开为每个 Y 级别一个 BlockState。
     * 不阻挡运动的方块（如水）替换为 nullptr，由特性系统放置。
     *
     * 必须在设置完 layersInfo 后调用 updateLayers() 来生成此列表。
     */
    [[nodiscard]] const std::vector<const BlockState*>& layers() const { return m_layers; }

    // === 生物群系 ===

    /** 获取生物群系 ID */
    [[nodiscard]] BiomeId biomeId() const { return m_biomeId; }

    /** 设置生物群系 ID */
    void setBiomeId(BiomeId id) { m_biomeId = id; }

    // === 特性标志 ===

    /** 是否添加生物群系装饰特性（矿石、树木等） */
    [[nodiscard]] bool hasDecoration() const { return m_decoration; }
    void setDecoration(bool decoration) { m_decoration = decoration; }

    /** 是否添加湖泊 */
    [[nodiscard]] bool hasLakes() const { return m_addLakes; }
    void setLakes(bool addLakes) { m_addLakes = addLakes; }

    // === 虚空检测 ===

    /** 是否为虚空世界（所有层都是空气） */
    [[nodiscard]] bool isVoidGen() const { return m_voidGen; }

    // === 更新操作 ===

    /**
     * @brief 从 layersInfo 重新计算展开层列表
     *
     * 必须在修改 layersInfo 后调用此方法。
     * 展开层列表从底部开始，每层按高度展开为多个条目。
     * 不阻挡运动的方块替换为 nullptr。
     */
    void updateLayers();

    /**
     * @brief 创建默认平坦世界设置
     *
     * 默认配置：
     * - 生物群系: Plains
     * - 层: 1x Bedrock + 2x Dirt + 1x Grass Block
     * - decoration: false
     * - addLakes: false
     */
    static FlatLevelGeneratorSettings createDefault();

private:
    std::vector<FlatLayerInfo> m_layersInfo; ///< 层定义（方块 + 高度）
    std::vector<const BlockState*>
        m_layers;                       ///< 展开后的层列表（每个 Y 级别一个 BlockState，nullptr 表示由特性系统放置）
    BiomeId m_biomeId = Biomes::Plains; ///< 生物群系
    bool m_decoration = false;          ///< 是否添加装饰特性
    bool m_addLakes = false;            ///< 是否添加湖泊
    bool m_voidGen = true;              ///< 是否为虚空世界（初始为 true，updateLayers 时更新）
};

} // namespace mc
