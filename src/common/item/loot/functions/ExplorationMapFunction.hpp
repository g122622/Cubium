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

#include "LootFunction.hpp"
#include "common/core/Types.hpp"
#include "common/world/map/MapDecoration.hpp"

namespace mc {
namespace loot {

/**
 * @brief 探险地图函数
 *
 * 生成探险地图，在地图上标记最近的目标结构位置。
 * 参考: net.minecraft.loot.functions.ExplorationMap
 *
 * 支持的目标结构类型：
 * - BuriedTreasure: 埋藏的宝藏（红色X标记）
 * - Mansion: 林地府邸（林地府邸标记）
 * - Monument: 海底神殿（海底神殿标记）
 * - Shipwreck: 沉船
 * - RuinedPortal: 废弃传送门
 *
 * 配置选项：
 * - destination: 目标结构类型
 * - zoom: 地图缩放级别（默认2）
 * - skipKnownStructures: 是否跳过已发现的结构（默认true）
 */
class ExplorationMapFunction : public LootFunction {
public:
    /**
     * @brief 地图目的地类型
     */
    enum class Destination : u8 {
        BuriedTreasure, // 埋藏的宝藏
        Mansion,        // 林地府邸
        Monument,       // 海底神殿
        Shipwreck,      // 沉船
        RuinedPortal    // 废弃传送门
    };

    /**
     * @brief 构造探险地图函数
     * @param destination 目的地类型
     * @param zoom 地图缩放级别（默认2）
     * @param skipKnownStructures 是否跳过已发现的结构（默认true）
     */
    explicit ExplorationMapFunction(
        Destination destination = Destination::BuriedTreasure, i32 zoom = 2, bool skipKnownStructures = true);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] std::string getType() const override { return "exploration_map"; }

    [[nodiscard]] Destination getDestination() const { return m_destination; }
    [[nodiscard]] i32 getZoom() const { return m_zoom; }
    [[nodiscard]] bool shouldSkipKnownStructures() const { return m_skipKnownStructures; }

    /**
     * @brief 将目的地类型转换为装饰类型
     */
    [[nodiscard]] static world::map::DecorationType destinationToDecorationType(Destination destination);

private:
    Destination m_destination;
    i32 m_zoom;
    bool m_skipKnownStructures;
};

} // namespace loot
} // namespace mc
