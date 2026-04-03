#include "VanillaBlocks.hpp"
#include "HarvestTool.hpp"
#include "blocks/LiquidBlock.hpp"
#include "blocks/DoorBlock.hpp"
#include "blocks/FenceGateBlock.hpp"
#include "blocks/CauldronBlock.hpp"
#include "blocks/EnchantingTableBlock.hpp"
#include "blocks/building/StairsBlock.hpp"
#include "blocks/building/SlabBlock.hpp"
#include "blocks/building/WallBlock.hpp"
#include "blocks/building/FenceBlock.hpp"
#include "blocks/building/TrapDoorBlock.hpp"
#include "blocks/ice/IceBlock.hpp"
#include "blocks/ocean/DriedKelpBlock.hpp"
#include "blocks/decorative/LanternBlock.hpp"
#include "blocks/decorative/CampfireBlock.hpp"
#include "blocks/functional/BeaconBlock.hpp"
#include "blocks/functional/BrewingStandBlock.hpp"
#include "blocks/redstone/RedstoneWireBlock.hpp"
#include "blocks/redstone/RedstoneTorchBlock.hpp"
#include "blocks/redstone/RedstoneWallTorchBlock.hpp"
#include "blocks/redstone/RedstoneBlock.hpp"
#include "blocks/redstone/RedstoneLampBlock.hpp"
#include "blocks/redstone/RedstoneRepeaterBlock.hpp"
#include "blocks/redstone/RedstoneComparatorBlock.hpp"
#include "blocks/redstone/ObserverBlock.hpp"
#include "blocks/redstone/LeverBlock.hpp"
#include "blocks/redstone/StoneButtonBlock.hpp"
#include "blocks/redstone/WoodButtonBlock.hpp"
#include "blocks/redstone/StonePressurePlateBlock.hpp"
#include "blocks/redstone/WoodPressurePlateBlock.hpp"
#include "blocks/redstone/WeightedPressurePlateBlock.hpp"
#include "blocks/redstone/DaylightDetectorBlock.hpp"
#include "blocks/redstone/PistonBlock.hpp"
#include "blocks/redstone/PistonHeadBlock.hpp"
#include "blocks/redstone/MovingPistonBlock.hpp"
#include "blocks/redstone/DispenserBlock.hpp"
#include "blocks/redstone/DropperBlock.hpp"
#include "blocks/redstone/NoteBlock.hpp"
#include "blocks/redstone/TNTBlock.hpp"
#include "blocks/redstone/TargetBlock.hpp"
#include "blocks/redstone/TripWireBlock.hpp"
#include "blocks/redstone/TripWireHookBlock.hpp"
#include "blocks/redstone/RailBlock.hpp"
#include "blocks/redstone/PoweredRailBlock.hpp"
#include "blocks/redstone/DetectorRailBlock.hpp"
#include "blocks/redstone/ActivatorRailBlock.hpp"
#include "blocks/nether/FireBlock.hpp"
#include "blocks/end/EndPortalBlock.hpp"
#include "../fluid/FluidRegistry.hpp"
#include "../fluid/fluids/WaterFluid.hpp"
#include "../fluid/fluids/LavaFluid.hpp"

