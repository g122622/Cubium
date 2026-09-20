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
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 探险地图函数
 *
 * 生成探险地图，在地图上标记最近的目标结构位置。
 * 对应原版 net.minecraft.world.level.storage.loot.functions.ExplorationMapFunction。
 *
 * JSON 字段（均可选）：
 * - destination: 目标结构类型（字符串，如 "minecraft:buried_treasure"）
 * - decoration: 地图装饰类型（字符串，如 "minecraft:red_x"）
 * - zoom: 地图缩放级别（整数，默认2）
 * - search_radius: 搜索半径（区块数，默认50）
 * - skip_existing_chunks: 是否跳过已发现的结构（布尔，默认true）
 *
 * 当同时指定 destination 和 decoration 时，decoration 优先；
 * 当仅指定 destination 时，自动推导对应的装饰类型。
 * 当两者都未指定时，默认 destination 为 BuriedTreasure，默认 decoration 为 RED_X。
 *
 * TODO: 与原版存在以下语义差异，待后续对齐：
 *  1. 原版 destination 是结构标签（默认 StructureTags.ON_TREASURE_MAPS = minecraft:on_treasure_maps），
 *     按标签搜索结构；本项目是 5 个具体结构的枚举，按单个结构 ID 搜索。
 *  2. 原版 decoration 是地图装饰注册表项（MapDecorationType，默认 MapDecorationTypes.WOODLAND_MANSION），
 *     且默认值与 destination 无关；本项目是枚举 DecorationType，缺省时由 destination 推导。
 *  3. 原版 destination / decoration 取值非法时解析失败；本项目回退到默认值。
 *  4. 原版 zoom 是 Codec.BYTE（超出 -128..127 即解析失败）；本项目按 i32 接收且不做范围校验。
 */
class ExplorationMapFunction : public LootFunction {
public:
    /**
     * @brief 地图目的地类型
     *
     * 每个枚举值对应一个结构资源位置字符串（如 minecraft:buried_treasure），
     * 用于 destinationToResourceLocation 搜索结构。
     */
    enum class Destination : u8 {
        BuriedTreasure, // minecraft:buried_treasure
        Mansion,        // minecraft:mansion
        Monument,       // minecraft:monument
        Shipwreck,      // minecraft:shipwreck
        RuinedPortal    // minecraft:ruined_portal
    };

    /**
     * @brief 构造探险地图函数
     * @param destination 目的地类型
     * @param decoration 装饰类型（nullopt 表示从 destination 自动推导）
     * @param zoom 地图缩放级别
     * @param searchRadius 搜索半径（区块数）
     * @param skipKnownStructures 是否跳过已发现的结构
     */
    explicit ExplorationMapFunction(Destination destination,
        std::optional<world::map::DecorationType> decoration,
        i32 zoom,
        i32 searchRadius,
        bool skipKnownStructures);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "exploration_map"; }

    [[nodiscard]] Destination getDestination() const { return m_destination; }
    [[nodiscard]] const std::optional<world::map::DecorationType>& getDecoration() const { return m_decoration; }
    [[nodiscard]] i32 getZoom() const { return m_zoom; }
    [[nodiscard]] i32 getSearchRadius() const { return m_searchRadius; }
    [[nodiscard]] bool shouldSkipKnownStructures() const { return m_skipKnownStructures; }

    /**
     * @brief 将目的地类型转换为装饰类型
     */
    [[nodiscard]] static world::map::DecorationType destinationToDecorationType(Destination destination);

    /**
     * @brief 将目的地字符串转换为 Destination 枚举
     * @param str 目的地字符串（如 "minecraft:buried_treasure" 或 "buried_treasure"）
     * @return 对应的 Destination 枚举，无法识别时返回 nullopt
     */
    [[nodiscard]] static std::optional<Destination> destinationFromString(const std::string& str);

    /**
     * @brief 将 Destination 枚举转换为字符串
     */
    [[nodiscard]] static const char* destinationToString(Destination dest);

    /**
     * @brief 获取生效的装饰类型
     *
     * 如果显式指定了 decoration 则使用它，否则从 destination 推导。
     */
    [[nodiscard]] world::map::DecorationType getEffectiveDecoration() const;

    /**
     * @brief 将目的地类型转换为结构资源位置（用于结构搜索）
     */
    [[nodiscard]] static ResourceLocation destinationToResourceLocation(Destination destination);

private:
    Destination m_destination;
    std::optional<world::map::DecorationType> m_decoration;
    i32 m_zoom;
    i32 m_searchRadius;
    bool m_skipKnownStructures;
};

} // namespace loot
} // namespace mc
