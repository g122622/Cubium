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

#include "JigsawPiece.hpp"
#include "common/core/Result.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace mc {

namespace resource {
class DataPackList;
} // namespace resource

namespace world {
namespace gen {
namespace jigsaw {

class JigsawPattern;
class JigsawPatternRegistry;

/**
 * @brief 模板池 JSON 加载器
 *
 * 从数据包加载 Jigsaw 模板池 JSON 文件。
 *
 * JSON 格式 (MC 1.16.5):
 * {
 *   "name": "minecraft:village/plains/town_centers",
 *   "fallback": "minecraft:village/plains/terminators",
 *   "elements": [
 *     {
 *       "weight": 1,
 *       "element": {
 *         "element_type": "minecraft:single_pool_element",
 *         "location": "minecraft:village/plains/town_center_01",
 *         "processors": "minecraft:empty",
 *         "projection": "rigid"
 *       }
 *     }
 *   ]
 * }
 *
 * 加载路径: data/<namespace>/worldgen/template_pool/<path>.json
 */
class TemplatePoolLoader {
public:
    /**
     * @brief 从数据包列表加载所有模板池
     *
     * @param dataPackList 数据包列表
     * @return 加载的模板池数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackList(const resource::DataPackList& dataPackList);

    /**
     * @brief 从单个资源包加载所有模板池
     *
     * @param pack 资源包
     * @return 加载的模板池数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const IResourcePack& pack);

    /**
     * @brief 从 JSON 字符串加载单个模板池
     *
     * @param json JSON 内容
     * @param location 模板池资源位置
     * @return 加载的模板池，或错误信息
     */
    [[nodiscard]] static Result<std::unique_ptr<JigsawPattern>> loadFromJson(
        const std::string& json, const ResourceLocation& location);

    /**
     * @brief 从 JSON 对象加载单个模板池
     *
     * @param jsonObj 已解析的 JSON 对象
     * @param location 模板池资源位置
     * @return 加载的模板池，或错误信息
     */
    [[nodiscard]] static Result<std::unique_ptr<JigsawPattern>> loadFromJson(
        const nlohmann::json& jsonObj, const ResourceLocation& location);

    // ============================================================================
    // 私有方法
    // ============================================================================
    /**
     * @brief 解析单个元素
     *
     * @param elementObj 元素 JSON 对象
     * @param outPiece 输出参数：解析的拼图块
     * @param outWeight 输出参数：权重
     * @return 是否成功
     */
    static bool _parseElement(const nlohmann::json& elementObj, std::unique_ptr<JigsawPiece>& outPiece, i32& outWeight);

    /**
     * @brief 解析元素类型
     *
     * @param elementObj 元素 JSON 对象
     * @return 解析的拼图块，失败返回 nullptr
     */
    static std::unique_ptr<JigsawPiece> _parseElementType(const nlohmann::json& elementObj);

    /**
     * @brief 解析 single_pool_element 类型
     */
    static std::unique_ptr<JigsawPiece> _parseSinglePoolElement(const nlohmann::json& elementObj);

    /**
     * @brief 解析 list_pool_element 类型
     */
    static std::unique_ptr<JigsawPiece> _parseListPoolElement(const nlohmann::json& elementObj);

    /**
     * @brief 解析 empty_pool_element 类型
     */
    static std::unique_ptr<JigsawPiece> _parseEmptyPoolElement(const nlohmann::json& elementObj);

    /**
     * @brief 解析 feature_pool_element 类型
     */
    static std::unique_ptr<JigsawPiece> _parseFeaturePoolElement(const nlohmann::json& elementObj);

    /**
     * @brief 解析投影类型
     *
     * @param projectionStr 投影类型字符串
     * @return 投影类型
     */
    static JigsawPlacementBehaviour _parseProjection(const std::string& projectionStr);
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