namespace mc {

// ============================================================================
// 静态成员初始化
// ============================================================================
bool VanillaBlocks::s_initialized = false;

// 基础方块
Block* VanillaBlocks::AIR = nullptr;
Block* VanillaBlocks::STONE = nullptr;
Block* VanillaBlocks::GRASS_BLOCK = nullptr;
Block* VanillaBlocks::DIRT = nullptr;
Block* VanillaBlocks::COBBLESTONE = nullptr;
Block* VanillaBlocks::OAK_PLANKS = nullptr;
Block* VanillaBlocks::WATER = nullptr;
Block* VanillaBlocks::LAVA = nullptr;
Block* VanillaBlocks::BEDROCK = nullptr;
Block* VanillaBlocks::SAND = nullptr;
Block* VanillaBlocks::GRAVEL = nullptr;

// 石头变种
Block* VanillaBlocks::GRANITE = nullptr;
Block* VanillaBlocks::POLISHED_GRANITE = nullptr;
Block* VanillaBlocks::DIORITE = nullptr;
Block* VanillaBlocks::POLISHED_DIORITE = nullptr;
Block* VanillaBlocks::ANDESITE = nullptr;
Block* VanillaBlocks::POLISHED_ANDESITE = nullptr;

// 泥土变种
Block* VanillaBlocks::COARSE_DIRT = nullptr;
Block* VanillaBlocks::PODZOL = nullptr;

// 砂岩系列
Block* VanillaBlocks::SANDSTONE = nullptr;
Block* VanillaBlocks::CHISELED_SANDSTONE = nullptr;
Block* VanillaBlocks::CUT_SANDSTONE = nullptr;
Block* VanillaBlocks::RED_SANDSTONE = nullptr;

// 矿石方块
Block* VanillaBlocks::GOLD_ORE = nullptr;
Block* VanillaBlocks::IRON_ORE = nullptr;
Block* VanillaBlocks::COAL_ORE = nullptr;
Block* VanillaBlocks::DIAMOND_ORE = nullptr;
Block* VanillaBlocks::DIAMOND_BLOCK = nullptr;
Block* VanillaBlocks::EMERALD_ORE = nullptr;
Block* VanillaBlocks::LAPIS_ORE = nullptr;
Block* VanillaBlocks::REDSTONE_ORE = nullptr;
Block* VanillaBlocks::COPPER_ORE = nullptr;

// 下界矿石
Block* VanillaBlocks::NETHER_QUARTZ_ORE = nullptr;
Block* VanillaBlocks::NETHER_GOLD_ORE = nullptr;
Block* VanillaBlocks::ANCIENT_DEBRIS = nullptr;

// 矿物方块
Block* VanillaBlocks::GOLD_BLOCK = nullptr;
Block* VanillaBlocks::IRON_BLOCK = nullptr;
Block* VanillaBlocks::LAPIS_BLOCK = nullptr;
Block* VanillaBlocks::EMERALD_BLOCK = nullptr;
Block* VanillaBlocks::REDSTONE_BLOCK = nullptr;

// 建筑方块
Block* VanillaBlocks::BRICKS = nullptr;
Block* VanillaBlocks::MOSSY_COBBLESTONE = nullptr;
Block* VanillaBlocks::BOOKSHELF = nullptr;
Block* VanillaBlocks::TNT = nullptr;
Block* VanillaBlocks::SPONGE = nullptr;
Block* VanillaBlocks::WET_SPONGE = nullptr;

// 功能方块
Block* VanillaBlocks::CRAFTING_TABLE = nullptr;
Block* VanillaBlocks::CAULDRON = nullptr;
Block* VanillaBlocks::ENCHANTING_TABLE = nullptr;

// 门和栅栏门
Block* VanillaBlocks::OAK_DOOR = nullptr;
Block* VanillaBlocks::IRON_DOOR = nullptr;
Block* VanillaBlocks::OAK_FENCE_GATE = nullptr;

// 楼梯
Block* VanillaBlocks::OAK_STAIRS = nullptr;
Block* VanillaBlocks::STONE_STAIRS = nullptr;
Block* VanillaBlocks::COBBLESTONE_STAIRS = nullptr;

// 台阶
Block* VanillaBlocks::OAK_SLAB = nullptr;
Block* VanillaBlocks::STONE_SLAB = nullptr;
Block* VanillaBlocks::COBBLESTONE_SLAB = nullptr;

// 墙
Block* VanillaBlocks::COBBLESTONE_WALL = nullptr;
Block* VanillaBlocks::STONE_BRICK_WALL = nullptr;

// 栅栏
Block* VanillaBlocks::OAK_FENCE = nullptr;

// 活板门
Block* VanillaBlocks::OAK_TRAPDOOR = nullptr;
Block* VanillaBlocks::IRON_TRAPDOOR = nullptr;

// 羊毛
Block* VanillaBlocks::WHITE_WOOL = nullptr;
Block* VanillaBlocks::ORANGE_WOOL = nullptr;
Block* VanillaBlocks::MAGENTA_WOOL = nullptr;
Block* VanillaBlocks::LIGHT_BLUE_WOOL = nullptr;
Block* VanillaBlocks::YELLOW_WOOL = nullptr;
Block* VanillaBlocks::LIME_WOOL = nullptr;
Block* VanillaBlocks::PINK_WOOL = nullptr;
Block* VanillaBlocks::GRAY_WOOL = nullptr;
Block* VanillaBlocks::LIGHT_GRAY_WOOL = nullptr;
Block* VanillaBlocks::CYAN_WOOL = nullptr;
Block* VanillaBlocks::PURPLE_WOOL = nullptr;
Block* VanillaBlocks::BLUE_WOOL = nullptr;
Block* VanillaBlocks::BROWN_WOOL = nullptr;
Block* VanillaBlocks::GREEN_WOOL = nullptr;
Block* VanillaBlocks::RED_WOOL = nullptr;
Block* VanillaBlocks::BLACK_WOOL = nullptr;

// 木板变种
Block* VanillaBlocks::SPRUCE_PLANKS = nullptr;
Block* VanillaBlocks::BIRCH_PLANKS = nullptr;
Block* VanillaBlocks::JUNGLE_PLANKS = nullptr;
Block* VanillaBlocks::ACACIA_PLANKS = nullptr;
Block* VanillaBlocks::DARK_OAK_PLANKS = nullptr;

// 原木和树叶
Block* VanillaBlocks::OAK_LOG = nullptr;
Block* VanillaBlocks::OAK_LEAVES = nullptr;
Block* VanillaBlocks::SPRUCE_LOG = nullptr;
Block* VanillaBlocks::BIRCH_LOG = nullptr;
Block* VanillaBlocks::JUNGLE_LOG = nullptr;
Block* VanillaBlocks::ACACIA_LOG = nullptr;
Block* VanillaBlocks::DARK_OAK_LOG = nullptr;
Block* VanillaBlocks::SPRUCE_LEAVES = nullptr;
Block* VanillaBlocks::BIRCH_LEAVES = nullptr;
Block* VanillaBlocks::JUNGLE_LEAVES = nullptr;
Block* VanillaBlocks::ACACIA_LEAVES = nullptr;
Block* VanillaBlocks::DARK_OAK_LEAVES = nullptr;

// 植被方块
Block* VanillaBlocks::SHORT_GRASS = nullptr;
Block* VanillaBlocks::TALL_GRASS = nullptr;
Block* VanillaBlocks::FERN = nullptr;
Block* VanillaBlocks::DANDELION = nullptr;
Block* VanillaBlocks::POPPY = nullptr;
Block* VanillaBlocks::BLUE_ORCHID = nullptr;
Block* VanillaBlocks::ALLIUM = nullptr;
Block* VanillaBlocks::AZURE_BLUET = nullptr;
Block* VanillaBlocks::RED_TULIP = nullptr;
Block* VanillaBlocks::ORANGE_TULIP = nullptr;
Block* VanillaBlocks::WHITE_TULIP = nullptr;
Block* VanillaBlocks::PINK_TULIP = nullptr;
Block* VanillaBlocks::OXEYE_DAISY = nullptr;
Block* VanillaBlocks::LILY_OF_THE_VALLEY = nullptr;
Block* VanillaBlocks::SUNFLOWER = nullptr;
Block* VanillaBlocks::LILAC = nullptr;
Block* VanillaBlocks::ROSE_BUSH = nullptr;
Block* VanillaBlocks::PEONY = nullptr;
Block* VanillaBlocks::CORNFLOWER = nullptr;
Block* VanillaBlocks::WITHER_ROSE = nullptr;
Block* VanillaBlocks::BROWN_MUSHROOM = nullptr;
Block* VanillaBlocks::RED_MUSHROOM = nullptr;
Block* VanillaBlocks::BROWN_MUSHROOM_BLOCK = nullptr;
Block* VanillaBlocks::RED_MUSHROOM_BLOCK = nullptr;
Block* VanillaBlocks::MUSHROOM_STEM = nullptr;

// 树苗
Block* VanillaBlocks::OAK_SAPLING = nullptr;
Block* VanillaBlocks::SPRUCE_SAPLING = nullptr;
Block* VanillaBlocks::BIRCH_SAPLING = nullptr;
Block* VanillaBlocks::JUNGLE_SAPLING = nullptr;
Block* VanillaBlocks::ACACIA_SAPLING = nullptr;
Block* VanillaBlocks::DARK_OAK_SAPLING = nullptr;

// 其他方块
Block* VanillaBlocks::SNOW = nullptr;
Block* VanillaBlocks::ICE = nullptr;
Block* VanillaBlocks::GLASS = nullptr;
Block* VanillaBlocks::NETHERRACK = nullptr;
Block* VanillaBlocks::GLOWSTONE = nullptr;
Block* VanillaBlocks::END_STONE = nullptr;
Block* VanillaBlocks::OBSIDIAN = nullptr;
Block* VanillaBlocks::NETHER_PORTAL = nullptr;
Block* VanillaBlocks::END_PORTAL = nullptr;
Block* VanillaBlocks::END_PORTAL_FRAME = nullptr;
Block* VanillaBlocks::END_GATEWAY = nullptr;
Block* VanillaBlocks::BEACON = nullptr;
Block* VanillaBlocks::BREWING_STAND = nullptr;
Block* VanillaBlocks::ENDER_CHEST = nullptr;
Block* VanillaBlocks::LANTERN = nullptr;
Block* VanillaBlocks::SOUL_LANTERN = nullptr;
Block* VanillaBlocks::CAMPFIRE = nullptr;
Block* VanillaBlocks::SOUL_CAMPFIRE = nullptr;
Block* VanillaBlocks::JACK_O_LANTERN = nullptr;

// 红石方块
Block* VanillaBlocks::REDSTONE_WIRE = nullptr;
Block* VanillaBlocks::REDSTONE_TORCH = nullptr;
Block* VanillaBlocks::REDSTONE_WALL_TORCH = nullptr;
Block* VanillaBlocks::REDSTONE_LAMP = nullptr;
Block* VanillaBlocks::REDSTONE_REPEATER = nullptr;
Block* VanillaBlocks::REDSTONE_COMPARATOR = nullptr;
Block* VanillaBlocks::OBSERVER = nullptr;
Block* VanillaBlocks::LEVER = nullptr;
Block* VanillaBlocks::STONE_BUTTON = nullptr;
Block* VanillaBlocks::OAK_BUTTON = nullptr;
Block* VanillaBlocks::STONE_PRESSURE_PLATE = nullptr;
Block* VanillaBlocks::OAK_PRESSURE_PLATE = nullptr;
Block* VanillaBlocks::LIGHT_WEIGHTED_PRESSURE_PLATE = nullptr;
Block* VanillaBlocks::HEAVY_WEIGHTED_PRESSURE_PLATE = nullptr;
Block* VanillaBlocks::DAYLIGHT_DETECTOR = nullptr;
Block* VanillaBlocks::PISTON = nullptr;
Block* VanillaBlocks::STICKY_PISTON = nullptr;
Block* VanillaBlocks::PISTON_HEAD = nullptr;
Block* VanillaBlocks::MOVING_PISTON = nullptr;
Block* VanillaBlocks::DISPENSER = nullptr;
Block* VanillaBlocks::DROPPER = nullptr;
Block* VanillaBlocks::NOTE_BLOCK = nullptr;
Block* VanillaBlocks::TRIPWIRE = nullptr;
Block* VanillaBlocks::TRIPWIRE_HOOK = nullptr;
Block* VanillaBlocks::TARGET = nullptr;

// 铁轨方块
Block* VanillaBlocks::RAIL = nullptr;
Block* VanillaBlocks::POWERED_RAIL = nullptr;
Block* VanillaBlocks::DETECTOR_RAIL = nullptr;
Block* VanillaBlocks::ACTIVATOR_RAIL = nullptr;

// 下界方块
Block* VanillaBlocks::SOUL_SAND = nullptr;
Block* VanillaBlocks::SOUL_SOIL = nullptr;
Block* VanillaBlocks::BASALT = nullptr;
Block* VanillaBlocks::POLISHED_BASALT = nullptr;
Block* VanillaBlocks::BLACKSTONE = nullptr;
Block* VanillaBlocks::POLISHED_BLACKSTONE = nullptr;
Block* VanillaBlocks::CRYING_OBSIDIAN = nullptr;
Block* VanillaBlocks::MAGMA = nullptr;
Block* VanillaBlocks::NETHER_WART_BLOCK = nullptr;
Block* VanillaBlocks::FIRE = nullptr;
Block* VanillaBlocks::SOUL_FIRE = nullptr;
Block* VanillaBlocks::NETHER_WART = nullptr;

// 自然方块扩展
Block* VanillaBlocks::CLAY = nullptr;
Block* VanillaBlocks::MYCELIUM = nullptr;
Block* VanillaBlocks::GRASS_PATH = nullptr;
Block* VanillaBlocks::PACKED_ICE = nullptr;
Block* VanillaBlocks::BLUE_ICE = nullptr;
Block* VanillaBlocks::FROSTED_ICE = nullptr;
Block* VanillaBlocks::SLIME_BLOCK = nullptr;
Block* VanillaBlocks::CACTUS = nullptr;
Block* VanillaBlocks::DEAD_BUSH = nullptr;
Block* VanillaBlocks::LILY_PAD = nullptr;
Block* VanillaBlocks::VINE = nullptr;
Block* VanillaBlocks::COBWEB = nullptr;
Block* VanillaBlocks::SUGAR_CANE = nullptr;
Block* VanillaBlocks::FARMLAND = nullptr;
Block* VanillaBlocks::RED_SAND = nullptr;
Block* VanillaBlocks::DRIED_KELP_BLOCK = nullptr;
Block* VanillaBlocks::CONDUIT = nullptr;
Block* VanillaBlocks::CRIMSON_STEM = nullptr;
Block* VanillaBlocks::WARPED_STEM = nullptr;
Block* VanillaBlocks::CRIMSON_NYLIUM = nullptr;
Block* VanillaBlocks::WARPED_NYLIUM = nullptr;
Block* VanillaBlocks::SHROOMLIGHT = nullptr;
Block* VanillaBlocks::CRIMSON_FUNGUS = nullptr;
Block* VanillaBlocks::WARPED_FUNGUS = nullptr;
Block* VanillaBlocks::WEEPING_VINES = nullptr;
Block* VanillaBlocks::TWISTING_VINES = nullptr;

// 石砖系列
Block* VanillaBlocks::STONE_BRICKS = nullptr;
Block* VanillaBlocks::MOSSY_STONE_BRICKS = nullptr;
Block* VanillaBlocks::CRACKED_STONE_BRICKS = nullptr;
Block* VanillaBlocks::CHISELED_STONE_BRICKS = nullptr;

// 石英系列
Block* VanillaBlocks::QUARTZ_BLOCK = nullptr;
Block* VanillaBlocks::CHISELED_QUARTZ_BLOCK = nullptr;
Block* VanillaBlocks::QUARTZ_PILLAR = nullptr;

// 海晶系列
Block* VanillaBlocks::PRISMARINE = nullptr;
Block* VanillaBlocks::PRISMARINE_BRICKS = nullptr;
Block* VanillaBlocks::DARK_PRISMARINE = nullptr;
Block* VanillaBlocks::SEA_LANTERN = nullptr;

// 紫珀系列
Block* VanillaBlocks::PURPUR_BLOCK = nullptr;
Block* VanillaBlocks::PURPUR_PILLAR = nullptr;

// 末地系列
Block* VanillaBlocks::END_STONE_BRICKS = nullptr;
Block* VanillaBlocks::END_ROD = nullptr;
Block* VanillaBlocks::CHORUS_PLANT = nullptr;
Block* VanillaBlocks::CHORUS_FLOWER = nullptr;
Block* VanillaBlocks::DRAGON_EGG = nullptr;

// 骨块与干草块
Block* VanillaBlocks::BONE_BLOCK = nullptr;
Block* VanillaBlocks::HAY_BLOCK = nullptr;

// 染色玻璃 (16色)
Block* VanillaBlocks::WHITE_STAINED_GLASS = nullptr;
Block* VanillaBlocks::ORANGE_STAINED_GLASS = nullptr;
Block* VanillaBlocks::MAGENTA_STAINED_GLASS = nullptr;
Block* VanillaBlocks::LIGHT_BLUE_STAINED_GLASS = nullptr;
Block* VanillaBlocks::YELLOW_STAINED_GLASS = nullptr;
Block* VanillaBlocks::LIME_STAINED_GLASS = nullptr;
Block* VanillaBlocks::PINK_STAINED_GLASS = nullptr;
Block* VanillaBlocks::GRAY_STAINED_GLASS = nullptr;
Block* VanillaBlocks::LIGHT_GRAY_STAINED_GLASS = nullptr;
Block* VanillaBlocks::CYAN_STAINED_GLASS = nullptr;
Block* VanillaBlocks::PURPLE_STAINED_GLASS = nullptr;
Block* VanillaBlocks::BLUE_STAINED_GLASS = nullptr;
Block* VanillaBlocks::BROWN_STAINED_GLASS = nullptr;
Block* VanillaBlocks::GREEN_STAINED_GLASS = nullptr;
Block* VanillaBlocks::RED_STAINED_GLASS = nullptr;
Block* VanillaBlocks::BLACK_STAINED_GLASS = nullptr;

// 混凝土 (16色)
Block* VanillaBlocks::WHITE_CONCRETE = nullptr;
Block* VanillaBlocks::ORANGE_CONCRETE = nullptr;
Block* VanillaBlocks::MAGENTA_CONCRETE = nullptr;
Block* VanillaBlocks::LIGHT_BLUE_CONCRETE = nullptr;
Block* VanillaBlocks::YELLOW_CONCRETE = nullptr;
Block* VanillaBlocks::LIME_CONCRETE = nullptr;
Block* VanillaBlocks::PINK_CONCRETE = nullptr;
Block* VanillaBlocks::GRAY_CONCRETE = nullptr;
Block* VanillaBlocks::LIGHT_GRAY_CONCRETE = nullptr;
Block* VanillaBlocks::CYAN_CONCRETE = nullptr;
Block* VanillaBlocks::PURPLE_CONCRETE = nullptr;
Block* VanillaBlocks::BLUE_CONCRETE = nullptr;
Block* VanillaBlocks::BROWN_CONCRETE = nullptr;
Block* VanillaBlocks::GREEN_CONCRETE = nullptr;
Block* VanillaBlocks::RED_CONCRETE = nullptr;
Block* VanillaBlocks::BLACK_CONCRETE = nullptr;

// 混凝土粉末 (16色)
Block* VanillaBlocks::WHITE_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::ORANGE_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::MAGENTA_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::LIGHT_BLUE_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::YELLOW_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::LIME_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::PINK_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::GRAY_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::LIGHT_GRAY_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::CYAN_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::PURPLE_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::BLUE_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::BROWN_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::GREEN_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::RED_CONCRETE_POWDER = nullptr;
Block* VanillaBlocks::BLACK_CONCRETE_POWDER = nullptr;

// 陶瓦 (16色 + 普通)
Block* VanillaBlocks::WHITE_TERRACOTTA = nullptr;
Block* VanillaBlocks::ORANGE_TERRACOTTA = nullptr;
Block* VanillaBlocks::MAGENTA_TERRACOTTA = nullptr;
Block* VanillaBlocks::LIGHT_BLUE_TERRACOTTA = nullptr;
Block* VanillaBlocks::YELLOW_TERRACOTTA = nullptr;
Block* VanillaBlocks::LIME_TERRACOTTA = nullptr;
Block* VanillaBlocks::PINK_TERRACOTTA = nullptr;
Block* VanillaBlocks::GRAY_TERRACOTTA = nullptr;
Block* VanillaBlocks::LIGHT_GRAY_TERRACOTTA = nullptr;
Block* VanillaBlocks::CYAN_TERRACOTTA = nullptr;
Block* VanillaBlocks::PURPLE_TERRACOTTA = nullptr;
Block* VanillaBlocks::BLUE_TERRACOTTA = nullptr;
Block* VanillaBlocks::BROWN_TERRACOTTA = nullptr;
Block* VanillaBlocks::GREEN_TERRACOTTA = nullptr;
Block* VanillaBlocks::RED_TERRACOTTA = nullptr;
Block* VanillaBlocks::BLACK_TERRACOTTA = nullptr;
Block* VanillaBlocks::TERRACOTTA = nullptr;

// ============================================================================
// 初始化
// ============================================================================
void VanillaBlocks::initialize() {
    if (s_initialized) {
        return;
    }

    registerBaseBlocks();
    registerOreBlocks();
    registerLogBlocks();
    registerStoneVariants();
    registerDirtVariants();
    registerSandstones();
    registerMineralBlocks();
    registerBuildingBlocks();
    registerFunctionalBlocks();
    registerRedstoneBlocks();
    registerWoolBlocks();
    registerPlanksVariants();
    registerNetherBlocks();
    registerTreeVariants();
    registerVegetationBlocks();
    registerColoredBlocks();
    registerStoneBricks();
    registerQuartzBlocks();
    registerPrismarineBlocks();
    registerPurpurBlocks();
    registerEndBlocks();
    registerBoneAndHayBlocks();
    registerNetherExtensionBlocks();
    registerNaturalBlocks();
    registerStairsSlabsWalls();

    s_initialized = true;
}

// ============================================================================
// 基础方块注册
// ============================================================================
void VanillaBlocks::registerBaseBlocks() {
    auto& registry = BlockRegistry::instance();

    // 首先初始化流体注册表（确保流体先于方块注册）
    // 这样LiquidBlock可以引用已注册的流体
    fluid::FluidRegistry::instance().initialize();

    // 空气 - ID 0
    AIR = &registry.registerBlock<AirBlock>(
        ResourceLocation("minecraft:air"),
        BlockProperties(Material::AIR)
            .noCollision()
            .notSolid()
            .replaceable()
            .opacity(0)
            .propagatesSkylightDown()
    );

    // 石头 - ID 1
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(1.5F, 6.0F))
    STONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:stone"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(0)
            .requiresTool()
    );

    // 草方块 - ID 2
    // 参考: new GrassBlock(Properties.create(Material.ORGANIC).tickRandomly().hardnessAndResistance(0.6F))
    GRASS_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:grass_block"),
        BlockProperties(Material::EARTH).hardness(0.6f)
    );

    // 泥土 - ID 3
    // 参考: new Block(Properties.create(Material.EARTH).hardnessAndResistance(0.5F))
    DIRT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dirt"),
        BlockProperties(Material::EARTH).hardness(0.5f)
    );

    // 圆石 - ID 4
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(2.0F, 6.0F))
    COBBLESTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cobblestone"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f)
    );

    // 橡木木板 - ID 5
    // 参考: new Block(Properties.create(Material.WOOD).hardnessAndResistance(2.0F, 3.0F))
    OAK_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:oak_planks"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable()
    );

    // 水 - ID 6
    // 使用LiquidBlock注册，关联FlowingFluid
    // 参考: net.minecraft.block.FlowingFluidBlock
    // 视觉修正：当前渲染链路下，opacity=1 会导致海底在约 15 格深度后出现纯黑。
    // 这里改为 opacity=0，保留不传播天空光语义，同时避免“y48 以下全黑”的断崖现象。
    {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(
            fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            auto* flowingWater = dynamic_cast<fluid::FlowingFluid*>(waterFluid);
            if (flowingWater != nullptr) {
                WATER = &registry.registerBlock<block::LiquidBlock>(
                    ResourceLocation("minecraft:water"),
                    *flowingWater,
                    BlockProperties(Material::WATER).noCollision().notSolid().opacity(0)
                );
            }
        }
    }

    // 岩浆 - ID 7
    // 使用LiquidBlock注册，关联FlowingFluid
    // 岩浆：发光15级，tick延迟30（主世界）
    {
        fluid::Fluid* lavaFluid = fluid::FluidRegistry::instance().getFluid(
            fluid::FluidRegistry::LAVA_ID);
        if (lavaFluid != nullptr) {
            auto* flowingLava = dynamic_cast<fluid::FlowingFluid*>(lavaFluid);
            if (flowingLava != nullptr) {
                LAVA = &registry.registerBlock<block::LiquidBlock>(
                    ResourceLocation("minecraft:lava"),
                    *flowingLava,
                    BlockProperties(Material::LAVA).noCollision().notSolid().lightLevel(15)
                );
            }
        }
    }

    // 基岩 - ID 8
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(-1.0F, 3600000.0F).noDrops())
    BEDROCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:bedrock"),
        BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f)
    );

    // 沙子 - ID 9
    // 参考: new SandBlock(14406560, Properties.create(Material.SAND).hardnessAndResistance(0.5F))
    SAND = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sand"),
        BlockProperties(Material::SAND).hardness(0.5f)
    );

    // 砾石 - ID 10
    // 参考: new GravelBlock(Properties.create(Material.SAND).hardnessAndResistance(0.6F))
    GRAVEL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gravel"),
        BlockProperties(Material::SAND).hardness(0.6f)
    );

    // 雪 - ID 18
    // 雪：透明度1，不传播天空光（阻挡）
    SNOW = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:snow"),
        BlockProperties(Material::SNOW).hardness(0.2f).opacity(1)
    );

    // 冰 - ID 79
    // 冰：透明度2，传播天空光，会融化
    ICE = &registry.registerBlock<blocks::IceBlock>(
        ResourceLocation("minecraft:ice"),
        BlockProperties(Material::ICE).hardness(0.5f).notSolid().opacity(2).propagatesSkylightDown()
    );

    // 玻璃 - ID 20 (调整后的ID)
    // 玻璃：完全透光并传播天空光
    GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:glass"),
        BlockProperties(Material::GLASS).hardness(0.3f).notSolid().opacity(0).propagatesSkylightDown()
    );

    // 下界岩 - ID 21
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(0.4F))
    NETHERRACK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:netherrack"),
        BlockProperties(Material::ROCK).hardness(0.4f)
    );

    // 荧石 - ID 21
    // 参考: new Block(Properties.create(Material.GLASS).hardnessAndResistance(0.3F).setLightLevel(15))
    GLOWSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:glowstone"),
        BlockProperties(Material::GLASS).hardness(0.3f).lightLevel(15)
    );

    // 末地石 - ID 22
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(3.0F, 9.0F))
    END_STONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:end_stone"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(9.0f)
    );

    // 黑曜石 - ID 23
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(50.0F, 1200.0F))
    OBSIDIAN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:obsidian"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f)
    );
}

