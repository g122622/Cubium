#include "WheatBlock.hpp"
#include "../../../../item/Items.hpp"

namespace mc {
namespace blocks {

WheatBlock::WheatBlock(const BlockProperties& properties)
    : CropBlock(properties) {
    // 小麦使用 CropBlock 的默认形状
}

u32 WheatBlock::getCropItem() const {
    // 返回小麦物品ID
    // 参考: net.minecraft.block.CropsBlock#getCropItem
    return Items::WHEAT->itemId();
}

u32 WheatBlock::getSeedItem() const {
    // 返回小麦种子物品ID
    return Items::WHEAT_SEEDS->itemId();
}

} // namespace blocks
} // namespace mc
