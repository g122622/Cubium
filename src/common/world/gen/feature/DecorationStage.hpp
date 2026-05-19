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

#include "../../../core/Types.hpp"
#include <string>
#include <vector>

namespace mc {

/**
 * @brief 装饰阶段枚举
 *
 * 定义特征生成的顺序阶段。
 * 参考 MC 1.16.5 GenerationStage.Decoration
 *
 * 生成顺序：
 * 1. RAW_GENERATION - 原始生成（基岩、岛屿）
 * 2. LAKES - 湖泊
 * 3. LOCAL_MODIFICATIONS - 局部修改
 * 4. UNDERGROUND_STRUCTURES - 地下结构（地牢）
 * 5. SURFACE_STRUCTURES - 地表结构（村庄、神殿）
 * 6. STRONGHOLDS - 要塞
 * 7. UNDERGROUND_ORES - 地下矿石
 * 8. UNDERGROUND_DECORATION - 地下装饰（化石）
 * 9. VEGETAL_DECORATION - 植被装饰（树木、花草）
 * 10. TOP_LAYER_MODIFICATION - 顶层修改（雪、冰）
 */
enum class DecorationStage : u8 {
    RawGeneration = 0,     ///< 原始生成
    Lakes,                 ///< 湖泊
    LocalModifications,    ///< 局部修改
    UndergroundStructures, ///< 地下结构
    SurfaceStructures,     ///< 地表结构
    Strongholds,           ///< 要塞
    UndergroundOres,       ///< 地下矿石
    UndergroundDecoration, ///< 地下装饰
    VegetalDecoration,     ///< 植被装饰
    TopLayerModification,  ///< 顶层修改

    Count ///< 阶段总数
};

/**
 * @brief 装饰阶段工具类
 *
 * 提供装饰阶段相关的工具方法。
 */
struct DecorationStages {
    /**
     * @brief 获取所有装饰阶段（按顺序）
     * @return 装饰阶段列表
     */
    static const std::vector<DecorationStage>& getAll()
    {
        static const std::vector<DecorationStage> stages = {DecorationStage::RawGeneration,
            DecorationStage::Lakes,
            DecorationStage::LocalModifications,
            DecorationStage::UndergroundStructures,
            DecorationStage::SurfaceStructures,
            DecorationStage::Strongholds,
            DecorationStage::UndergroundOres,
            DecorationStage::UndergroundDecoration,
            DecorationStage::VegetalDecoration,
            DecorationStage::TopLayerModification};
        return stages;
    }

    /**
     * @brief 获取阶段名称
     * @param stage 装饰阶段
     * @return 阶段名称字符串
     */
    static const char* getName(DecorationStage stage)
    {
        switch (stage) {
            case DecorationStage::RawGeneration:
                return "raw_generation";
            case DecorationStage::Lakes:
                return "lakes";
            case DecorationStage::LocalModifications:
                return "local_modifications";
            case DecorationStage::UndergroundStructures:
                return "underground_structures";
            case DecorationStage::SurfaceStructures:
                return "surface_structures";
            case DecorationStage::Strongholds:
                return "strongholds";
            case DecorationStage::UndergroundOres:
                return "underground_ores";
            case DecorationStage::UndergroundDecoration:
                return "underground_decoration";
            case DecorationStage::VegetalDecoration:
                return "vegetal_decoration";
            case DecorationStage::TopLayerModification:
                return "top_layer_modification";
            default:
                return "unknown";
        }
    }

    /**
     * @brief 获取阶段索引
     * @param stage 装饰阶段
     * @return 索引值（0-9）
     */
    static constexpr u8 getIndex(DecorationStage stage) { return static_cast<u8>(stage); }

    /**
     * @brief 根据索引获取阶段
     * @param index 索引值
     * @return 装饰阶段，如果无效返回 RawGeneration
     */
    static DecorationStage fromIndex(u8 index)
    {
        if (index < static_cast<u8>(DecorationStage::Count)) {
            return static_cast<DecorationStage>(index);
        }
        return DecorationStage::RawGeneration;
    }

    /**
     * @brief 根据名称获取阶段
     * @param name 阶段名称
     * @return 装饰阶段，如果无效返回 RawGeneration
     */
    static DecorationStage fromName(const std::string& name)
    {
        if (name == "raw_generation") return DecorationStage::RawGeneration;
        if (name == "lakes") return DecorationStage::Lakes;
        if (name == "local_modifications") return DecorationStage::LocalModifications;
        if (name == "underground_structures") return DecorationStage::UndergroundStructures;
        if (name == "surface_structures") return DecorationStage::SurfaceStructures;
        if (name == "strongholds") return DecorationStage::Strongholds;
        if (name == "underground_ores") return DecorationStage::UndergroundOres;
        if (name == "underground_decoration") return DecorationStage::UndergroundDecoration;
        if (name == "vegetal_decoration") return DecorationStage::VegetalDecoration;
        if (name == "top_layer_modification") return DecorationStage::TopLayerModification;
        return DecorationStage::RawGeneration;
    }
};

} // namespace mc