// ============================================================================
// 矿石方块注册
// ============================================================================
void VanillaBlocks::registerOreBlocks() {
    auto& registry = BlockRegistry::instance();

    // 金矿石 - ID 11
    // 参考: new OreBlock(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(3.0F, 3.0F))
    GOLD_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gold_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 铁矿石 - ID 12
    IRON_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:iron_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 煤矿石 - ID 13
    COAL_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:coal_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 钻石矿石 - ID 14
    DIAMOND_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:diamond_ore"),
        BlockProperties(Material::ROCK)
            .hardness(3.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(2)  // 需要铁镐及以上
            .requiresTool()
    );

    // 钻石块 - ID 15
    // 参考: new Block(Properties.create(Material.IRON).setRequiresTool().hardnessAndResistance(5.0F, 6.0F))
    DIAMOND_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:diamond_block"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f)
    );

    // 绿宝石矿石
    EMERALD_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:emerald_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 青金石矿石
    LAPIS_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lapis_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 红石矿石
    REDSTONE_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:redstone_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 铜矿 (1.17+)
    // 参考: new OreBlock(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(3.0F, 3.0F))
    COPPER_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:copper_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 下界石英矿
    // 参考: new OreBlock(Properties.create(Material.ROCK).hardnessAndResistance(3.0F, 3.0F))
    NETHER_QUARTZ_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:nether_quartz_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 下界金矿
    NETHER_GOLD_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:nether_gold_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 远古残骸
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(50.0F, 1200.0F))
    ANCIENT_DEBRIS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:ancient_debris"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f)
    );
}

