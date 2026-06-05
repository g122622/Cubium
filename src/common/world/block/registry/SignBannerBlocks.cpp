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

#include "world/block/registry/SignBannerBlocks.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/blocks/SignBlock.hpp"
#include "world/block/blocks/decorative/BannerBlock.hpp"

namespace mc {
namespace block_registry {

// 告示牌（含水）
Block* SignBannerBlocks::OAK_SIGN = nullptr;
Block* SignBannerBlocks::OAK_WALL_SIGN = nullptr;
Block* SignBannerBlocks::SPRUCE_SIGN = nullptr;
Block* SignBannerBlocks::SPRUCE_WALL_SIGN = nullptr;
Block* SignBannerBlocks::BIRCH_SIGN = nullptr;
Block* SignBannerBlocks::BIRCH_WALL_SIGN = nullptr;
Block* SignBannerBlocks::JUNGLE_SIGN = nullptr;
Block* SignBannerBlocks::JUNGLE_WALL_SIGN = nullptr;
Block* SignBannerBlocks::ACACIA_SIGN = nullptr;
Block* SignBannerBlocks::ACACIA_WALL_SIGN = nullptr;
Block* SignBannerBlocks::DARK_OAK_SIGN = nullptr;
Block* SignBannerBlocks::DARK_OAK_WALL_SIGN = nullptr;
Block* SignBannerBlocks::CRIMSON_SIGN = nullptr;
Block* SignBannerBlocks::CRIMSON_WALL_SIGN = nullptr;
Block* SignBannerBlocks::WARPED_SIGN = nullptr;
Block* SignBannerBlocks::WARPED_WALL_SIGN = nullptr;

// 旗帜 (16色 × 2形态 = 32方块)
Block* SignBannerBlocks::WHITE_BANNER = nullptr;
Block* SignBannerBlocks::WHITE_WALL_BANNER = nullptr;
Block* SignBannerBlocks::ORANGE_BANNER = nullptr;
Block* SignBannerBlocks::ORANGE_WALL_BANNER = nullptr;
Block* SignBannerBlocks::MAGENTA_BANNER = nullptr;
Block* SignBannerBlocks::MAGENTA_WALL_BANNER = nullptr;
Block* SignBannerBlocks::LIGHT_BLUE_BANNER = nullptr;
Block* SignBannerBlocks::LIGHT_BLUE_WALL_BANNER = nullptr;
Block* SignBannerBlocks::YELLOW_BANNER = nullptr;
Block* SignBannerBlocks::YELLOW_WALL_BANNER = nullptr;
Block* SignBannerBlocks::LIME_BANNER = nullptr;
Block* SignBannerBlocks::LIME_WALL_BANNER = nullptr;
Block* SignBannerBlocks::PINK_BANNER = nullptr;
Block* SignBannerBlocks::PINK_WALL_BANNER = nullptr;
Block* SignBannerBlocks::GRAY_BANNER = nullptr;
Block* SignBannerBlocks::GRAY_WALL_BANNER = nullptr;
Block* SignBannerBlocks::LIGHT_GRAY_BANNER = nullptr;
Block* SignBannerBlocks::LIGHT_GRAY_WALL_BANNER = nullptr;
Block* SignBannerBlocks::CYAN_BANNER = nullptr;
Block* SignBannerBlocks::CYAN_WALL_BANNER = nullptr;
Block* SignBannerBlocks::PURPLE_BANNER = nullptr;
Block* SignBannerBlocks::PURPLE_WALL_BANNER = nullptr;
Block* SignBannerBlocks::BLUE_BANNER = nullptr;
Block* SignBannerBlocks::BLUE_WALL_BANNER = nullptr;
Block* SignBannerBlocks::BROWN_BANNER = nullptr;
Block* SignBannerBlocks::BROWN_WALL_BANNER = nullptr;
Block* SignBannerBlocks::GREEN_BANNER = nullptr;
Block* SignBannerBlocks::GREEN_WALL_BANNER = nullptr;
Block* SignBannerBlocks::RED_BANNER = nullptr;
Block* SignBannerBlocks::RED_WALL_BANNER = nullptr;
Block* SignBannerBlocks::BLACK_BANNER = nullptr;
Block* SignBannerBlocks::BLACK_WALL_BANNER = nullptr;

void registerSignBannerBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ========== 告示牌注册 ==========
    BlockProperties signProps = BlockProperties(Material::WOOD).hardness(1.0f).noCollision().notSolid();

    // 注册各木材类型的告示牌
    // 橡木
    OAK_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:oak_sign"), signProps, blocks::WoodType::Oak);
    OAK_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:oak_wall_sign"), signProps, blocks::WoodType::Oak);

    // 云杉木
    SPRUCE_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:spruce_sign"), signProps, blocks::WoodType::Spruce);
    SPRUCE_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:spruce_wall_sign"), signProps, blocks::WoodType::Spruce);

