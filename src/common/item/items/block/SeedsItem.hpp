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

#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/world/block/Block.hpp"

namespace mc {
namespace item {
namespace items {

/**
 * @brief 种子物品类
 *
 * 继承自 BlockItem，用于将种子物品与对应的作物方块关联。
 * 右键耕地时，通过 BlockItem 的放置逻辑自动种植作物方块。
 * 作物方块的 isValidPosition() 会检查下方是否为耕地以及光照条件，
 * 因此种子的种植限制由作物方块自身处理，无需在物品侧额外判断。
 *
 * 种子与作物方块的映射关系：
 *   WHEAT_SEEDS     -> WheatBlock (minecraft:wheat)
 *   PUMPKIN_SEEDS   -> PumpkinStemBlock (minecraft:pumpkin_stem)
 *   MELON_SEEDS     -> MelonStemBlock (minecraft:melon_stem)
 *   BEETROOT_SEEDS  -> BeetrootBlock (minecraft:beetroots)
 *   TORCHFLOWER_SEEDS -> TorchflowerCropBlock (minecraft:torchflower_crop)
 *   PITCHER_POD     -> PitcherCropBlock (minecraft:pitcher_crop)
 *
 * 在 MC Java 1.21+ 中，种子物品直接使用 BlockItem（不再有 SeedsItem 子类），
 * 但本项目保留此子类以提供更清晰的语义区分和未来扩展点。
 *
 * 参考: net.minecraft.world.item.BlockItem（MC 1.21+ 中种子直接注册为 BlockItem 实例）
 */
class SeedsItem : public BlockItem {
public:
    /**
     * @brief 构造种子物品
     * @param cropBlock 种子对应的作物方块
     * @param properties 物品属性
     */
    SeedsItem(const Block& cropBlock, ItemProperties properties);

    ~SeedsItem() override = default;
};

} // namespace items
} // namespace item
} // namespace mc