// ============================================================================
// 原木注册
// ============================================================================
void VanillaBlocks::registerLogBlocks() {
    auto& registry = BlockRegistry::instance();

    // 橡木原木 - ID 16 (有3个状态，对应3个轴)
    OAK_LOG = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:oak_log"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable()
    );

    // 橡木树叶 - ID 17
    OAK_LEAVES = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:oak_leaves"),
        BlockProperties(Material::LEAVES)
            .hardness(0.2f)
            .flammable()
            .notSolid()
            .opacity(1)
            .propagatesSkylightDown()
    );
}

// ============================================================================
// 石头变种注册
// ============================================================================
void VanillaBlocks::registerStoneVariants() {
    auto& registry = BlockRegistry::instance();

    // 花岗岩
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(1.5F, 6.0F))
    GRANITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:granite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 磨制花岗岩
    POLISHED_GRANITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:polished_granite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 闪长岩
    DIORITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:diorite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 磨制闪长岩
    POLISHED_DIORITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:polished_diorite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 安山岩
    ANDESITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:andesite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 磨制安山岩
    POLISHED_ANDESITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:polished_andesite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );
}

// ============================================================================
// 泥土变种注册
// ============================================================================
void VanillaBlocks::registerDirtVariants() {
    auto& registry = BlockRegistry::instance();

    // 粗泥土
    // 参考: new Block(Properties.create(Material.EARTH).hardnessAndResistance(0.5F))
    COARSE_DIRT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:coarse_dirt"),
        BlockProperties(Material::EARTH).hardness(0.5f)
    );

    // 灰化土
    // 参考: new SnowyDirtBlock(Properties.create(Material.EARTH).hardnessAndResistance(0.5F))
    PODZOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:podzol"),
        BlockProperties(Material::EARTH).hardness(0.5f)
    );
}

// ============================================================================
// 砂岩系列注册
// ============================================================================
void VanillaBlocks::registerSandstones() {
    auto& registry = BlockRegistry::instance();

    // 砂岩
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(0.8F))
    SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sandstone"),
        BlockProperties(Material::ROCK).hardness(0.8f)
    );

    // 錾制砂岩
    CHISELED_SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:chiseled_sandstone"),
        BlockProperties(Material::ROCK).hardness(0.8f)
    );

    // 切制砂岩
    CUT_SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cut_sandstone"),
        BlockProperties(Material::ROCK).hardness(0.8f)
    );

    // 红砂岩
    RED_SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_sandstone"),
        BlockProperties(Material::ROCK).hardness(0.8f)
    );
}

// ============================================================================
// 矿物方块注册
// ============================================================================
void VanillaBlocks::registerMineralBlocks() {
    auto& registry = BlockRegistry::instance();

    // 金块
    // 参考: new Block(Properties.create(Material.IRON).setRequiresTool().hardnessAndResistance(3.0F, 6.0F))
    GOLD_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gold_block"),
        BlockProperties(Material::IRON).hardness(3.0f).resistance(6.0f)
    );

    // 铁块
    // 参考: new Block(Properties.create(Material.IRON).setRequiresTool().hardnessAndResistance(5.0F, 6.0F))
    IRON_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:iron_block"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f)
    );

    // 青金石块
    // 参考: new Block(Properties.create(Material.IRON).setRequiresTool().hardnessAndResistance(3.0F, 3.0F))
    LAPIS_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lapis_block"),
        BlockProperties(Material::IRON).hardness(3.0f).resistance(3.0f)
    );

    // 绿宝石块
    EMERALD_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:emerald_block"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f)
    );

    // 红石块
    REDSTONE_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:redstone_block"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f)
    );
}

// ============================================================================
// 建筑方块注册
// ============================================================================
void VanillaBlocks::registerBuildingBlocks() {
    auto& registry = BlockRegistry::instance();

    // 砖块
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(2.0F, 6.0F))
    BRICKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:bricks"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f)
    );

    // 苔石圆石
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(2.0F, 6.0F))
    MOSSY_COBBLESTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:mossy_cobblestone"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f)
    );

    // 书架
    // 参考: new Block(Properties.create(Material.WOOD).hardnessAndResistance(1.5F))
    BOOKSHELF = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:bookshelf"),
        BlockProperties(Material::WOOD).hardness(1.5f).flammable()
    );

    // 海绵
    // 参考: new SpongeBlock(Properties.create(Material.SPONGE).hardnessAndResistance(0.6F))
    SPONGE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sponge"),
        BlockProperties(Material::SPONGE).hardness(0.6f)
    );

    // 湿海绵
    WET_SPONGE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:wet_sponge"),
        BlockProperties(Material::SPONGE).hardness(0.6f)
    );
}

// ============================================================================
// 功能方块注册
// ============================================================================
void VanillaBlocks::registerFunctionalBlocks() {
    auto& registry = BlockRegistry::instance();

    // 工作台
    // 参考: new CraftingTableBlock(Properties.create(Material.WOOD).hardnessAndResistance(2.5F))
    CRAFTING_TABLE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:crafting_table"),
        BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).flammable()
    );

    // 炼药锅
    // 参考: new CauldronBlock(Properties.create(Material.IRON).hardnessAndResistance(2.0F).notSolid())
    CAULDRON = &registry.registerBlock<blocks::CauldronBlock>(
        ResourceLocation("minecraft:cauldron"),
        BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid()
    );

    // 附魔台
    // 参考: new EnchantingTableBlock(Properties.create(Material.ROCK).hardnessAndResistance(5.0F).notSolid())
    ENCHANTING_TABLE = &registry.registerBlock<blocks::EnchantingTableBlock>(
        ResourceLocation("minecraft:enchanting_table"),
        BlockProperties(Material::ROCK).hardness(5.0f).resistance(1200.0f).notSolid().lightLevel(7)
    );

    // 橡木门
    // 参考: new DoorBlock(Material.WOOD, Block.Properties.create(Material.WOOD).hardnessAndResistance(3.0F).notSolid())
    OAK_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:oak_door"),
        BlockProperties(Material::WOOD).hardness(3.0f).resistance(3.0f).notSolid().flammable(),
        false  // 不是铁门
    );

    // 铁门
    // 参考: new DoorBlock(Material.IRON, Block.Properties.create(Material.IRON).hardnessAndResistance(5.0F).notSolid())
    IRON_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:iron_door"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f).notSolid(),
        true  // 是铁门
    );

    // 橡木栅栏门
    // 参考: new FenceGateBlock(Material.WOOD, Block.Properties.create(Material.WOOD).hardnessAndResistance(2.0F).notSolid())
    OAK_FENCE_GATE = &registry.registerBlock<blocks::FenceGateBlock>(
        ResourceLocation("minecraft:oak_fence_gate"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).notSolid().flammable()
    );
}

// ============================================================================
// 羊毛注册 (16色)
// ============================================================================
void VanillaBlocks::registerWoolBlocks() {
    auto& registry = BlockRegistry::instance();

    // 参考: new Block(Properties.create(Material.WOOL).hardnessAndResistance(0.8F))
    // 所有羊毛使用相同的属性
    BlockProperties woolProps = BlockProperties(Material::WOOL).hardness(0.8f);

    WHITE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:white_wool"), woolProps);
    ORANGE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:orange_wool"), woolProps);
    MAGENTA_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:magenta_wool"), woolProps);
    LIGHT_BLUE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_blue_wool"), woolProps);
    YELLOW_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:yellow_wool"), woolProps);
    LIME_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lime_wool"), woolProps);
    PINK_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:pink_wool"), woolProps);
    GRAY_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gray_wool"), woolProps);
    LIGHT_GRAY_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_gray_wool"), woolProps);
    CYAN_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cyan_wool"), woolProps);
    PURPLE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:purple_wool"), woolProps);
    BLUE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blue_wool"), woolProps);
    BROWN_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:brown_wool"), woolProps);
    GREEN_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:green_wool"), woolProps);
    RED_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_wool"), woolProps);
    BLACK_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:black_wool"), woolProps);
}

// ============================================================================
// 木板变种注册
// ============================================================================
void VanillaBlocks::registerPlanksVariants() {
    auto& registry = BlockRegistry::instance();

    // 参考: new Block(Properties.create(Material.WOOD).hardnessAndResistance(2.0F, 3.0F))
    BlockProperties planksProps = BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable();

    SPRUCE_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:spruce_planks"), planksProps);
    BIRCH_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:birch_planks"), planksProps);
    JUNGLE_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:jungle_planks"), planksProps);
    ACACIA_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:acacia_planks"), planksProps);
    DARK_OAK_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dark_oak_planks"), planksProps);
}

