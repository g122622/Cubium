#pragma once

#include "CropBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 小麦作物
 *
 * 8个生长阶段（AGE_0_7），成熟时掉落小麦和小麦种子。
 * 形状高度随年龄增长：2, 4, 6, 8, 10, 12, 14, 16 像素。
 *
 * 参考: net.minecraft.block.CropsBlock（小麦直接使用 CropsBlock）
 */
class WheatBlock : public CropBlock {
public:
    explicit WheatBlock(const BlockProperties& properties);
    ~WheatBlock() override = default;

    [[nodiscard]] u32 getCropItem() const override;
    [[nodiscard]] u32 getSeedItem() const override;
};

} // namespace blocks
} // namespace mc
