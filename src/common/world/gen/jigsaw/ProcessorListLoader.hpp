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

#include "ProcessorListRegistry.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/feature/template/Template.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <simdjson.h>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 处理器列表 JSON 加载器
 *
 * 从数据包加载 processor_list JSON 文件并注册到 ProcessorListRegistry。
 * JSON 解析使用 simdjson On-Demand,只读场景零拷贝字段访问。
 *
 * JSON 格式 (MC 1.21):
 * {
 *   "processors": [
 *     { "processor_type": "minecraft:block_rot", "integrity": 0.5 },
 *     { "processor_type": "minecraft:gravity", "heightmap": "WORLD_SURFACE_WG" },
 *     { "processor_type": "minecraft:jigsaw_replacement" }
 *   ]
 * }
 *
 * 加载路径: data/<namespace>/worldgen/processor_list/<path>.json
 */
class ProcessorListLoader {
public:
    /**
     * @brief 从数据包列表加载所有处理器列表
     *
     * @param dataPackList 数据包列表
     * @return 加载的处理器列表数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有处理器列表
     *
     * @param pack 资源包
     * @return 加载的处理器列表数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 字符串加载单个处理器列表
     *
     * @param json JSON 内容
     * @param location 处理器列表资源位置
     * @return 是否成功
     */
    [[nodiscard]] static Result<void> loadFromJson(const std::string& json, const ResourceLocation& location);

    /**
     * @brief 解析内联处理器列表（JSON 数组）
     *
     * 模板池元素的 processors 字段可为内联数组（如 [{"processor_type": "minecraft:gravity", ...}]），
     * 此方法解析数组中每个处理器并组装为 StructureProcessorList。
     * 用于 TemplatePoolLoader::_parseProcessors 处理内联处理器列表场景。
     *
     * @param processorsValue 处理器 JSON 数组(ondemand::value,内部 get_array 遍历)
     * @return 处理器列表，数组为空或解析失败时返回空列表
     */
    [[nodiscard]] static std::unique_ptr<feature::template_::StructureProcessorList> parseInlineProcessorList(
        simdjson::ondemand::value& processorsValue);

private:
    /**
     * @brief 从 simdjson On-Demand 对象加载处理器列表
     *
     * @param jsonObj 已解析的 JSON 对象
     * @param location 处理器列表资源位置
     * @return 是否成功
     */
    static Result<void> _loadFromJsonObj(simdjson::ondemand::object& jsonObj, const ResourceLocation& location);

    /**
     * @brief 解析单个处理器
     *
     * @param processorObj 处理器 JSON 对象
     * @return 处理器实例，或 nullptr
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseProcessor(
        simdjson::ondemand::object& processorObj);

    /**
     * @brief 解析 block_ignore 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseBlockIgnoreProcessor(
        simdjson::ondemand::object& processorObj);

    /**
     * @brief 解析 block_rot (integrity) 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseBlockRotProcessor(
        simdjson::ondemand::object& processorObj);

    /**
     * @brief 解析 gravity 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseGravityProcessor(
        simdjson::ondemand::object& processorObj);

    /**
     * @brief 解析 jigsaw_replacement 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseJigsawReplacementProcessor();

    /**
     * @brief 解析 rule 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseRuleProcessor(
        simdjson::ondemand::object& processorObj);

    /**
     * @brief 解析 block_age (mossification) 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseBlockAgeProcessor(
        simdjson::ondemand::object& processorObj);

    /**
     * @brief 解析 blackstone_replace 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseBlackstoneReplaceProcessor();

    /**
     * @brief 解析 lava_submerged_block 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseLavaSubmergedBlockProcessor();

    /**
     * @brief 解析 nop 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseNopProcessor();

    /**
     * @brief 解析 protected_blocks 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseProtectedBlocksProcessor(
        simdjson::ondemand::object& processorObj);

    /**
     * @brief 解析 capped 处理器
     */
    static std::unique_ptr<feature::template_::StructureProcessor> _parseCappedProcessor(
        simdjson::ondemand::object& processorObj);
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
