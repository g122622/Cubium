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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include <string>

namespace mc::world::gen::feature::cave {

/**
 * @brief 洞穴表面方向枚举
 *
 * 指示植被贴片生成在洞穴的地面还是天花板。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.CaveSurface
 */
enum class CaveSurface : u8 {
    /// 地面：植被放置在地面方块上方
    Floor,
    /// 天花板：植被悬挂在天花板方块下方
    Ceiling
};

/**
 * @brief 获取洞穴表面的扫描方向
 * @param surface 洞穴表面类型
 * @return 扫描方向（Floor→Down, Ceiling→Up）
 */
[[nodiscard]] inline Direction getScanDirection(CaveSurface surface)
{
    return surface == CaveSurface::Floor ? Direction::Down : Direction::Up;
}

/**
 * @brief 获取植被放置的Y偏移
 * @param surface 洞穴表面类型
 * @return Y偏移（Floor→+1, Ceiling→-1）
 */
[[nodiscard]] inline i32 getVegetationYOffset(CaveSurface surface)
{
    return surface == CaveSurface::Floor ? 1 : -1;
}

/**
 * @brief 检查方块是否匹配标签
 */
[[nodiscard]] inline bool matchesTag(const BlockState& state, const std::string& tagName)
{
    auto* tag = mc::BlockTags::getTag(mc::ResourceLocation(tagName));
    return tag != nullptr && tag->contains(state);
}

} // namespace mc::world::gen::feature::cave