    // 白桦木
    BIRCH_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:birch_sign"), signProps, blocks::WoodType::Birch);
    BIRCH_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:birch_wall_sign"), signProps, blocks::WoodType::Birch);

    // 丛林木
    JUNGLE_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:jungle_sign"), signProps, blocks::WoodType::Jungle);
    JUNGLE_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:jungle_wall_sign"), signProps, blocks::WoodType::Jungle);

    // 金合欢木
    ACACIA_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:acacia_sign"), signProps, blocks::WoodType::Acacia);
    ACACIA_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:acacia_wall_sign"), signProps, blocks::WoodType::Acacia);

    // 深色橡木
    DARK_OAK_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:dark_oak_sign"), signProps, blocks::WoodType::DarkOak);
    DARK_OAK_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:dark_oak_wall_sign"), signProps, blocks::WoodType::DarkOak);

    // 绯红菌（下界木材）
    CRIMSON_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:crimson_sign"), signProps, blocks::WoodType::Crimson);
    CRIMSON_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:crimson_wall_sign"), signProps, blocks::WoodType::Crimson);

    // 诡异菌（下界木材）
    WARPED_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:warped_sign"), signProps, blocks::WoodType::Warped);
    WARPED_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:warped_wall_sign"), signProps, blocks::WoodType::Warped);

    // ========== 旗帜注册 ==========
    BlockProperties bannerProps =
        BlockProperties(Material::WOOD).hardness(1.0f).resistance(1.0f).notSolid().soundType(BlockSoundTypes::WOOD);

    // 白色旗帜
    WHITE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:white_banner"), bannerProps, DyeColor::White);
    WHITE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:white_wall_banner"), bannerProps, DyeColor::White);

    // 橙色旗帜
    ORANGE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:orange_banner"), bannerProps, DyeColor::Orange);
    ORANGE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:orange_wall_banner"), bannerProps, DyeColor::Orange);

    // 品红色旗帜
    MAGENTA_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:magenta_banner"), bannerProps, DyeColor::Magenta);
    MAGENTA_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:magenta_wall_banner"), bannerProps, DyeColor::Magenta);

    // 淡蓝色旗帜
    LIGHT_BLUE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:light_blue_banner"), bannerProps, DyeColor::LightBlue);
    LIGHT_BLUE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:light_blue_wall_banner"), bannerProps, DyeColor::LightBlue);

    // 黄色旗帜
    YELLOW_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:yellow_banner"), bannerProps, DyeColor::Yellow);
    YELLOW_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:yellow_wall_banner"), bannerProps, DyeColor::Yellow);

    // 黄绿色旗帜
    LIME_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:lime_banner"), bannerProps, DyeColor::Lime);
    LIME_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:lime_wall_banner"), bannerProps, DyeColor::Lime);

    // 粉红色旗帜
    PINK_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:pink_banner"), bannerProps, DyeColor::Pink);
    PINK_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:pink_wall_banner"), bannerProps, DyeColor::Pink);

    // 灰色旗帜
    GRAY_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:gray_banner"), bannerProps, DyeColor::Gray);
    GRAY_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:gray_wall_banner"), bannerProps, DyeColor::Gray);

    // 淡灰色旗帜
    LIGHT_GRAY_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:light_gray_banner"), bannerProps, DyeColor::LightGray);
    LIGHT_GRAY_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:light_gray_wall_banner"), bannerProps, DyeColor::LightGray);

    // 青色旗帜
    CYAN_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:cyan_banner"), bannerProps, DyeColor::Cyan);
    CYAN_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:cyan_wall_banner"), bannerProps, DyeColor::Cyan);

    // 紫色旗帜
    PURPLE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:purple_banner"), bannerProps, DyeColor::Purple);
    PURPLE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:purple_wall_banner"), bannerProps, DyeColor::Purple);

    // 蓝色旗帜
    BLUE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:blue_banner"), bannerProps, DyeColor::Blue);
    BLUE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:blue_wall_banner"), bannerProps, DyeColor::Blue);

    // 棕色旗帜
    BROWN_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:brown_banner"), bannerProps, DyeColor::Brown);
    BROWN_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:brown_wall_banner"), bannerProps, DyeColor::Brown);

    // 绿色旗帜
    GREEN_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:green_banner"), bannerProps, DyeColor::Green);
    GREEN_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:green_wall_banner"), bannerProps, DyeColor::Green);

    // 红色旗帜
    RED_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:red_banner"), bannerProps, DyeColor::Red);
    RED_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:red_wall_banner"), bannerProps, DyeColor::Red);

    // 黑色旗帜
    BLACK_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:black_banner"), bannerProps, DyeColor::Black);
    BLACK_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:black_wall_banner"), bannerProps, DyeColor::Black);
}

} // namespace block_registry
} // namespace mc
