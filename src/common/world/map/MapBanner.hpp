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

#include "MapDecoration.hpp"
#include "core/Types.hpp"
#include "util/color/DyeColor.hpp"
#include "util/nbt/Nbt.hpp"
#include "world/block/BlockPos.hpp"
#include <memory>
#include <optional>
#include <string>

namespace mc::text {
class ITextComponent;
}

namespace mc {
class IWorld;
}

namespace mc::world::map {

/**
 * @brief 地图旗帜标记
 *
 * 记录旗帜在地图上的位置和颜色信息，用于在地图上显示旗帜图标。
 * 参考: net.minecraft.world.storage.MapBanner
 */
class MapBanner {
public:
    MapBanner() = default;
    MapBanner(BlockPos pos, DyeColor color, std::unique_ptr<text::ITextComponent> name);

    ~MapBanner() = default;

    MapBanner(const MapBanner& other);
    MapBanner& operator=(const MapBanner& other);
    MapBanner(MapBanner&&) noexcept = default;
    MapBanner& operator=(MapBanner&&) noexcept = default;

    /**
     * @brief 从NBT读取旗帜标记
     */
    static MapBanner fromNbt(const nbt::tags::compound_tag& tag);

    /**
     * @brief 写入NBT
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 获取对应的装饰类型
     */
    [[nodiscard]] DecorationType getDecorationType() const;

    /**
     * @brief 获取地图装饰ID（用于标识唯一旗帜）
     */
    [[nodiscard]] std::string getMapDecorationId() const;

    /**
     * @brief 从世界中的旗帜方块实体创建 MapBanner
     *
     * 检查指定位置是否存在 BannerEntity，如果存在则返回对应的 MapBanner。
     * 参考: net.minecraft.world.level.saveddata.maps.MapBanner.fromWorld
     *
     * @param world 世界引用
     * @param pos 旗帜方块位置
     * @return 如果存在旗帜则返回 MapBanner，否则返回 std::nullopt
     */
    [[nodiscard]] static std::optional<MapBanner> fromWorld(IWorld& world, const BlockPos& pos);

    /**
     * @brief 比较两个 MapBanner 是否相等
     *
     * 仅比较位置和颜色，不比较名称（与 MC Java 版一致）。
     * 参考: net.minecraft.world.level.saveddata.maps.MapBanner.equals
     */
    [[nodiscard]] bool equals(const MapBanner& other) const;

    [[nodiscard]] BlockPos pos() const { return m_pos; }
    [[nodiscard]] DyeColor color() const { return m_color; }
    [[nodiscard]] const text::ITextComponent* name() const { return m_name.get(); }

private:
    BlockPos m_pos;
    DyeColor m_color;
    std::unique_ptr<text::ITextComponent> m_name;
};

} // namespace mc::world::map
