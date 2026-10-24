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
 * THE SOFTWARE IS PROVIDED "AS IS", KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "LootFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/map/MapDecoration.hpp"
#include <memory>
#include <optional>
#include <string_view>

namespace mc {
namespace loot {

/**
 * @brief 探险地图函数
 *
 * 生成一张已填充地图，在地图上标记最近的目标结构位置。
 * destination 与 decoration 均为地图数据包注册表项（与其它战利品函数中的
 * 物品/附魔/药水引用同一套注册表解析规则：命名空间可省略，默认 minecraft），
 * 取值合法性在加载战利品表时校验。
 *
 * JSON 字段（均可选）：
 * - destination: 结构标签（默认 "minecraft:on_treasure_maps"），按标签搜索最近结构
 * - decoration: 地图装饰类型（默认 "minecraft:woodland_mansion"），与 destination 无关
 * - zoom: 地图缩放级别（字节，默认 2，取值 -128..127）
 * - search_radius: 搜索半径（区块数，默认 50）
 * - skip_existing_chunks: 是否跳过已发现的结构（布尔，默认 true）
 */
class ExplorationMapFunction : public LootFunction {
public:
    /// 默认目标结构标签（对应 StructureTags.ON_TREASURE_MAPS）
    static const ResourceLocation& defaultDestination();
    /// 默认地图装饰类型
    static constexpr world::map::DecorationType DEFAULT_DECORATION = world::map::DecorationType::MANSION;
    /// 默认地图缩放级别（对应原版 Codec.BYTE 编码的缩放值）
    static constexpr i8 DEFAULT_ZOOM = 2;
    /// 默认搜索半径（区块数）
    static constexpr i32 DEFAULT_SEARCH_RADIUS = 50;

    /**
     * @brief 构造探险地图函数
     * @param destination 目标结构标签
     * @param decoration 装饰类型
     * @param zoom 地图缩放级别
     * @param searchRadius 搜索半径（区块数，非负）
     * @param searchSkipsLoadedChunks 是否跳过已加载（已发现）区块中的结构
     */
    explicit ExplorationMapFunction(const ResourceLocation& destination,
        world::map::DecorationType decoration,
        i8 zoom,
        i32 searchRadius,
        bool searchSkipsLoadedChunks);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const noexcept override
    {
        return "exploration_map";
    }

    [[nodiscard]] const ResourceLocation& getDestination() const
    {
        return m_destination;
    }

    [[nodiscard]] world::map::DecorationType getDecoration() const
    {
        return m_decoration;
    }

    [[nodiscard]] i8 getZoom() const
    {
        return m_zoom;
    }

    [[nodiscard]] i32 getSearchRadius() const
    {
        return m_searchRadius;
    }

    [[nodiscard]] bool shouldSearchSkipLoadedChunks() const
    {
        return m_searchSkipsLoadedChunks;
    }

    /**
     * @brief 解析结构标签资源位置
     *
     * 接受带命名空间前缀（"minecraft:on_treasure_maps"）与不带前缀（"on_treasure_maps"）
     * 两种写法；资源位置语法非法时返回 nullopt，标签是否存在由世界侧搜索时判定。
     *
     * @param str 资源位置字符串
     * @return 解析结果，无法解析时返回 nullopt
     */
    [[nodiscard]] static std::optional<ResourceLocation> parseDestination(std::string_view str);

private:
    ResourceLocation m_destination;
    world::map::DecorationType m_decoration;
    i8 m_zoom;
    i32 m_searchRadius;
    bool m_searchSkipsLoadedChunks;
};

} // namespace loot
} // namespace mc
