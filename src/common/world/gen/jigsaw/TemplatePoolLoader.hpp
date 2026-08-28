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
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <simdjson.h>

namespace mc {

namespace resource {
class DataPackRepository;
} // namespace resource

namespace world {
namespace gen {
namespace jigsaw {

class TemplatePool;
class TemplatePoolRegistry;

/**
 * @brief 模板池 JSON 加载器
 *
 * 从数据包加载 Jigsaw 模板池 JSON 文件。JSON 解析使用 simdjson On-Demand,
 * 只读场景下字段访问走哈希查找而非逐次字符串比较,消除原 nlohmann DOM 的每元素
 * ~12μs lookup 开销。
 */
class TemplatePoolLoader {
public:
    /**
     * @brief 从数据包列表加载所有模板池
     *
     * @param dataPackList 数据包列表
     * @return 加载的模板池数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

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
    [[nodiscard]] static Result<std::unique_ptr<TemplatePool>> loadFromJson(
        const std::string& json, const ResourceLocation& location);

    // ============================================================================
    // 私有方法
    // ============================================================================
private:
    /**
     * @brief 从 simdjson On-Demand 文档加载单个模板池
     *
     * @param doc 已解析的 simdjson On-Demand 文档
     * @param location 模板池资源位置
     * @return 加载的模板池，或错误信息
     */
    static Result<std::unique_ptr<TemplatePool>> loadFromJson(
        simdjson::ondemand::document& doc, const ResourceLocation& location);

    /**
     * @brief 解析单个元素
     *
     * @param elementObj 元素 JSON 对象
     * @param outPiece 输出参数：解析的拼图块
     * @param outWeight 输出参数：权重
     * @return 是否成功
     */
    static bool _parseElement(
        simdjson::ondemand::object& elementObj, std::unique_ptr<JigsawPiece>& outPiece, i32& outWeight);

    /**
     * @brief 解析元素类型
     *
     * @param elementObj 元素 JSON 对象
     * @return 解析的拼图块，失败返回 nullptr
     */
    static std::unique_ptr<JigsawPiece> _parseElementType(simdjson::ondemand::object& elementObj);

    /**
     * @brief 解析 single_pool_element 类型
     */
    static std::unique_ptr<JigsawPiece> _parseSinglePoolElement(simdjson::ondemand::object& elementObj);

    /**
     * @brief 解析 legacy_single_pool_element 类型
     *
     * 与 single_pool_element 相同，但放置时使用 STRUCTURE_AND_AIR 忽略策略。
     */
    static std::unique_ptr<JigsawPiece> _parseLegacySinglePoolElement(simdjson::ondemand::object& elementObj);

    /**
     * @brief 解析 list_pool_element 类型
     */
    static std::unique_ptr<JigsawPiece> _parseListPoolElement(simdjson::ondemand::object& elementObj);

    /**
     * @brief 解析 empty_pool_element 类型
     */
    static std::unique_ptr<JigsawPiece> _parseEmptyPoolElement();

    /**
     * @brief 解析 feature_pool_element 类型
     */
    static std::unique_ptr<JigsawPiece> _parseFeaturePoolElement(simdjson::ondemand::object& elementObj);

    /**
     * @brief 解析 processors 字段
     *
     * processors 字段可以是：
     * - 字符串："minecraft:mossify_10_percent"（处理器列表引用）
     * - 对象：{"processors": [...]}（内联处理器列表）
     * - 数组：[...]（内联处理器列表）
     *
     * 内联处理器列表通过 ProcessorListLoader::parseInlineProcessorList 解析后注册到
     * ProcessorListRegistry，返回合成资源位置供 SingleJigsawPiece 查找。
     *
     * @param elementObj 包含 processors 字段的 JSON 对象
     * @return 处理器列表资源位置，无处理器则返回 nullopt
     */
    static std::optional<ResourceLocation> _parseProcessors(simdjson::ondemand::object& elementObj);

    /**
     * @brief 解析内联处理器数组并注册到 ProcessorListRegistry
     *
     * @param processorsArray 处理器 JSON 数组
     * @return 合成资源位置（inline_processor_list_<序号>）；空数组返回 minecraft:empty
     */
    static std::optional<ResourceLocation> _registerInlineProcessors(simdjson::ondemand::value& processorsValue);

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
