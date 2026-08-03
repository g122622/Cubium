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

#include "DamageTypeTag.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

/**
 * @brief 伤害类型标签 JSON 加载器
 *
 * 从数据包加载伤害类型标签 JSON 文件，对应 MC 1.21.11 的 damage_type 标签系统。
 *
 * JSON 格式:
 * {
 *   "replace": false,
 *   "values": [
 *     "minecraft:drown",
 *     "#minecraft:bypasses_invulnerability"
 *   ]
 * }
 *
 * values 中以 # 开头的条目表示引用其他标签。
 *
 * 加载路径: data/<namespace>/tags/damage_type/
 *
 * 多数据包合并语义：
 * - 默认追加模式：新数据包的标签内容追加到已有标签内容之后
 * - replace=true 时：清空已有标签内容，仅使用当前数据包的内容
 * - 数据包优先级从低到高遍历
 *
 * 两阶段加载（与 EntityTypeTagLoader 一致）：
 * 1. 第一阶段：解析所有 JSON 文件，收集原始条目数据（不解析 # 引用），
 *    并将尚不存在的标签注册为空标签到 DamageTypeTags
 * 2. 第二阶段：按依赖顺序递归解析 # 标签引用并填充标签内容。
 *    使用 resolved/resolving 集合检测循环依赖。
 *
 * 参考: net.minecraft.tags.DamageTypeTags (MC 1.21.11)
 */
class DamageTypeTagLoader {
public:
    /**
     * @brief 从数据包仓库加载所有伤害类型标签
     *
     * 必须在 DamageTypeTags::initialize() 之后调用，以确保内置标签已注册。
     *
     * @param dataPackList 数据包仓库
     * @return 加载的标签数量
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有伤害类型标签
     *
     * @param pack 资源包
     * @return 加载的标签数量
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 字符串加载单个伤害类型标签
     *
     * @param json JSON 内容
     * @param location 标签资源位置
     * @return 解析的标签，或错误信息
     */
    [[nodiscard]] static Result<std::unique_ptr<DamageTypeTag>> loadFromJson(
        const std::string& json, const ResourceLocation& location);
};

} // namespace mc
