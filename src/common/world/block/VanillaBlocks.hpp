#pragma once

#include "Block.hpp"
#include "BlockRegistry.hpp"
#include "blocks/AirBlock.hpp"
#include "blocks/SimpleBlock.hpp"
#include "blocks/RotatedPillarBlock.hpp"

namespace mc {

/**
 * @brief 原版方块静态引用
 *
 * 提供所有原版方块的静态指针，便于快速访问。
 * 在游戏初始化时调用 VanillaBlocks::initialize() 进行注册。
 *
 * 参考: net.minecraft.block.Blocks
 */
class VanillaBlocks {
public:
    /**
     * @brief 初始化所有原版方块
     *
     * 必须在使用任何方块前调用。
     */
    static void initialize();

    // ========================================================================
    // 基础方块
    // ========================================================================
    static Block* AIR;
    static Block* STONE;
    static Block* GRASS_BLOCK;
    static Block* DIRT;
    static Block* COBBLESTONE;
    static Block* OAK_PLANKS;
    static Block* WATER;
    static Block* LAVA;
    static Block* BEDROCK;
    static Block* SAND;
    static Block* GRAVEL;

    // ========================================================================
    // 石头变种
    // ========================================================================
    static Block* GRANITE;
    static Block* POLISHED_GRANITE;
    static Block* DIORITE;
    static Block* POLISHED_DIORITE;
    static Block* ANDESITE;
    static Block* POLISHED_ANDESITE;

    // ========================================================================
    // 泥土变种
    // ========================================================================
    static Block* COARSE_DIRT;
    static Block* PODZOL;

    // ========================================================================
    // 砂岩系列
    // ========================================================================
    static Block* SANDSTONE;
    static Block* CHISELED_SANDSTONE;
    static Block* CUT_SANDSTONE;
    static Block* RED_SANDSTONE;

    // ========================================================================
    // 矿石方块
    // ========================================================================
    static Block* GOLD_ORE;
    static Block* IRON_ORE;
    static Block* COAL_ORE;
    static Block* DIAMOND_ORE;
    static Block* DIAMOND_BLOCK;
    static Block* EMERALD_ORE;
    static Block* LAPIS_ORE;
    static Block* REDSTONE_ORE;
    static Block* COPPER_ORE;           // 铜矿 (1.17+)

    // ========================================================================
    // 下界矿石
    // ========================================================================
    static Block* NETHER_QUARTZ_ORE;    // 下界石英矿
    static Block* NETHER_GOLD_ORE;      // 下界金矿
    static Block* ANCIENT_DEBRIS;       // 远古残骸

    // ========================================================================
    // 矿物方块
    // ========================================================================
    static Block* GOLD_BLOCK;
    static Block* IRON_BLOCK;
    static Block* LAPIS_BLOCK;
    static Block* EMERALD_BLOCK;
    static Block* REDSTONE_BLOCK;

    // ========================================================================
    // 建筑方块
    // ========================================================================
    static Block* BRICKS;
    static Block* MOSSY_COBBLESTONE;
    static Block* BOOKSHELF;
    static Block* TNT;
    static Block* SPONGE;
    static Block* WET_SPONGE;

    // ========================================================================
    // 功能方块
    // ========================================================================
    static Block* CRAFTING_TABLE;
    static Block* CAULDRON;
    static Block* ENCHANTING_TABLE;

    // ========================================================================
    // 门和栅栏门
    // ========================================================================
    static Block* OAK_DOOR;
    static Block* IRON_DOOR;
    static Block* OAK_FENCE_GATE;

    // ========================================================================
    // 羊毛 (16色)
    // ========================================================================
    static Block* WHITE_WOOL;
    static Block* ORANGE_WOOL;
    static Block* MAGENTA_WOOL;
    static Block* LIGHT_BLUE_WOOL;
    static Block* YELLOW_WOOL;
    static Block* LIME_WOOL;
    static Block* PINK_WOOL;
    static Block* GRAY_WOOL;
    static Block* LIGHT_GRAY_WOOL;
    static Block* CYAN_WOOL;
    static Block* PURPLE_WOOL;
    static Block* BLUE_WOOL;
    static Block* BROWN_WOOL;
    static Block* GREEN_WOOL;
    static Block* RED_WOOL;
    static Block* BLACK_WOOL;

    // ========================================================================
    // 木板变种
    // ========================================================================
    static Block* SPRUCE_PLANKS;
    static Block* BIRCH_PLANKS;
    static Block* JUNGLE_PLANKS;
    static Block* ACACIA_PLANKS;
    static Block* DARK_OAK_PLANKS;

