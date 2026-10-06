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

#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <cstddef>
#include <functional>
#include <string_view>
#include <vector>

namespace mc::resource {
class DataPackRepository;
class IResourcePack;
} // namespace mc::resource

namespace mc::resource::tag {

/**
 * @brief 成员解析失败哨兵值
 *
 * TagMemberResolver 对无法解析的成员 id（未知方块/流体/游戏事件，或 required=false
 * 的缺失引用）返回此值，通用加载器据此跳过该条目。
 */
inline constexpr std::size_t TAG_MEMBER_NOT_FOUND = static_cast<std::size_t>(-1);

/**
 * @brief 单个成员 id 解析回调
 *
 * 输入为标签 JSON 中一条非引用成员（如 "minecraft:stone"），返回成员在具体标签
 * 类型中的索引（如 BlockTag 的 ResourceLocation 序号、FluidTag 的流体序号、
 * GameEventTag 的 GameEvent 指针索引）。无法解析时返回 TAG_MEMBER_NOT_FOUND。
 *
 * 注意：id 已剥离 '#' 前缀（即调用方只会收到标签引用之外的字面成员名）。
 */
using TagMemberResolver = std::function<std::size_t(std::string_view memberId)>;

/**
 * @brief 读取已解析标签成员的回调
 *
 * 输入为被引用标签（#namespace:path）的资源位置，返回该标签当前的成员索引集合。
 * 若标签尚未解析/不存在，返回空集合。通用加载器保证被引用标签先于引用者解析。
 */
using TagMemberReader = std::function<std::vector<std::size_t>(const ResourceLocation& tagId)>;

/**
 * @brief 写入标签成员的回调
 *
 * 输入为标签资源位置、成员索引集合与 replace 标志。实现须保证标签存在
 * （不存在则创建）；replace=true 时先清空已有成员（含内置默认值）再写入，
 * 否则追加。通用加载器在第一阶段用空成员集合调用本回调注册占位标签。
 */
using TagFiller =
    std::function<void(const ResourceLocation& tagId, const std::vector<std::size_t>& members, bool replace)>;

/**
 * @brief 通用标签 JSON 加载器
 *
 * 抽取自 ItemTagLoader / EntityTypeTagLoader 的公共骨架，供成员类型各异
 * （方块、流体、游戏事件）但 JSON 结构完全一致的标签系统复用，避免重复实现。
 *
 * JSON 格式（与 vanilla 数据包标签文件一致）：
 * @code
 * {
 *   "replace": false,
 *   "values": [
 *     "minecraft:stone",
 *     "#minecraft:base_stone_overworld",
 *     {"id": "minecraft:bedrock", "required": false}
 *   ]
 * }
 * @endcode
 *
 * 加载路径: data/<namespace>/<tagDirectory>/（如 tags/block、tags/fluid、tags/game_event）。
 *
 * 多数据包合并语义（对齐 vanilla）：按数据包优先级从低到高遍历同名标签文件，
 * 默认追加，replace=true 时清空已有条目后追加。合并后若任一贡献包带 replace，
 * 则最终标签替换内置默认值（而非追加）。
 *
 * 两阶段加载：
 * 1. 第一阶段解析所有 JSON，收集原始条目并合并；随后为每个标签调用一次
 *    filler(tagId, {}, false) 注册空占位，确保 # 引用可解析到目标标签。
 * 2. 第二阶段按依赖顺序递归解析 # 标签引用（被引用标签先于引用者填充），
 *    使用 resolved/resolving 集合检测循环依赖。
 */
class GenericTagLoader {
public:
    GenericTagLoader() = delete;

    /**
     * @brief 从数据包仓库加载某类标签
     *
     * 遍历所有命名空间下的 <tagDirectory>/ 目录，加载所有标签 JSON 文件，
     * 经 resolveMember/readTag/fillTag 三个回调适配到具体标签类型。
     *
     * 必须在目标标签系统的内置 initialize() 之后调用，以确保内置默认值已注册
     * （数据包在默认值之上追加或替换）。
     *
     * @param dataPackList 数据包仓库
     * @param tagDirectory 标签目录（相对 data/<ns>/，如 "tags/block"）
     * @param resolveMember 成员 id 解析回调
     * @param readTag 已解析标签成员读取回调
     * @param fillTag 标签成员写入回调
     * @return 处理的标签数量，或错误信息
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList,
        std::string_view tagDirectory,
        const TagMemberResolver& resolveMember,
        const TagMemberReader& readTag,
        const TagFiller& fillTag);

    /**
     * @brief 从单个资源包加载某类标签
     *
     * 与 loadFromDataPackRepository() 不同，此方法仅加载单个资源包中的标签，
     * 不支持多数据包合并语义。适用于测试或单包加载场景。
     *
     * @param pack 资源包
     * @param tagDirectory 标签目录（相对 data/<ns>/）
     * @param resolveMember 成员 id 解析回调
     * @param readTag 已解析标签成员读取回调
     * @param fillTag 标签成员写入回调
     * @return 处理的标签数量，或错误信息
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack,
        std::string_view tagDirectory,
        const TagMemberResolver& resolveMember,
        const TagMemberReader& readTag,
        const TagFiller& fillTag);
};

} // namespace mc::resource::tag