// ============================================================================
// 下界方块注册
// ============================================================================
void VanillaBlocks::registerNetherBlocks() {
    auto& registry = BlockRegistry::instance();

    // 灵魂沙
    // 参考: new Block(Properties.create(Material.SAND).hardnessAndResistance(0.5F))
    SOUL_SAND = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:soul_sand"),
        BlockProperties(Material::SAND).hardness(0.5f)
    );

    // 灵魂土
    // 参考: new Block(Properties.create(Material.EARTH).hardnessAndResistance(0.5F))
    SOUL_SOIL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:soul_soil"),
        BlockProperties(Material::EARTH).hardness(0.5f)
    );

    // 玄武岩
    // 参考: new RotatedPillarBlock(Properties.create(Material.ROCK).hardnessAndResistance(1.25F, 4.2F))
    BASALT = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:basalt"),
        BlockProperties(Material::ROCK).hardness(1.25f).resistance(4.2f)
    );

    // 磨制玄武岩
    POLISHED_BASALT = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:polished_basalt"),
        BlockProperties(Material::ROCK).hardness(1.25f).resistance(4.2f)
    );

    // 黑石
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(1.5F, 6.0F))
    BLACKSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blackstone"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 磨制黑石
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(2.0F, 6.0F))
    POLISHED_BLACKSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:polished_blackstone"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f)
    );

    // 哭泣的黑曜石
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(50.0F, 1200.0F).setLightLevel(10))
    CRYING_OBSIDIAN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:crying_obsidian"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f).lightLevel(10)
    );

    // 火 - 普通火焰
    // 参考: new FireBlock(Properties.create(Material.FIRE).doesNotBlockMovement().zeroHardnessAndResistance().setLightLevel(15))
    FIRE = &registry.registerBlock<blocks::FireBlock>(
        ResourceLocation("minecraft:fire"),
        BlockProperties(Material::FIRE).noCollision().hardness(0.0f).lightLevel(15)
    );

    // 灵魂火 - 蓝色火焰，伤害更高
    // 参考: new SoulFireBlock(Properties.create(Material.FIRE).doesNotBlockMovement().zeroHardnessAndResistance().setLightLevel(10))
    SOUL_FIRE = &registry.registerBlock<blocks::SoulFireBlock>(
        ResourceLocation("minecraft:soul_fire"),
        BlockProperties(Material::FIRE).noCollision().hardness(0.0f).lightLevel(10)
    );

    // 下界传送门
    // 参考: new NetherPortalBlock(Properties.create(Material.PORTAL).doesNotBlockMovement().zeroHardnessAndResistance().setLightLevel(11))
    NETHER_PORTAL = &registry.registerBlock<blocks::NetherPortalBlock>(
        ResourceLocation("minecraft:nether_portal"),
        BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(11)
    );

    // 下界疣 - 作物方块
    // 参考: new NetherWartBlock(Properties.create(Material.PLANT).doesNotBlockMovement().zeroHardnessAndResistance())
    NETHER_WART = &registry.registerBlock<blocks::NetherWartBlock>(
        ResourceLocation("minecraft:nether_wart"),
        BlockProperties(Material::PLANT).noCollision().hardness(0.0f)
    );
}

// ============================================================================
// 树木变种注册
// ============================================================================
void VanillaBlocks::registerTreeVariants() {
    auto& registry = BlockRegistry::instance();

    // 木头属性：完全不透明
    BlockProperties logProps = BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable();

    // 树叶属性：参考 Java 1.16.5 LeavesBlock#getOpacity() = 1
    // 光线穿过树叶每层衰减 1 级，避免树荫过黑。
    BlockProperties leavesProps = BlockProperties(Material::LEAVES)
        .hardness(0.2f).flammable().notSolid().opacity(1).propagatesSkylightDown();

    // 云杉原木和树叶
    SPRUCE_LOG = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:spruce_log"), logProps);
    SPRUCE_LEAVES = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:spruce_leaves"), leavesProps);

    // 白桦原木和树叶
    BIRCH_LOG = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:birch_log"), logProps);
    BIRCH_LEAVES = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:birch_leaves"), leavesProps);

    // 丛林原木和树叶
    JUNGLE_LOG = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:jungle_log"), logProps);
    JUNGLE_LEAVES = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:jungle_leaves"), leavesProps);

    // 金合欢原木和树叶
    ACACIA_LOG = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:acacia_log"), logProps);
    ACACIA_LEAVES = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:acacia_leaves"), leavesProps);

    // 深色橡木原木和树叶
    DARK_OAK_LOG = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:dark_oak_log"), logProps);
    DARK_OAK_LEAVES = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dark_oak_leaves"), leavesProps);
}

// ============================================================================
// 植被方块注册
// ============================================================================
void VanillaBlocks::registerVegetationBlocks() {
    auto& registry = BlockRegistry::instance();

    // 草和蕨的属性
    BlockProperties grassProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid();

    // 矮草 - ID 51
    // 参考: new TallGrassBlock(Properties.create(Material.REPLACEABLE_PLANT).doesNotBlockMovement().zeroHardnessAndResistance())
    SHORT_GRASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:short_grass"), grassProps);

    // 高草 - ID 52
    TALL_GRASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:tall_grass"), grassProps);

    // 蕨 - ID 53
    FERN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:fern"), grassProps);

    // 花朵属性
    BlockProperties flowerProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid();

    // 蒲公英 - ID 54
    DANDELION = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dandelion"), flowerProps);

    // 虞美人 - ID 55
    POPPY = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:poppy"), flowerProps);

    // 兰花 - ID 56
    BLUE_ORCHID = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blue_orchid"), flowerProps);

    // 绒球葱 - ID 57
    ALLIUM = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:allium"), flowerProps);

    // 蓝花美耳草 - ID 58
    AZURE_BLUET = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:azure_bluet"), flowerProps);

    // 郁金香系列 - ID 59-62
    RED_TULIP = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_tulip"), flowerProps);
    ORANGE_TULIP = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:orange_tulip"), flowerProps);
    WHITE_TULIP = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:white_tulip"), flowerProps);
    PINK_TULIP = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:pink_tulip"), flowerProps);

    // 滨菊 - ID 63
    OXEYE_DAISY = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:oxeye_daisy"), flowerProps);

    // 铃兰
    LILY_OF_THE_VALLEY = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lily_of_the_valley"), flowerProps);

    // 矢车菊
    CORNFLOWER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cornflower"), flowerProps);

    // 凋零玫瑰
    WITHER_ROSE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:wither_rose"), flowerProps);

    // 高花属性（双高植物）
    BlockProperties tallFlowerProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid();

    // 向日葵
    SUNFLOWER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sunflower"), tallFlowerProps);

    // 丁香
    LILAC = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lilac"), tallFlowerProps);

    // 玫瑰丛
    ROSE_BUSH = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:rose_bush"), tallFlowerProps);

    // 牡丹
    PEONY = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:peony"), tallFlowerProps);

    // 蘑菇属性
    BlockProperties mushroomProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid().lightLevel(1);

    // 棕色蘑菇 - ID 64
    BROWN_MUSHROOM = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:brown_mushroom"), mushroomProps);

    // 红色蘑菇 - ID 65
    RED_MUSHROOM = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_mushroom"), mushroomProps);

    // 巨型蘑菇方块属性
    BlockProperties hugeMushroomProps = BlockProperties(Material::WOOD).hardness(0.2f);

    // 棕色蘑菇方块
    BROWN_MUSHROOM_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:brown_mushroom_block"), hugeMushroomProps);

    // 红色蘑菇方块
    RED_MUSHROOM_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_mushroom_block"), hugeMushroomProps);

    // 蘑菇柄
    MUSHROOM_STEM = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:mushroom_stem"), hugeMushroomProps);

    // 树苗属性
    BlockProperties saplingProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid();

    // 橡树树苗 - 已在 registerLogBlocks 中注册，这里不需要重复
    // 但我们需要添加其他树苗
    OAK_SAPLING = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:oak_sapling"), saplingProps);
    SPRUCE_SAPLING = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:spruce_sapling"), saplingProps);
    BIRCH_SAPLING = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:birch_sapling"), saplingProps);
    JUNGLE_SAPLING = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:jungle_sapling"), saplingProps);
    ACACIA_SAPLING = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:acacia_sapling"), saplingProps);
    DARK_OAK_SAPLING = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dark_oak_sapling"), saplingProps);
}

// ============================================================================
// 彩色方块注册（染色玻璃、混凝土、混凝土粉末、陶瓦）
// ============================================================================
void VanillaBlocks::registerColoredBlocks() {
    auto& registry = BlockRegistry::instance();

    // 染色玻璃属性
    // 参考: new GlassBlock(Properties.create(Material.GLASS).hardnessAndResistance(0.3F).notSolid())
    // 染色玻璃：透明度0，不传播天空光
    BlockProperties stainedGlassProps = BlockProperties(Material::GLASS).hardness(0.3f).notSolid();

    WHITE_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:white_stained_glass"), stainedGlassProps);
    ORANGE_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:orange_stained_glass"), stainedGlassProps);
    MAGENTA_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:magenta_stained_glass"), stainedGlassProps);
    LIGHT_BLUE_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_blue_stained_glass"), stainedGlassProps);
    YELLOW_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:yellow_stained_glass"), stainedGlassProps);
    LIME_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lime_stained_glass"), stainedGlassProps);
    PINK_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:pink_stained_glass"), stainedGlassProps);
    GRAY_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gray_stained_glass"), stainedGlassProps);
    LIGHT_GRAY_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_gray_stained_glass"), stainedGlassProps);
    CYAN_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cyan_stained_glass"), stainedGlassProps);
    PURPLE_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:purple_stained_glass"), stainedGlassProps);
    BLUE_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blue_stained_glass"), stainedGlassProps);
    BROWN_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:brown_stained_glass"), stainedGlassProps);
    GREEN_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:green_stained_glass"), stainedGlassProps);
    RED_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_stained_glass"), stainedGlassProps);
    BLACK_STAINED_GLASS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:black_stained_glass"), stainedGlassProps);

    // 混凝土属性
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(1.8F))
    BlockProperties concreteProps = BlockProperties(Material::ROCK).hardness(1.8f).resistance(1.8f);

    WHITE_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:white_concrete"), concreteProps);
    ORANGE_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:orange_concrete"), concreteProps);
    MAGENTA_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:magenta_concrete"), concreteProps);
    LIGHT_BLUE_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_blue_concrete"), concreteProps);
    YELLOW_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:yellow_concrete"), concreteProps);
    LIME_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lime_concrete"), concreteProps);
    PINK_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:pink_concrete"), concreteProps);
    GRAY_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gray_concrete"), concreteProps);
    LIGHT_GRAY_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_gray_concrete"), concreteProps);
    CYAN_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cyan_concrete"), concreteProps);
    PURPLE_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:purple_concrete"), concreteProps);
    BLUE_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blue_concrete"), concreteProps);
    BROWN_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:brown_concrete"), concreteProps);
    GREEN_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:green_concrete"), concreteProps);
    RED_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_concrete"), concreteProps);
    BLACK_CONCRETE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:black_concrete"), concreteProps);

    // 混凝土粉末属性
    // 参考: new ConcretePowderBlock(Properties.create(Material.SAND).hardnessAndResistance(0.5F))
    BlockProperties concretePowderProps = BlockProperties(Material::SAND).hardness(0.5f);

    WHITE_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:white_concrete_powder"), concretePowderProps);
    ORANGE_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:orange_concrete_powder"), concretePowderProps);
    MAGENTA_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:magenta_concrete_powder"), concretePowderProps);
    LIGHT_BLUE_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_blue_concrete_powder"), concretePowderProps);
    YELLOW_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:yellow_concrete_powder"), concretePowderProps);
    LIME_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lime_concrete_powder"), concretePowderProps);
    PINK_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:pink_concrete_powder"), concretePowderProps);
    GRAY_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gray_concrete_powder"), concretePowderProps);
    LIGHT_GRAY_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_gray_concrete_powder"), concretePowderProps);
    CYAN_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cyan_concrete_powder"), concretePowderProps);
    PURPLE_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:purple_concrete_powder"), concretePowderProps);
    BLUE_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blue_concrete_powder"), concretePowderProps);
    BROWN_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:brown_concrete_powder"), concretePowderProps);
    GREEN_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:green_concrete_powder"), concretePowderProps);
    RED_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_concrete_powder"), concretePowderProps);
    BLACK_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:black_concrete_powder"), concretePowderProps);

    // 陶瓦属性
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(1.4F, 4.2F))
    BlockProperties terracottaProps = BlockProperties(Material::ROCK).hardness(1.4f).resistance(4.2f);

    // 普通陶瓦
    TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:terracotta"), terracottaProps);

    // 染色陶瓦 (16色)
    WHITE_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:white_terracotta"), terracottaProps);
    ORANGE_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:orange_terracotta"), terracottaProps);
    MAGENTA_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:magenta_terracotta"), terracottaProps);
    LIGHT_BLUE_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_blue_terracotta"), terracottaProps);
    YELLOW_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:yellow_terracotta"), terracottaProps);
    LIME_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lime_terracotta"), terracottaProps);
    PINK_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:pink_terracotta"), terracottaProps);
    GRAY_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gray_terracotta"), terracottaProps);
    LIGHT_GRAY_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_gray_terracotta"), terracottaProps);
    CYAN_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cyan_terracotta"), terracottaProps);
    PURPLE_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:purple_terracotta"), terracottaProps);
    BLUE_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blue_terracotta"), terracottaProps);
    BROWN_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:brown_terracotta"), terracottaProps);
    GREEN_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:green_terracotta"), terracottaProps);
    RED_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_terracotta"), terracottaProps);
    BLACK_TERRACOTTA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:black_terracotta"), terracottaProps);
}