    // ========================================================================
    // 原木和树叶
    // ========================================================================
    static Block* OAK_LOG;
    static Block* OAK_LEAVES;
    static Block* SPRUCE_LOG;
    static Block* BIRCH_LOG;
    static Block* JUNGLE_LOG;
    static Block* ACACIA_LOG;
    static Block* DARK_OAK_LOG;
    static Block* SPRUCE_LEAVES;
    static Block* BIRCH_LEAVES;
    static Block* JUNGLE_LEAVES;
    static Block* ACACIA_LEAVES;
    static Block* DARK_OAK_LEAVES;

    // ========================================================================
    // 植被方块
    // ========================================================================
    static Block* SHORT_GRASS;
    static Block* TALL_GRASS;
    static Block* FERN;
    static Block* DANDELION;
    static Block* POPPY;
    static Block* BLUE_ORCHID;
    static Block* ALLIUM;
    static Block* AZURE_BLUET;
    static Block* RED_TULIP;
    static Block* ORANGE_TULIP;
    static Block* WHITE_TULIP;
    static Block* PINK_TULIP;
    static Block* OXEYE_DAISY;
    static Block* BROWN_MUSHROOM;
    static Block* RED_MUSHROOM;

    // ========================================================================
    // 树苗
    // ========================================================================
    static Block* OAK_SAPLING;
    static Block* SPRUCE_SAPLING;
    static Block* BIRCH_SAPLING;
    static Block* JUNGLE_SAPLING;
    static Block* ACACIA_SAPLING;
    static Block* DARK_OAK_SAPLING;

    // ========================================================================
    // 石砖系列
    // ========================================================================
    static Block* STONE_BRICKS;
    static Block* MOSSY_STONE_BRICKS;
    static Block* CRACKED_STONE_BRICKS;
    static Block* CHISELED_STONE_BRICKS;

    // ========================================================================
    // 石英系列
    // ========================================================================
    static Block* QUARTZ_BLOCK;
    static Block* CHISELED_QUARTZ_BLOCK;
    static Block* QUARTZ_PILLAR;
    static Block* QUARTZ_ORE;

    // ========================================================================
    // 海晶系列
    // ========================================================================
    static Block* PRISMARINE;
    static Block* PRISMARINE_BRICKS;
    static Block* DARK_PRISMARINE;
    static Block* SEA_LANTERN;

    // ========================================================================
    // 紫珀系列
    // ========================================================================
    static Block* PURPUR_BLOCK;
    static Block* PURPUR_PILLAR;

    // ========================================================================
    // 末地系列
    // ========================================================================
    static Block* END_STONE_BRICKS;
    static Block* END_ROD;

    // ========================================================================
    // 骨块与干草块
    // ========================================================================
    static Block* BONE_BLOCK;
    static Block* HAY_BLOCK;

    // ========================================================================
    // 其他方块
    // ========================================================================
    static Block* SNOW;
    static Block* ICE;
    static Block* GLASS;          // 玻璃（透明，不传播天空光）
    static Block* NETHERRACK;
    static Block* GLOWSTONE;
    static Block* END_STONE;
    static Block* OBSIDIAN;

    // ========================================================================
    // 染色玻璃 (16色)
    // ========================================================================
    static Block* WHITE_STAINED_GLASS;
    static Block* ORANGE_STAINED_GLASS;
    static Block* MAGENTA_STAINED_GLASS;
    static Block* LIGHT_BLUE_STAINED_GLASS;
    static Block* YELLOW_STAINED_GLASS;
    static Block* LIME_STAINED_GLASS;
    static Block* PINK_STAINED_GLASS;
    static Block* GRAY_STAINED_GLASS;
    static Block* LIGHT_GRAY_STAINED_GLASS;
    static Block* CYAN_STAINED_GLASS;
    static Block* PURPLE_STAINED_GLASS;
    static Block* BLUE_STAINED_GLASS;
    static Block* BROWN_STAINED_GLASS;
    static Block* GREEN_STAINED_GLASS;
    static Block* RED_STAINED_GLASS;
    static Block* BLACK_STAINED_GLASS;

    // ========================================================================
    // 混凝土 (16色)
    // ========================================================================
    static Block* WHITE_CONCRETE;
    static Block* ORANGE_CONCRETE;
    static Block* MAGENTA_CONCRETE;
    static Block* LIGHT_BLUE_CONCRETE;
    static Block* YELLOW_CONCRETE;
    static Block* LIME_CONCRETE;
    static Block* PINK_CONCRETE;
    static Block* GRAY_CONCRETE;
    static Block* LIGHT_GRAY_CONCRETE;
    static Block* CYAN_CONCRETE;
    static Block* PURPLE_CONCRETE;
    static Block* BLUE_CONCRETE;
    static Block* BROWN_CONCRETE;
    static Block* GREEN_CONCRETE;
    static Block* RED_CONCRETE;
    static Block* BLACK_CONCRETE;

