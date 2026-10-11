#pragma once

#include "common/core/Types.hpp"
#include <algorithm>
#include <array>
#include <iterator>
#include <optional>
#include <string_view>

namespace mc::network::backend::java {

/** @brief 旗帜图案的配置注册序、组件编码和标签共用的映射。 */
class JavaBannerPatternIds {
public:
    inline static constexpr std::array<std::string_view, 43> ENTRIES{"minecraft:base",
        "minecraft:border",
        "minecraft:bricks",
        "minecraft:circle",
        "minecraft:creeper",
        "minecraft:cross",
        "minecraft:curly_border",
        "minecraft:diagonal_left",
        "minecraft:diagonal_right",
        "minecraft:diagonal_up_left",
        "minecraft:diagonal_up_right",
        "minecraft:flow",
        "minecraft:flower",
        "minecraft:globe",
        "minecraft:gradient",
        "minecraft:gradient_up",
        "minecraft:guster",
        "minecraft:half_horizontal",
        "minecraft:half_horizontal_bottom",
        "minecraft:half_vertical",
        "minecraft:half_vertical_right",
        "minecraft:mojang",
        "minecraft:piglin",
        "minecraft:rhombus",
        "minecraft:skull",
        "minecraft:small_stripes",
        "minecraft:square_bottom_left",
        "minecraft:square_bottom_right",
        "minecraft:square_top_left",
        "minecraft:square_top_right",
        "minecraft:straight_cross",
        "minecraft:stripe_bottom",
        "minecraft:stripe_center",
        "minecraft:stripe_downleft",
        "minecraft:stripe_downright",
        "minecraft:stripe_left",
        "minecraft:stripe_middle",
        "minecraft:stripe_right",
        "minecraft:stripe_top",
        "minecraft:triangle_bottom",
        "minecraft:triangle_top",
        "minecraft:triangles_bottom",
        "minecraft:triangles_top"};
    /** @brief 根据资源名查找同步注册 ID。 */
    [[nodiscard]] static std::optional<i32> toWireId(std::string_view name)
    {
        const auto it = std::find(ENTRIES.begin(), ENTRIES.end(), name);
        return it == ENTRIES.end() ? std::nullopt
                                   : std::optional<i32>(static_cast<i32>(std::distance(ENTRIES.begin(), it)));
    }
};

} // namespace mc::network::backend::java