// ============================================================================
// 石砖系列注册
// ============================================================================
void VanillaBlocks::registerStoneBricks() {
    auto& registry = BlockRegistry::instance();

    // 石砖属性
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(1.5F, 6.0F))
    BlockProperties stoneBrickProps = BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f);

    STONE_BRICKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:stone_bricks"), stoneBrickProps);
    MOSSY_STONE_BRICKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:mossy_stone_bricks"), stoneBrickProps);
    CRACKED_STONE_BRICKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cracked_stone_bricks"), stoneBrickProps);
    CHISELED_STONE_BRICKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:chiseled_stone_bricks"), stoneBrickProps);
}

// ============================================================================
// 石英系列注册
// ============================================================================
void VanillaBlocks::registerQuartzBlocks() {
    auto& registry = BlockRegistry::instance();

    // 石英块属性
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(0.8F))
    BlockProperties quartzProps = BlockProperties(Material::ROCK).hardness(0.8f);

    QUARTZ_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:quartz_block"), quartzProps);
    CHISELED_QUARTZ_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:chiseled_quartz_block"), quartzProps);

    // 石英柱 - 有轴属性
    QUARTZ_PILLAR = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:quartz_pillar"), quartzProps);
    // 注：下界石英矿 NETHER_QUARTZ_ORE 已在 registerNetherBlocks() 中注册
}

// ============================================================================
// 海晶系列注册
// ============================================================================
void VanillaBlocks::registerPrismarineBlocks() {
    auto& registry = BlockRegistry::instance();

    // 海晶石属性
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(1.5F, 6.0F))
    BlockProperties prismarineProps = BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f);

    PRISMARINE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:prismarine"), prismarineProps);
    PRISMARINE_BRICKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:prismarine_bricks"), prismarineProps);
    DARK_PRISMARINE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dark_prismarine"), prismarineProps);

    // 海晶灯 - 发光15级
    // 参考: new Block(Properties.create(Material.GLASS).hardnessAndResistance(0.3F).setLightLevel(15))
    SEA_LANTERN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sea_lantern"),
        BlockProperties(Material::GLASS).hardness(0.3f).lightLevel(15));
}

// ============================================================================
// 紫珀系列注册
// ============================================================================
void VanillaBlocks::registerPurpurBlocks() {
    auto& registry = BlockRegistry::instance();

    // 紫珀块属性
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(1.5F, 6.0F))
    BlockProperties purpurProps = BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f);

    PURPUR_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:purpur_block"), purpurProps);

    // 紫珀柱 - 有轴属性
    PURPUR_PILLAR = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:purpur_pillar"), purpurProps);
}

// ============================================================================
// 末地方块注册
// ============================================================================
void VanillaBlocks::registerEndBlocks() {
    auto& registry = BlockRegistry::instance();

    // 末地石砖属性
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(3.0F, 9.0F))
    BlockProperties endStoneBrickProps = BlockProperties(Material::ROCK).hardness(3.0f).resistance(9.0f);

    END_STONE_BRICKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:end_stone_bricks"), endStoneBrickProps);

    // 末地烛 - 发光14级
    // 参考: new EndRodBlock(Properties.create(Material.DECORATION).hardnessAndResistance(0.0F).setLightLevel(14).noCollision())
    END_ROD = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:end_rod"),
        BlockProperties(Material::DECORATION).noCollision().lightLevel(14));

    // 末地传送门 - 穿越后传送到末地
    // 参考: new EndPortalBlock(Properties.create(Material.PORTAL).doesNotBlockMovement().zeroHardnessAndResistance().setLightLevel(15))
    END_PORTAL = &registry.registerBlock<blocks::EndPortalBlock>(
        ResourceLocation("minecraft:end_portal"),
        BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(15)
    );

    // 末地传送门框架 - 放置末影之眼激活传送门
    // 参考: new EndPortalFrameBlock(Properties.create(Material.ROCK).hardnessAndResistance(-1.0F, 3600000.0F).setLightLevel(1))
    END_PORTAL_FRAME = &registry.registerBlock<blocks::EndPortalFrameBlock>(
        ResourceLocation("minecraft:end_portal_frame"),
        BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).lightLevel(1)
    );

    // 末地折跃门 - 在末地之间传送
    // 参考: new EndGatewayBlock(Properties.create(Material.PORTAL).doesNotBlockMovement().zeroHardnessAndResistance().setLightLevel(15))
    END_GATEWAY = &registry.registerBlock<blocks::EndGatewayBlock>(
        ResourceLocation("minecraft:end_gateway"),
        BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(15)
    );

    // 紫颂植物 - 末地植物
    // 参考: new ChorusPlantBlock(Properties.create(Material.PLANT).doesNotBlockMovement().zeroHardnessAndResistance())
    CHORUS_PLANT = &registry.registerBlock<blocks::ChorusPlantBlock>(
        ResourceLocation("minecraft:chorus_plant"),
        BlockProperties(Material::PLANT).noCollision().hardness(0.0f)
    );

    // 紫颂花 - 紫颂植物的顶部
    // 参考: new ChorusFlowerBlock(Properties.create(Material.PLANT).doesNotBlockMovement().zeroHardnessAndResistance())
    CHORUS_FLOWER = &registry.registerBlock<blocks::ChorusFlowerBlock>(
        ResourceLocation("minecraft:chorus_flower"),
        BlockProperties(Material::PLANT).noCollision().hardness(0.0f)
    );

    // 龙蛋 - 末影龙掉落物
    // 参考: new DragonEggBlock(Properties.create(Material.DRAGON_EGG).hardnessAndResistance(3.0F).setLightLevel(1))
    DRAGON_EGG = &registry.registerBlock<blocks::DragonEggBlock>(
        ResourceLocation("minecraft:dragon_egg"),
        BlockProperties(Material::ROCK).hardness(3.0f).lightLevel(1)
    );

    // 信标 - 发光15级（通过 getLightLevel）
    // 参考: new BeaconBlock(Properties.create(Material.GLASS).setLightLevel(15))
    BEACON = &registry.registerBlock<blocks::BeaconBlock>(
        ResourceLocation("minecraft:beacon"),
        BlockProperties(Material::GLASS).hardness(3.0f)
    );

    // 酿造台 - 发光1级（通过 getLightLevel）
    // 参考: new BrewingStandBlock(Properties.create(Material.IRON).setLightLevel(1))
    BREWING_STAND = &registry.registerBlock<blocks::BrewingStandBlock>(
        ResourceLocation("minecraft:brewing_stand"),
        BlockProperties(Material::IRON).hardness(0.5f)
    );

    // 末影箱 - 发光7级
    // 参考: new EnderChestBlock(Properties.create(Material.ROCK).hardnessAndResistance(22.5F).setLightLevel(7))
    ENDER_CHEST = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:ender_chest"),
        BlockProperties(Material::ROCK).hardness(22.5f).resistance(600.0f).lightLevel(7)
    );

    // 灯笼 - 发光15级（通过构造函数参数）
    // 参考: new LanternBlock(Properties.create(Material.IRON).hardnessAndResistance(3.5F).setLightLevel(15))
    LANTERN = &registry.registerBlock<blocks::LanternBlock>(
        ResourceLocation("minecraft:lantern"),
        BlockProperties(Material::IRON).hardness(3.5f).resistance(3.5f),
        15  // 光照等级
    );

    // 灵魂灯笼 - 发光10级（通过构造函数参数）
    // 参考: new LanternBlock(Properties.create(Material.IRON).hardnessAndResistance(3.5F).setLightLevel(10))
    SOUL_LANTERN = &registry.registerBlock<blocks::LanternBlock>(
        ResourceLocation("minecraft:soul_lantern"),
        BlockProperties(Material::IRON).hardness(3.5f).resistance(3.5f),
        10  // 光照等级
    );

    // 营火 - 发光15级（点燃时，通过 getLightLevel 动态计算）
    // 参考: new CampfireBlock(Properties.create(Material.WOOD).hardness(2.0F).setLightLevel(15))
    CAMPFIRE = &registry.registerBlock<blocks::CampfireBlock>(
        ResourceLocation("minecraft:campfire"),
        BlockProperties(Material::WOOD).hardness(2.0f),
        15  // 点燃时光照等级
    );

    // 灵魂营火 - 发光10级（点燃时，通过 getLightLevel 动态计算）
    // 参考: new CampfireBlock(Properties.create(Material.WOOD).hardness(2.0F).setLightLevel(10))
    SOUL_CAMPFIRE = &registry.registerBlock<blocks::SoulCampfireBlock>(
        ResourceLocation("minecraft:soul_campfire"),
        BlockProperties(Material::WOOD).hardness(2.0f)
    );

    // 南瓜灯 - 发光15级
    // 参考: new Block(Properties.create(Material.GOURD).hardness(1.0F).setLightLevel(15))
    JACK_O_LANTERN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:jack_o_lantern"),
        BlockProperties(Material::EARTH).hardness(1.0f).lightLevel(15)
    );
}

