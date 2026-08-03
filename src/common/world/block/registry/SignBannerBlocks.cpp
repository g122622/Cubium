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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/blocks/HangingSignBlock.hpp"
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
Block* SignBannerBlocks::MANGROVE_SIGN = nullptr;
Block* SignBannerBlocks::MANGROVE_WALL_SIGN = nullptr;
Block* SignBannerBlocks::CHERRY_SIGN = nullptr;
Block* SignBannerBlocks::CHERRY_WALL_SIGN = nullptr;
Block* SignBannerBlocks::BAMBOO_SIGN = nullptr;
Block* SignBannerBlocks::BAMBOO_WALL_SIGN = nullptr;
Block* SignBannerBlocks::PALE_OAK_SIGN = nullptr;
Block* SignBannerBlocks::PALE_OAK_WALL_SIGN = nullptr;

// 悬挂告示牌（1.20 Trails & Tales 新增）
Block* SignBannerBlocks::OAK_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::OAK_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::SPRUCE_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::SPRUCE_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::BIRCH_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::BIRCH_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::JUNGLE_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::JUNGLE_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::ACACIA_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::ACACIA_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::DARK_OAK_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::DARK_OAK_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::CRIMSON_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::CRIMSON_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::WARPED_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::WARPED_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::MANGROVE_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::MANGROVE_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::CHERRY_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::CHERRY_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::BAMBOO_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::BAMBOO_WALL_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::PALE_OAK_HANGING_SIGN = nullptr;
Block* SignBannerBlocks::PALE_OAK_WALL_HANGING_SIGN = nullptr;

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
    SignBannerBlocks::OAK_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:oak_sign"), signProps, blocks::WoodType::Oak);
    SignBannerBlocks::OAK_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:oak_wall_sign"), signProps, blocks::WoodType::Oak);

    // 云杉木
    SignBannerBlocks::SPRUCE_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:spruce_sign"), signProps, blocks::WoodType::Spruce);
    SignBannerBlocks::SPRUCE_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:spruce_wall_sign"), signProps, blocks::WoodType::Spruce);

    // 白桦木
    SignBannerBlocks::BIRCH_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:birch_sign"), signProps, blocks::WoodType::Birch);
    SignBannerBlocks::BIRCH_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:birch_wall_sign"), signProps, blocks::WoodType::Birch);

    // 丛林木
    SignBannerBlocks::JUNGLE_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:jungle_sign"), signProps, blocks::WoodType::Jungle);
    SignBannerBlocks::JUNGLE_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:jungle_wall_sign"), signProps, blocks::WoodType::Jungle);

    // 金合欢木
    SignBannerBlocks::ACACIA_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:acacia_sign"), signProps, blocks::WoodType::Acacia);
    SignBannerBlocks::ACACIA_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:acacia_wall_sign"), signProps, blocks::WoodType::Acacia);

    // 深色橡木
    SignBannerBlocks::DARK_OAK_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:dark_oak_sign"), signProps, blocks::WoodType::DarkOak);
    SignBannerBlocks::DARK_OAK_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:dark_oak_wall_sign"), signProps, blocks::WoodType::DarkOak);

    // 绯红菌（下界木材）
    SignBannerBlocks::CRIMSON_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:crimson_sign"), signProps, blocks::WoodType::Crimson);
    SignBannerBlocks::CRIMSON_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:crimson_wall_sign"), signProps, blocks::WoodType::Crimson);

    // 诡异菌（下界木材）
    SignBannerBlocks::WARPED_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:warped_sign"), signProps, blocks::WoodType::Warped);
    SignBannerBlocks::WARPED_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:warped_wall_sign"), signProps, blocks::WoodType::Warped);

    // 红树木（1.19）
    SignBannerBlocks::MANGROVE_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:mangrove_sign"), signProps, blocks::WoodType::Mangrove);
    SignBannerBlocks::MANGROVE_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:mangrove_wall_sign"), signProps, blocks::WoodType::Mangrove);

    // 樱花木（1.20）
    SignBannerBlocks::CHERRY_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:cherry_sign"), signProps, blocks::WoodType::Cherry);
    SignBannerBlocks::CHERRY_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:cherry_wall_sign"), signProps, blocks::WoodType::Cherry);

    // 竹木（1.20）
    SignBannerBlocks::BAMBOO_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:bamboo_sign"), signProps, blocks::WoodType::Bamboo);
    SignBannerBlocks::BAMBOO_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:bamboo_wall_sign"), signProps, blocks::WoodType::Bamboo);

    // 苍白橡木（1.21.2）
    SignBannerBlocks::PALE_OAK_SIGN = &registry.registerBlock<blocks::StandingSignBlock>(
        ResourceLocation("minecraft:pale_oak_sign"), signProps, blocks::WoodType::PaleOak);
    SignBannerBlocks::PALE_OAK_WALL_SIGN = &registry.registerBlock<blocks::WallSignBlock>(
        ResourceLocation("minecraft:pale_oak_wall_sign"), signProps, blocks::WoodType::PaleOak);

    // ========== 悬挂告示牌注册（1.20 Trails & Tales）==========
    // 悬挂告示牌使用与普通告示牌相同的材质属性
    BlockProperties hangingSignProps = BlockProperties(Material::WOOD).hardness(1.0f).noCollision().notSolid();

    // 橡木悬挂告示牌
    SignBannerBlocks::OAK_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:oak_hanging_sign"), hangingSignProps, blocks::WoodType::Oak);
    SignBannerBlocks::OAK_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:oak_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Oak);

    // 云杉木悬挂告示牌
    SignBannerBlocks::SPRUCE_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:spruce_hanging_sign"), hangingSignProps, blocks::WoodType::Spruce);
    SignBannerBlocks::SPRUCE_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:spruce_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Spruce);

    // 白桦木悬挂告示牌
    SignBannerBlocks::BIRCH_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:birch_hanging_sign"), hangingSignProps, blocks::WoodType::Birch);
    SignBannerBlocks::BIRCH_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:birch_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Birch);

    // 丛林木悬挂告示牌
    SignBannerBlocks::JUNGLE_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:jungle_hanging_sign"), hangingSignProps, blocks::WoodType::Jungle);
    SignBannerBlocks::JUNGLE_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:jungle_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Jungle);

    // 金合欢木悬挂告示牌
    SignBannerBlocks::ACACIA_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:acacia_hanging_sign"), hangingSignProps, blocks::WoodType::Acacia);
    SignBannerBlocks::ACACIA_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:acacia_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Acacia);

    // 深色橡木悬挂告示牌
    SignBannerBlocks::DARK_OAK_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:dark_oak_hanging_sign"), hangingSignProps, blocks::WoodType::DarkOak);
    SignBannerBlocks::DARK_OAK_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:dark_oak_wall_hanging_sign"), hangingSignProps, blocks::WoodType::DarkOak);

    // 绯红菌悬挂告示牌（下界木材）
    SignBannerBlocks::CRIMSON_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:crimson_hanging_sign"), hangingSignProps, blocks::WoodType::Crimson);
    SignBannerBlocks::CRIMSON_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:crimson_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Crimson);

    // 诡异菌悬挂告示牌（下界木材）
    SignBannerBlocks::WARPED_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:warped_hanging_sign"), hangingSignProps, blocks::WoodType::Warped);
    SignBannerBlocks::WARPED_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:warped_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Warped);

    // 红树木悬挂告示牌（1.19）
    SignBannerBlocks::MANGROVE_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:mangrove_hanging_sign"), hangingSignProps, blocks::WoodType::Mangrove);
    SignBannerBlocks::MANGROVE_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:mangrove_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Mangrove);

    // 樱花木悬挂告示牌（1.20）
    SignBannerBlocks::CHERRY_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:cherry_hanging_sign"), hangingSignProps, blocks::WoodType::Cherry);
    SignBannerBlocks::CHERRY_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:cherry_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Cherry);

    // 竹木悬挂告示牌（1.20）
    SignBannerBlocks::BAMBOO_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:bamboo_hanging_sign"), hangingSignProps, blocks::WoodType::Bamboo);
    SignBannerBlocks::BAMBOO_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:bamboo_wall_hanging_sign"), hangingSignProps, blocks::WoodType::Bamboo);

    // 苍白橡木悬挂告示牌（1.21.2）
    SignBannerBlocks::PALE_OAK_HANGING_SIGN = &registry.registerBlock<blocks::CeilingHangingSignBlock>(
        ResourceLocation("minecraft:pale_oak_hanging_sign"), hangingSignProps, blocks::WoodType::PaleOak);
    SignBannerBlocks::PALE_OAK_WALL_HANGING_SIGN = &registry.registerBlock<blocks::WallHangingSignBlock>(
        ResourceLocation("minecraft:pale_oak_wall_hanging_sign"), hangingSignProps, blocks::WoodType::PaleOak);

    // ========== 旗帜注册 ==========
    BlockProperties bannerProps =
        BlockProperties(Material::WOOD).hardness(1.0f).resistance(1.0f).notSolid().soundType(BlockSoundTypes::WOOD);

    // 白色旗帜
    SignBannerBlocks::WHITE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:white_banner"), bannerProps, DyeColor::White);
    SignBannerBlocks::WHITE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:white_wall_banner"), bannerProps, DyeColor::White);

    // 橙色旗帜
    SignBannerBlocks::ORANGE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:orange_banner"), bannerProps, DyeColor::Orange);
    SignBannerBlocks::ORANGE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:orange_wall_banner"), bannerProps, DyeColor::Orange);

    // 品红色旗帜
    SignBannerBlocks::MAGENTA_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:magenta_banner"), bannerProps, DyeColor::Magenta);
    SignBannerBlocks::MAGENTA_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:magenta_wall_banner"), bannerProps, DyeColor::Magenta);

    // 淡蓝色旗帜
    SignBannerBlocks::LIGHT_BLUE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:light_blue_banner"), bannerProps, DyeColor::LightBlue);
    SignBannerBlocks::LIGHT_BLUE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:light_blue_wall_banner"), bannerProps, DyeColor::LightBlue);

    // 黄色旗帜
    SignBannerBlocks::YELLOW_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:yellow_banner"), bannerProps, DyeColor::Yellow);
    SignBannerBlocks::YELLOW_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:yellow_wall_banner"), bannerProps, DyeColor::Yellow);

    // 黄绿色旗帜
    SignBannerBlocks::LIME_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:lime_banner"), bannerProps, DyeColor::Lime);
    SignBannerBlocks::LIME_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:lime_wall_banner"), bannerProps, DyeColor::Lime);

    // 粉红色旗帜
    SignBannerBlocks::PINK_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:pink_banner"), bannerProps, DyeColor::Pink);
    SignBannerBlocks::PINK_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:pink_wall_banner"), bannerProps, DyeColor::Pink);

    // 灰色旗帜
    SignBannerBlocks::GRAY_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:gray_banner"), bannerProps, DyeColor::Gray);
    SignBannerBlocks::GRAY_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:gray_wall_banner"), bannerProps, DyeColor::Gray);

    // 淡灰色旗帜
    SignBannerBlocks::LIGHT_GRAY_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:light_gray_banner"), bannerProps, DyeColor::LightGray);
    SignBannerBlocks::LIGHT_GRAY_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:light_gray_wall_banner"), bannerProps, DyeColor::LightGray);

    // 青色旗帜
    SignBannerBlocks::CYAN_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:cyan_banner"), bannerProps, DyeColor::Cyan);
    SignBannerBlocks::CYAN_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:cyan_wall_banner"), bannerProps, DyeColor::Cyan);

    // 紫色旗帜
    SignBannerBlocks::PURPLE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:purple_banner"), bannerProps, DyeColor::Purple);
    SignBannerBlocks::PURPLE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:purple_wall_banner"), bannerProps, DyeColor::Purple);

    // 蓝色旗帜
    SignBannerBlocks::BLUE_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:blue_banner"), bannerProps, DyeColor::Blue);
    SignBannerBlocks::BLUE_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:blue_wall_banner"), bannerProps, DyeColor::Blue);

    // 棕色旗帜
    SignBannerBlocks::BROWN_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:brown_banner"), bannerProps, DyeColor::Brown);
    SignBannerBlocks::BROWN_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:brown_wall_banner"), bannerProps, DyeColor::Brown);

    // 绿色旗帜
    SignBannerBlocks::GREEN_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:green_banner"), bannerProps, DyeColor::Green);
    SignBannerBlocks::GREEN_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:green_wall_banner"), bannerProps, DyeColor::Green);

    // 红色旗帜
    SignBannerBlocks::RED_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:red_banner"), bannerProps, DyeColor::Red);
    SignBannerBlocks::RED_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:red_wall_banner"), bannerProps, DyeColor::Red);

    // 黑色旗帜
    SignBannerBlocks::BLACK_BANNER = &registry.registerBlock<blocks::StandingBannerBlock>(
        ResourceLocation("minecraft:black_banner"), bannerProps, DyeColor::Black);
    SignBannerBlocks::BLACK_WALL_BANNER = &registry.registerBlock<blocks::WallBannerBlock>(
        ResourceLocation("minecraft:black_wall_banner"), bannerProps, DyeColor::Black);
}

} // namespace block_registry
} // namespace mc