    // ========================================================================
    // 混凝土粉末 (16色)
    // ========================================================================
    static Block* WHITE_CONCRETE_POWDER;
    static Block* ORANGE_CONCRETE_POWDER;
    static Block* MAGENTA_CONCRETE_POWDER;
    static Block* LIGHT_BLUE_CONCRETE_POWDER;
    static Block* YELLOW_CONCRETE_POWDER;
    static Block* LIME_CONCRETE_POWDER;
    static Block* PINK_CONCRETE_POWDER;
    static Block* GRAY_CONCRETE_POWDER;
    static Block* LIGHT_GRAY_CONCRETE_POWDER;
    static Block* CYAN_CONCRETE_POWDER;
    static Block* PURPLE_CONCRETE_POWDER;
    static Block* BLUE_CONCRETE_POWDER;
    static Block* BROWN_CONCRETE_POWDER;
    static Block* GREEN_CONCRETE_POWDER;
    static Block* RED_CONCRETE_POWDER;
    static Block* BLACK_CONCRETE_POWDER;

    // ========================================================================
    // 陶瓦 (16色)
    // ========================================================================
    static Block* WHITE_TERRACOTTA;
    static Block* ORANGE_TERRACOTTA;
    static Block* MAGENTA_TERRACOTTA;
    static Block* LIGHT_BLUE_TERRACOTTA;
    static Block* YELLOW_TERRACOTTA;
    static Block* LIME_TERRACOTTA;
    static Block* PINK_TERRACOTTA;
    static Block* GRAY_TERRACOTTA;
    static Block* LIGHT_GRAY_TERRACOTTA;
    static Block* CYAN_TERRACOTTA;
    static Block* PURPLE_TERRACOTTA;
    static Block* BLUE_TERRACOTTA;
    static Block* BROWN_TERRACOTTA;
    static Block* GREEN_TERRACOTTA;
    static Block* RED_TERRACOTTA;
    static Block* BLACK_TERRACOTTA;
    static Block* TERRACOTTA;  // 普通陶瓦

    // ========================================================================
    // 下界方块
    // ========================================================================
    static Block* SOUL_SAND;
    static Block* SOUL_SOIL;
    static Block* BASALT;
    static Block* POLISHED_BASALT;
    static Block* BLACKSTONE;
    static Block* POLISHED_BLACKSTONE;
    static Block* CRYING_OBSIDIAN;
    static Block* MAGMA;              // 岩浆块 (发光)
    static Block* NETHER_WART_BLOCK;  // 地狱疣块

    // ========================================================================
    // 自然方块扩展
    // ========================================================================
    static Block* CLAY;               // 粘土
    static Block* MYCELIUM;           // 菌丝
    static Block* GRASS_PATH;         // 草径
    static Block* PACKED_ICE;         // 浮冰
    static Block* SLIME_BLOCK;        // 粘液块
    static Block* CACTUS;             // 仙人掌
    static Block* DEAD_BUSH;          // 枯萎灌木
    static Block* LILY_PAD;           // 睡莲
    static Block* VINE;               // 藤蔓
    static Block* COBWEB;             // 蜘蛛网
    static Block* SUGAR_CANE;         // 甘蔗

    // ========================================================================
    // 辅助函数
    // ========================================================================

    /**
     * @brief 安全获取方块默认状态
     *
     * 用于在初始化阶段可能尚未注册方块时安全获取默认状态。
     * 如果方块为空指针，返回 nullptr。
     *
     * @param block 方块指针（可能为 nullptr）
     * @return 方块默认状态指针，如果方块为空则返回 nullptr
     */
    [[nodiscard]] static const BlockState* getState(Block* block) {
        return block ? &block->defaultState() : nullptr;
    }

private:
    static bool s_initialized;

    static void registerBaseBlocks();
    static void registerOreBlocks();
    static void registerLogBlocks();
    static void registerStoneVariants();
    static void registerDirtVariants();
    static void registerSandstones();
    static void registerMineralBlocks();
    static void registerBuildingBlocks();
    static void registerFunctionalBlocks();
    static void registerWoolBlocks();
    static void registerPlanksVariants();
    static void registerNetherBlocks();
    static void registerTreeVariants();
    static void registerVegetationBlocks();
    static void registerColoredBlocks();  // 染色玻璃、混凝土、陶瓦等
    static void registerStoneBricks();     // 石砖系列
    static void registerQuartzBlocks();    // 石英系列
    static void registerPrismarineBlocks(); // 海晶系列
    static void registerPurpurBlocks();    // 紫珀系列
    static void registerEndBlocks();       // 末地方块
    static void registerBoneAndHayBlocks(); // 骨块和干草块
    static void registerNetherExtensionBlocks(); // 下界扩展方块（岩浆块等）
    static void registerNaturalBlocks();    // 自然扩展方块
};

} // namespace mc