// ============================================================================
// 骨块和干草块注册
// ============================================================================
void VanillaBlocks::registerBoneAndHayBlocks() {
    auto& registry = BlockRegistry::instance();

    // 骨块 - 有轴属性
    // 参考: new RotatedPillarBlock(Properties.create(Material.ROCK).hardnessAndResistance(2.0F, 2.0F))
    BONE_BLOCK = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:bone_block"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));

    // 干草块 - 有轴属性
    // 参考: new RotatedPillarBlock(Properties.create(Material.ORGANIC).hardnessAndResistance(0.5F))
    HAY_BLOCK = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:hay_block"),
        BlockProperties(Material::EARTH).hardness(0.5f).flammable());
}

// ============================================================================
// 下界扩展方块注册（岩浆块、地狱疣块等）
// ============================================================================
void VanillaBlocks::registerNetherExtensionBlocks() {
    auto& registry = BlockRegistry::instance();

    // 岩浆块 - 发光3级
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(0.5F).setLightLevel(3))
    MAGMA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:magma"),
        BlockProperties(Material::ROCK).hardness(0.5f).lightLevel(3));

    // 地狱疣块
    // 参考: new Block(Properties.create(Material.ORGANIC).hardnessAndResistance(1.0F))
    NETHER_WART_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:nether_wart_block"),
        BlockProperties(Material::EARTH).hardness(1.0f));

    // 绯红菌柄
    CRIMSON_STEM = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:crimson_stem"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 诡异菌柄
    WARPED_STEM = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:warped_stem"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 绯红菌岩
    CRIMSON_NYLIUM = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:crimson_nylium"),
        BlockProperties(Material::ROCK).hardness(0.4f).resistance(0.4f));

    // 诡异菌岩
    WARPED_NYLIUM = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:warped_nylium"),
        BlockProperties(Material::ROCK).hardness(0.4f).resistance(0.4f));

    // 菌光体
    SHROOMLIGHT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:shroomlight"),
        BlockProperties(Material::EARTH).hardness(1.0f).lightLevel(15));

    // 绯红菌
    CRIMSON_FUNGUS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:crimson_fungus"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 诡异菌
    WARPED_FUNGUS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:warped_fungus"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 垂泪藤
    WEEPING_VINES = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:weeping_vines"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 扭曲藤
    TWISTING_VINES = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:twisting_vines"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());
}

// ============================================================================
// 自然扩展方块注册
// ============================================================================
void VanillaBlocks::registerNaturalBlocks() {
    auto& registry = BlockRegistry::instance();

    // 粘土
    // 参考: new Block(Properties.create(Material.EARTH).hardnessAndResistance(0.6F))
    CLAY = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:clay"),
        BlockProperties(Material::EARTH).hardness(0.6f));

    // 菌丝
    // 参考: new SnowyDirtBlock(Properties.create(Material.EARTH).hardnessAndResistance(0.6F))
    MYCELIUM = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:mycelium"),
        BlockProperties(Material::EARTH).hardness(0.6f));

    // 草径
    // 参考: new Block(Properties.create(Material.EARTH).hardnessAndResistance(0.65F))
    GRASS_PATH = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:grass_path"),
        BlockProperties(Material::EARTH).hardness(0.65f));

    // 浮冰 - ID 174
    // 参考: new Block(Properties.create(Material.ICE).hardnessAndResistance(0.5F))
    // 浮冰：不透明，不融化
    PACKED_ICE = &registry.registerBlock<blocks::PackedIceBlock>(
        ResourceLocation("minecraft:packed_ice"),
        BlockProperties(Material::ICE).hardness(0.5f).opacity(2).propagatesSkylightDown());

    // 蓝冰 - ID 266
    // 参考: new Block(Properties.create(Material.ICE).hardnessAndResistance(2.8F))
    // 蓝冰：最滑的方块，摩擦力0.989
    BLUE_ICE = &registry.registerBlock<blocks::BlueIceBlock>(
        ResourceLocation("minecraft:blue_ice"),
        BlockProperties(Material::ICE).hardness(2.8f).resistance(2.8f));

    // 霜冰 - ID 212
    // 由冰霜行者附魔生成的临时冰
    // 霜冰：透明，会在光源附近融化
    FROSTED_ICE = &registry.registerBlock<blocks::FrostedIceBlock>(
        ResourceLocation("minecraft:frosted_ice"),
        BlockProperties(Material::ICE).hardness(0.5f).notSolid().opacity(2).propagatesSkylightDown());

    // 粘液块
    // 参考: new Block(Properties.create(Material.SLIME).hardnessAndResistance(0.0F).slipperiness(0.8F))
    SLIME_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:slime_block"),
        BlockProperties(Material::SLIME).hardness(0.0f));

    // 仙人掌
    // 参考: new CactusBlock(Properties.create(Material.CACTUS).hardnessAndResistance(0.4F).noCollision())
    CACTUS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cactus"),
        BlockProperties(Material::PLANT).hardness(0.4f).noCollision());

    // 枯萎灌木
    // 参考: new BushBlock(Properties.create(Material.REPLACEABLE_PLANT).zeroHardnessAndResistance().noCollision())
    DEAD_BUSH = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dead_bush"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 睡莲
    // 参考: new LilyPadBlock(Properties.create(Material.PLANT).hardnessAndResistance(0.0F).noCollision())
    LILY_PAD = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lily_pad"),
        BlockProperties(Material::PLANT).noCollision().notSolid());

    // 藤蔓
    // 参考: new VineBlock(Properties.create(Material.REPLACEABLE_PLANT).hardnessAndResistance(0.2F).noCollision())
    VINE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:vine"),
        BlockProperties(Material::REPLACEABLE_PLANT).hardness(0.2f).noCollision().notSolid());

    // 蜘蛛网
    // 参考: new WebBlock(Properties.create(Material.WEB).hardnessAndResistance(4.0F))
    COBWEB = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cobweb"),
        BlockProperties(Material::WEB).hardness(4.0f).noCollision());

    // 甘蔗
    // 参考: new SugarCaneBlock(Properties.create(Material.REPLACEABLE_PLANT).zeroHardnessAndResistance().noCollision())
    SUGAR_CANE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sugar_cane"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 耕地
    FARMLAND = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:farmland"),
        BlockProperties(Material::EARTH).hardness(0.6f));

    // 红沙
    RED_SAND = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_sand"),
        BlockProperties(Material::SAND).hardness(0.5f));

    // 干海带块 - ID 171
    // 可以作为燃料使用，燃烧时间200tick（10秒）
    // 参考: new Block(Properties.create(Material.PLANT).hardnessAndResistance(0.5F))
    DRIED_KELP_BLOCK = &registry.registerBlock<blocks::DriedKelpBlock>(
        ResourceLocation("minecraft:dried_kelp_block"),
        BlockProperties(Material::PLANT).hardness(0.5f).resistance(0.5f));

    // 潮涌核心 - ID 545
    // 水下信标类方块，需要潮涌框架激活
    // 参考: new ConduitBlock(Properties.create(Material.GLASS).hardnessAndResistance(3.0F).notSolid())
    CONDUIT = &registry.registerBlock<blocks::ConduitBlock>(
        ResourceLocation("minecraft:conduit"),
        BlockProperties(Material::GLASS).hardness(3.0f).resistance(3.0f).notSolid());
}

// ============================================================================
// 红石方块注册
// ============================================================================
void VanillaBlocks::registerRedstoneBlocks() {
    auto& registry = BlockRegistry::instance();

    // 红石线
    // 参考: new RedstoneWireBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().zeroHardnessAndResistance())
    REDSTONE_WIRE = &registry.registerBlock<blocks::RedstoneWireBlock>(
        ResourceLocation("minecraft:redstone_wire"),
        BlockProperties(Material::DECORATION).noCollision().notSolid()
    );

    // 红石火把
    // 参考: new RedstoneTorchBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().zeroHardnessAndResistance().setLightLevel(7))
    REDSTONE_TORCH = &registry.registerBlock<blocks::RedstoneTorchBlock>(
        ResourceLocation("minecraft:redstone_torch"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(7)
    );

    // 墙上的红石火把
    REDSTONE_WALL_TORCH = &registry.registerBlock<blocks::RedstoneWallTorchBlock>(
        ResourceLocation("minecraft:redstone_wall_torch"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(7)
    );

    // 红石灯
    // 参考: new RedstoneLampBlock(Properties.create(Material.REDSTONE_LIGHT).hardnessAndResistance(0.3F))
    REDSTONE_LAMP = &registry.registerBlock<blocks::RedstoneLampBlock>(
        ResourceLocation("minecraft:redstone_lamp"),
        BlockProperties(Material::REDSTONE_LIGHT).hardness(0.3f)
    );

    // 红石中继器
    // 参考: new RepeaterBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().zeroHardnessAndResistance())
    REDSTONE_REPEATER = &registry.registerBlock<blocks::RedstoneRepeaterBlock>(
        ResourceLocation("minecraft:repeater"),
        BlockProperties(Material::DECORATION).noCollision().notSolid()
    );

    // 红石比较器
    // 参考: new ComparatorBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().zeroHardnessAndResistance())
    REDSTONE_COMPARATOR = &registry.registerBlock<blocks::RedstoneComparatorBlock>(
        ResourceLocation("minecraft:comparator"),
        BlockProperties(Material::DECORATION).noCollision().notSolid()
    );

    // 侦测器
    // 参考: new ObserverBlock(Properties.create(Material.ROCK).hardnessAndResistance(3.0F))
    OBSERVER = &registry.registerBlock<blocks::ObserverBlock>(
        ResourceLocation("minecraft:observer"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 拉杆
    // 参考: new LeverBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().zeroHardnessAndResistance())
    LEVER = &registry.registerBlock<blocks::LeverBlock>(
        ResourceLocation("minecraft:lever"),
        BlockProperties(Material::DECORATION).noCollision().notSolid()
    );

    // 石头按钮
    // 参考: new StoneButtonBlock(Properties.create(Material.ROCK).doesNotBlockMovement().zeroHardnessAndResistance())
    STONE_BUTTON = &registry.registerBlock<blocks::StoneButtonBlock>(
        ResourceLocation("minecraft:stone_button"),
        BlockProperties(Material::ROCK).noCollision().notSolid()
    );

    // 橡木按钮
    // 参考: new WoodButtonBlock(Properties.create(Material.WOOD).doesNotBlockMovement().zeroHardnessAndResistance())
    OAK_BUTTON = &registry.registerBlock<blocks::WoodButtonBlock>(
        ResourceLocation("minecraft:oak_button"),
        BlockProperties(Material::WOOD).noCollision().notSolid().flammable()
    );

    // 石头压力板
    // 参考: new PressurePlateBlock(PressurePlateBlock.Sensitivity.EVERYTHING, Properties.create(Material.ROCK).doesNotBlockMovement().hardnessAndResistance(0.5F))
    STONE_PRESSURE_PLATE = &registry.registerBlock<blocks::StonePressurePlateBlock>(
        ResourceLocation("minecraft:stone_pressure_plate"),
        BlockProperties(Material::ROCK).noCollision().notSolid().hardness(0.5f)
    );

    // 橡木压力板
    // 参考: new PressurePlateBlock(PressurePlateBlock.Sensitivity.EVERYTHING, Properties.create(Material.WOOD).doesNotBlockMovement().hardnessAndResistance(0.5F))
    OAK_PRESSURE_PLATE = &registry.registerBlock<blocks::WoodPressurePlateBlock>(
        ResourceLocation("minecraft:oak_pressure_plate"),
        BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable()
    );

    // 轻质测重压力板
    // 参考: new WeightedPressurePlateBlock(1, Properties.create(Material.IRON).doesNotBlockMovement().hardnessAndResistance(0.5F))
    LIGHT_WEIGHTED_PRESSURE_PLATE = &registry.registerBlock<blocks::WeightedPressurePlateBlock>(
        ResourceLocation("minecraft:light_weighted_pressure_plate"),
        BlockProperties(Material::IRON).noCollision().notSolid().hardness(0.5f),
        blocks::WeightedPressurePlateBlock::Sensitivity::Light
    );

    // 重质测重压力板
    // 参考: new WeightedPressurePlateBlock(10, Properties.create(Material.IRON).doesNotBlockMovement().hardnessAndResistance(0.5F))
    HEAVY_WEIGHTED_PRESSURE_PLATE = &registry.registerBlock<blocks::WeightedPressurePlateBlock>(
        ResourceLocation("minecraft:heavy_weighted_pressure_plate"),
        BlockProperties(Material::IRON).noCollision().notSolid().hardness(0.5f),
        blocks::WeightedPressurePlateBlock::Sensitivity::Heavy
    );

    // 日光探测器
    // 参考: new DaylightDetectorBlock(Properties.create(Material.WOOD).hardnessAndResistance(0.2F))
    DAYLIGHT_DETECTOR = &registry.registerBlock<blocks::DaylightDetectorBlock>(
        ResourceLocation("minecraft:daylight_detector"),
        BlockProperties(Material::WOOD).hardness(0.2f).flammable()
    );

    // 活塞
    // 参考: new PistonBlock(false, Properties.create(Material.PISTON).hardnessAndResistance(0.5F))
    PISTON = &registry.registerBlock<blocks::PistonBlock>(
        ResourceLocation("minecraft:piston"),
        BlockProperties(Material::PISTON).hardness(0.5f).resistance(0.5f),
        false  // not sticky
    );

    // 粘性活塞
    // 参考: new PistonBlock(true, Properties.create(Material.PISTON).hardnessAndResistance(0.5F))
    STICKY_PISTON = &registry.registerBlock<blocks::PistonBlock>(
        ResourceLocation("minecraft:sticky_piston"),
        BlockProperties(Material::PISTON).hardness(0.5f).resistance(0.5f),
        true  // sticky
    );

    // 活塞头
    PISTON_HEAD = &registry.registerBlock<blocks::PistonHeadBlock>(
        ResourceLocation("minecraft:piston_head"),
        BlockProperties(Material::PISTON).hardness(0.5f).resistance(0.5f)
    );

    // 移动中的活塞
    // 参考: new MovingPistonBlock(Properties.create(Material.PISTON).hardnessAndResistance(-1.0F))
    // MC Java: 移动中的活塞是不可破坏的，硬度为 -1.0
    MOVING_PISTON = &registry.registerBlock<blocks::MovingPistonBlock>(
        ResourceLocation("minecraft:moving_piston"),
        BlockProperties(Material::PISTON).hardness(-1.0f).resistance(-1.0f)
    );

    // 发射器
    // 参考: new DispenserBlock(Properties.create(Material.ROCK).hardnessAndResistance(3.0F))
    DISPENSER = &registry.registerBlock<blocks::DispenserBlock>(
        ResourceLocation("minecraft:dispenser"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 投掷器
    // 参考: new DropperBlock(Properties.create(Material.ROCK).hardnessAndResistance(3.0F))
    DROPPER = &registry.registerBlock<blocks::DropperBlock>(
        ResourceLocation("minecraft:dropper"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 音符盒
    // 参考: new NoteBlock(Properties.create(Material.WOOD).hardnessAndResistance(0.8F))
    NOTE_BLOCK = &registry.registerBlock<blocks::NoteBlock>(
        ResourceLocation("minecraft:note_block"),
        BlockProperties(Material::WOOD).hardness(0.8f).flammable()
    );

    // TNT
    // 参考: new TNTBlock(Properties.create(Material.TNT).hardnessAndResistance(0.0F, 0.0F))
    TNT = &registry.registerBlock<blocks::TNTBlock>(
        ResourceLocation("minecraft:tnt"),
        BlockProperties(Material::TNT).hardness(0.0f)
    );

    // 标靶
    // 参考: new TargetBlock(Properties.create(Material.WOOL).hardnessAndResistance(0.5F))
    TARGET = &registry.registerBlock<blocks::TargetBlock>(
        ResourceLocation("minecraft:target"),
        BlockProperties(Material::WOOL).hardness(0.5f)
    );

    // 绊线
    // 参考: new TripWireBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().zeroHardnessAndResistance())
    TRIPWIRE = &registry.registerBlock<blocks::TripWireBlock>(
        ResourceLocation("minecraft:tripwire"),
        BlockProperties(Material::DECORATION).noCollision().notSolid()
    );

    // 绊线钩
    // 参考: new TripWireHookBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().zeroHardnessAndResistance())
    TRIPWIRE_HOOK = &registry.registerBlock<blocks::TripWireHookBlock>(
        ResourceLocation("minecraft:tripwire_hook"),
        BlockProperties(Material::DECORATION).noCollision().notSolid()
    );

    // 普通铁轨
    // 参考: new RailBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().hardnessAndResistance(0.7F))
    RAIL = &registry.registerBlock<blocks::RailBlock>(
        ResourceLocation("minecraft:rail"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().hardness(0.7f)
    );

    // 动力铁轨
    // 参考: new PoweredRailBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().hardnessAndResistance(0.7F))
    POWERED_RAIL = &registry.registerBlock<blocks::PoweredRailBlock>(
        ResourceLocation("minecraft:powered_rail"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().hardness(0.7f)
    );

    // 探测铁轨
    // 参考: new DetectorRailBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().hardnessAndResistance(0.7F))
    DETECTOR_RAIL = &registry.registerBlock<blocks::DetectorRailBlock>(
        ResourceLocation("minecraft:detector_rail"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().hardness(0.7f)
    );

    // 激活铁轨
    // 参考: new ActivatorRailBlock(Properties.create(Material.MISCELLANEOUS).doesNotBlockMovement().hardnessAndResistance(0.7F))
    ACTIVATOR_RAIL = &registry.registerBlock<blocks::ActivatorRailBlock>(
        ResourceLocation("minecraft:activator_rail"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().hardness(0.7f)
    );
}

// ============================================================================
// 楼梯、台阶、墙、栅栏、活板门注册
// ============================================================================
void VanillaBlocks::registerStairsSlabsWalls() {
    auto& registry = BlockRegistry::instance();

    // ========== 楼梯 ==========
    // 橡木楼梯
    // 参考: new StairsBlock(OAK_PLANKS.defaultBlockState(), Properties.create(Material.WOOD).hardnessAndResistance(2.0F))
    OAK_STAIRS = &registry.registerBlock<blocks::StairsBlock>(
        ResourceLocation("minecraft:oak_stairs"),
        OAK_PLANKS->defaultState(),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable()
    );

    // 石头楼梯
    STONE_STAIRS = &registry.registerBlock<blocks::StairsBlock>(
        ResourceLocation("minecraft:stone_stairs"),
        STONE->defaultState(),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe)
    );

    // 圆石楼梯
    COBBLESTONE_STAIRS = &registry.registerBlock<blocks::StairsBlock>(
        ResourceLocation("minecraft:cobblestone_stairs"),
        COBBLESTONE->defaultState(),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe)
    );

    // ========== 台阶 ==========
    // 橡木台阶
    // 参考: new SlabBlock(Properties.create(Material.WOOD).hardnessAndResistance(2.0F))
    OAK_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:oak_slab"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable()
    );

    // 石头台阶
    STONE_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:stone_slab"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe)
    );

    // 圆石台阶
    COBBLESTONE_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:cobblestone_slab"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe)
    );

    // ========== 墙 ==========
    // 圆石墙
    // 参考: new WallBlock(Properties.create(Material.ROCK).doesNotBlockMovement().hardnessAndResistance(1.5F).setRequiresTool())
    COBBLESTONE_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:cobblestone_wall"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe)
    );

    // 石砖墙
    STONE_BRICK_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:stone_brick_wall"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe)
    );

    // ========== 栅栏 ==========
    // 橡木栅栏
    // 参考: new FenceBlock(Properties.create(Material.WOOD).hardnessAndResistance(2.0F, 3.0F))
    OAK_FENCE = &registry.registerBlock<blocks::FenceBlock>(
        ResourceLocation("minecraft:oak_fence"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable()
    );

    // ========== 活板门 ==========
    // 橡木活板门
    // 参考: new TrapDoorBlock(Properties.create(Material.WOOD).hardnessAndResistance(3.0F).noCollission())
    OAK_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:oak_trapdoor"),
        BlockProperties(Material::WOOD).hardness(3.0f).resistance(3.0f).flammable(),
        false  // not iron
    );

    // 铁活板门
    IRON_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:iron_trapdoor"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f).harvestTool(HarvestTool::Pickaxe),
        true  // iron
    );
}

} // namespace mc
