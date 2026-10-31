# tests/diff/CMakeLists.txt
# 差分测试探针（differential probes）。
#
# 定位：本目录是「同一份输入喂给 Cubium 与参照实现（原版数据 / Java oracle），
# 要求两侧输出逐字节相同」的第四类测试。它既不是 tests/unit（写死期望值），
# 也不是 tests/integrated（行为包 + GameTest），也不是 tests/e2e（真协议 + 真服务端）。
#
# 约定：
#   - 每个「能力领域」是一个 probe（probes/<domain>.cpp + probes/<domain>.hpp）。
#   - 所有 probe 共享同一入口 probes/runner_main.cpp，由编译期宏
#     MC_DIFF_PROBE_DOMAIN 选择能力，避免每个能力复制一份 main。
#   - TypeScript 编排层位于 harness/，负责构造共同输入、运行两侧并保存报告。
#
# 开启 MC_BUILD_DIFF_PROBES 后，全部探针随常规构建更新。
#
# 为什么不是每能力一个独立 target：多个可执行文件都链同一个静态库 mc_common，
# 拆成多个 target 只是多几行 CMake，但对增量构建没有区别；合并到一个 target 里
# 用 foreach 生成，新增能力的成本最低。

# ---------------------------------------------------------------------------
# 能力清单：新增一个差分能力时，在三张平行列表的**同一位置**各追加一项，
# 然后在 probes/<domain>.cpp 实现它（runner_main.cpp 已共享，无需改动）。
#   后缀：可执行文件名 diff_<后缀>，也是测试脚本里引用的 domain
#   源文件：实现该能力的 .cpp
#   宏名：写入基类的 MC_DIFF_PROBE_DOMAIN，供 runner_main.cpp 选择能力
# ---------------------------------------------------------------------------
set(MC_DIFF_PROBE_SUFFIXES text_encode block_state_ids item_summary item_components entity_shape enchantment_table block_properties mob_category entity_attributes block_state_names random_sequence command_argument command_tree biome_climate damage_type block_tags item_tags loot_table loot_table_block loot_table_entity loot_container advancement biome_spawns recipe spawn_placement spawn_rules status_effect block_experience item_attribute_modifiers damage_reduction gamerule armor_material tool_material enchantment_effects attribute_range mob_experience block_destroy_progress walk_path_type path_finder fly_path_finder structure_density structure_starts villager_trades piston_movement experience potion potion_brewing block_support food_properties entity_type_tags jukebox_song enchantment_tags painting_variant damage_type_tags)
set(MC_DIFF_PROBE_SOURCES
    probes/text_encode.cpp
    probes/block_state_ids.cpp
    probes/item_summary.cpp
    probes/item_components.cpp
    probes/entity_shape.cpp
    probes/enchantment_table.cpp
    probes/block_properties.cpp
    probes/mob_category.cpp
    probes/entity_attributes.cpp
    probes/block_state_names.cpp
    probes/random_sequence.cpp
    probes/command_argument.cpp
    probes/command_tree.cpp
    probes/biome_climate.cpp
    probes/damage_type.cpp
    probes/block_tags.cpp
    probes/item_tags.cpp
    probes/loot_table.cpp
    probes/loot_table_block.cpp
    probes/loot_table_entity.cpp
    probes/loot_container.cpp
    probes/advancement.cpp
    probes/biome_spawns.cpp
    probes/recipe.cpp
    probes/spawn_placement.cpp
    probes/spawn_rules.cpp
    probes/status_effect.cpp
    probes/block_experience.cpp
    probes/item_attribute_modifiers.cpp
    probes/damage_reduction.cpp
    probes/gamerule.cpp
    probes/armor_material.cpp
    probes/tool_material.cpp
    probes/enchantment_effects.cpp
    probes/attribute_range.cpp
    probes/mob_experience.cpp
    probes/block_destroy_progress.cpp
    probes/structure_density.cpp
    probes/structure_starts.cpp
    probes/villager_trades.cpp
    probes/walk_path_type.cpp
    probes/path_finder.cpp
    probes/fly_path_finder.cpp
    probes/piston_movement.cpp
    probes/experience.cpp
    probes/potion.cpp
    probes/potion_brewing.cpp
    probes/block_support.cpp
    probes/food_properties.cpp
    probes/entity_type_tags.cpp
    probes/jukebox_song.cpp
    probes/enchantment_tags.cpp
    probes/painting_variant.cpp
    probes/damage_type_tags.cpp
)
set(MC_DIFF_PROBE_DOMAINS text_encode block_state_ids item_summary item_components entity_shape enchantment_table block_properties mob_category entity_attributes block_state_names random_sequence command_argument command_tree biome_climate damage_type block_tags item_tags loot_table loot_table_block loot_table_entity loot_container advancement biome_spawns recipe spawn_placement spawn_rules status_effect block_experience item_attribute_modifiers damage_reduction gamerule armor_material tool_material enchantment_effects attribute_range mob_experience block_destroy_progress walk_path_type path_finder fly_path_finder structure_density structure_starts villager_trades piston_movement experience potion potion_brewing block_support food_properties entity_type_tags jukebox_song enchantment_tags painting_variant damage_type_tags)

function(mc_add_diff_probe targetSuffix source domain)
    set(targetName "diff_${targetSuffix}")
    add_executable(${targetName}
        probes/runner_main.cpp
        ${source}
    )

    target_include_directories(${targetName} PRIVATE
        # probes/ 为根：probe 源码以 #include "<domain>.hpp" 表达私有头。
        ${CMAKE_CURRENT_SOURCE_DIR}/probes
        ${CMAKE_SOURCE_DIR}/src
    )

    target_compile_definitions(${targetName} PRIVATE
        MC_DIFF_PROBE_DOMAIN=${domain}
        MC_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )

    # 依赖：mc_common 是本层唯一被测的公共库；mc_bedrock_addon 不是选择，而是方块/
# 物品注册链的链接期闭包——BaseBlocks 等触发器引用了基岩 addon 的组件注册表
# （BlockComponentRegistry 等），不链就一堆 undefined symbol。它也属 src/common。
    # 刻意不链 mc_gen / mc_server_storage：那两个是 OBJECT 库，会把世界生成与存档
    # 的全部目标文件无条件塞进链接行，而差分探针按定义不测它们。
    # command_tree 测的是服务端命令注册：注册全部命令后遍历分发器，与原版命令树逐节点比对。
    # CommandRegistry 的静态闭包会引用整个服务端（时间、玩家、维度、世界、光照、结构……），
    # 故整体编译 src/server（不含进程入口、客户端、测试与存档 OBJECT 库）。
    # world/gen/feature 按服务端目标自己的源清单收集（目录通配会把只登记在其它目标里的
    # 文件也拉进来，它们单独编译有既有的重定义问题）。探针只注册与遍历，从不执行回调。
    if(targetSuffix STREQUAL "command_tree")
        target_link_options(${targetName} PRIVATE "-Xlinker" "/ERRORLIMIT:0")
        file(GLOB_RECURSE _diff_cmd_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/server/*.cpp)
        list(FILTER _diff_cmd_sources EXCLUDE REGEX "/src/server/client/")
        list(FILTER _diff_cmd_sources EXCLUDE REGEX "/src/server/tools?/")
        list(FILTER _diff_cmd_sources EXCLUDE REGEX "/src/server/main\.cpp$")
        list(FILTER _diff_cmd_sources EXCLUDE REGEX "/src/server/world/storage/")
        # 交互式终端（application/console/）依赖 FTXUI，探针不测它：只注册并遍历命令树，
        # 从不走 --interactive 入口。目录通配会带来两类问题：
        #   1) console 的 .o 引用 FTXUI 符号，探针未链接 FTXUI，链接失败；
        #   2) ServerApplicationEntry.cpp 引用 ServerConsole，若排除 console 而不排除入口，
        #      又会缺符号。
        # 故整体排除"进程入口 + 交互式终端"：二者都不属于命令树差分范围。
        list(FILTER _diff_cmd_sources EXCLUDE REGEX "/src/server/application/console/")
        list(FILTER _diff_cmd_sources EXCLUDE REGEX "/src/server/application/ServerApplicationEntry\\.cpp$")
        # 唯一例外：/panel 命令（非原版命令，不参与本探针的树比对）引用的面板统计逻辑。
        # 它们只依赖 mc_common 与已在链接行里的 ServerChunkManager/维度模块，不含 FTXUI。
        list(APPEND _diff_cmd_sources
            ${CMAKE_SOURCE_DIR}/src/server/application/console/render/PanelStats.cpp
            ${CMAKE_SOURCE_DIR}/src/server/application/console/render/PanelStatsCollector.cpp)
        # world/gen/feature 下按服务端目标自己的清单重新收集：目录通配会把「只登记在
        # 其它目标里」的文件也拉进来，而它们单独编译有既有的重定义问题。
        list(FILTER _diff_cmd_sources EXCLUDE REGEX "/src/server/world/gen/feature/")
        list(APPEND _diff_cmd_sources
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/Feature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/FeatureSpread.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/FeatureSorter.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ConfiguredFeatureRegistry.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ConfiguredFeatureLoader.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/FeatureTypeRegistry.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/MonsterRoomFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/BlockColumnFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/SimpleBlockFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/RandomBooleanSelectorFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/SimpleRandomSelectorFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/RandomSelectorFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/EndPlatformFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/BonusChestFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/VoidStartPlatformFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/BlockBlobFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/BasaltPillarFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/RandomPatchFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/BlockPileFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/DiskFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/NetherForestVegetationFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/VinesFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/WeepingVinesFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/TwistingVinesFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/SpringFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ScatteredOreFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ReplaceBlobsFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/DripstoneUtils.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/Column.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/BlockStateParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/BlockStateProviderParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/FluidStateParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/RuleTestParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/TrunkPlacerParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/FoliagePlacerParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/FeatureSizeParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/BlockPredicateParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/state/BlockStateProvider.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/state/RuleBasedBlockStateProvider.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/state/RotatedBlockStateProvider.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/state/NoiseThresholdBlockStateProvider.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/state/NoiseBlockStateProvider.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/state/DualNoiseBlockStateProvider.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/state/RandomizedIntBlockStateProvider.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ore/OreFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/TreeFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/ServerTreeGenerators.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/FallenTreeFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/decorator/TreeDecorator.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/decorator/AttachToLogsDecorator.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/decorator/PlaceOnGroundDecorator.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/decorator/SimpleTreeDecorators.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/decorator/TrunkVineDecorator.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/trunk/TrunkPlacer.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/trunk/StraightTrunkPlacer.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/trunk/TrunkPlacers.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/trunk/CherryTrunkPlacer.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/trunk/UpwardsBranchingTrunkPlacer.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/foliage/FoliagePlacer.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/foliage/BlobFoliagePlacer.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/foliage/FoliagePlacers.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/foliage/CherryFoliagePlacer.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/foliage/RandomSpreadFoliagePlacer.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/vegetation/FlowerFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/vegetation/BigMushroomFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/vegetation/ServerBigMushroomGenerators.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/vegetation/BambooFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/RootSystemFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/VegetationPatchFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/PointedDripstoneFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/LargeDripstoneFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/DripstoneClusterFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/DesertWellFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/IcebergFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/FossilFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/GeodeFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/MultifaceGrowthFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/cave/SculkPatchFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ocean/KelpFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ocean/SeagrassFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ocean/SeaPickleFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ocean/CoralFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ocean/CoralTreeFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ocean/CoralMushroomFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ocean/CoralClawFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ocean/BlueIceFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/end/EndSpikeFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/end/EndGatewayFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/end/IceSpikeFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/end/ChorusPlantFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/end/EndIslandFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/nether/HugeFungusFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/nether/GlowstoneFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/nether/BasaltFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/nether/DeltaFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/nether/UnderwaterMagmaFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/LakeFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/SnowAndFreezeFeature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/predicate/BlockPredicate.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/predicate/MatchingFluidsPredicate.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/Template.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/TemplateManager.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/TemplateManagerHostBinding.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/TemplateLoader.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/RuleTest.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/CappedStructureProcessor.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/ProtectedBlocksProcessor.cpp
        )
        target_sources(${targetName} PRIVATE ${_diff_cmd_sources})
        target_link_libraries(${targetName} PRIVATE mc_server_storage)
        # 模板目录整体排除（见上）；其符号由占位文件兜底。
    endif()

    target_link_libraries(${targetName} PRIVATE
        mc_common
        mc_bedrock_addon
        mc_offset_allocator
        nlohmann_json::nlohmann_json
    )

    # 战利品表要按资源名解析「乐器标签」（`set_instrument` 函数），乐器与乐器标签的
    # 数据包加载器在 src/server/item/instrument 下。那里只有这两个加载器，不牵出
    # 服务端其它子系统，因此只给战利品探针单独补上，避免所有探针都链接服务端。
    if(targetSuffix MATCHES "^loot_table" OR targetSuffix STREQUAL "loot_container")
        target_sources(${targetName} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/server/item/instrument/InstrumentLoader.cpp
            ${CMAKE_SOURCE_DIR}/src/server/item/instrument/InstrumentTagLoader.cpp
        )
    endif()

    # 结构地形适配（Beardifier）的实现位于 mc_gen（OBJECT 库，会把整个世界生成的目标文件
    # 无条件塞进链接行）。这里只补上探针真正用到的那个编译单元：Beardifier 自身只依赖
    # mc_common 的包围盒与 JigsawJunction，不牵出区块生成器。
    if(targetSuffix STREQUAL "structure_density")
        target_sources(${targetName} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/density/Beardifier.cpp
        )
    endif()

    # 结构起点（STRUCTURE_STARTS）要跑真实的世界生成管线：生成器、群系源、结构集装配、
    # 拼图模板。这些都住在 mc_gen（OBJECT 库）里，因此按 command_tree 的办法重新收集
    # 世界生成的源码，而不是链接该 OBJECT 库——后者会把雕刻、装饰、后处理的整条管线
    # 一并塞进链接行，其中不少编译单元单独编译有既有的重定义问题。
    if(targetSuffix STREQUAL "structure_starts")
        target_link_options(${targetName} PRIVATE "-Xlinker" "/ERRORLIMIT:0")
        file(GLOB_RECURSE _diff_gen_sources CONFIGURE_DEPENDS
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/*.cpp)
        # 与 command_tree 相同的排除：feature 子树只登记在服务端自己的源清单里，目录通配会把它
        # 的依赖一起拉进来又编译不成。结构装配会经由片段回调间接引用地物、规则测试与方块
        # 谓词，因此下面按服务端清单补上「结构装配真正会调用」的那一批。
        list(FILTER _diff_gen_sources EXCLUDE REGEX "/src/server/world/gen/feature/")
        target_sources(${targetName} PRIVATE ${_diff_gen_sources})
        # 拼图装配与模板池：结构定义加载时就要读 `.nbt` 模板，缺了它们结构全部退化。
        target_sources(${targetName} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/Template.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/TemplateLoader.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/TemplateManager.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/TemplateManagerHostBinding.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/RuleTest.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/CappedStructureProcessor.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/template/ProtectedBlocksProcessor.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/FeatureSorter.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ConfiguredFeatureRegistry.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/ConfiguredFeatureLoader.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/FeatureTypeRegistry.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/BlockStateParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/parser/RuleTestParser.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/Feature.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/FeatureSpread.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/spawn/WorldGenSpawner.cpp
        )
        # 结构模板里嵌着「放置配置化地物」的片段，结构定义装配期就要把对应地物类构造出来；
        # 这些地物分散在 feature/ 的各个子目录里，逐个登记会持续漂移，这里按目录整体收集。
        # 它们只被"构造出来"，不会执行 place，因此不会触碰世界状态。
        file(GLOB_RECURSE _diff_feature_sources CONFIGURE_DEPENDS
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/*.cpp)
        list(FILTER _diff_feature_sources EXCLUDE REGEX "/src/server/world/gen/feature/template/")
        # 目录里有两种「不是独立编译单元」的文件，必须整份剔除：
        #   1) 聚合单元（*Placers / *Decorators / *Generators / BlockPredicate），它们直接
        #      #include 同目录的兄弟 .cpp；
        #   2) 反过来被聚合单元包含的兄弟文件（只存在于树装饰器目录）。
        # 两条规则同时生效才不会与兄弟文件重复定义，聚合单元本身也不需要——
        # 结构装配只构造模板片段，从不解析树或巨型蘑菇的配置。
        file(GLOB_RECURSE _diff_aggregate_units CONFIGURE_DEPENDS
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/*Placers.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/*Decorators.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/*Generators.cpp
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/predicate/BlockPredicate.cpp)
        list(REMOVE_ITEM _diff_feature_sources ${_diff_aggregate_units})
        set(_diff_decorator_dir ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/decorator)
        list(REMOVE_ITEM _diff_feature_sources
            ${_diff_decorator_dir}/AttachToLogsDecorator.cpp
            ${_diff_decorator_dir}/PlaceOnGroundDecorator.cpp
            ${_diff_decorator_dir}/SimpleTreeDecorators.cpp)
        target_sources(${targetName} PRIVATE ${_diff_feature_sources})
        # 地物类型的注册表在解析树配置时会按名构造装饰器，因此装饰器目录必须整体编入。
        # 这三个文件不在任何聚合单元里，是独立的实现单元。
        target_sources(${targetName} PRIVATE
            ${_diff_decorator_dir}/TreeDecorator.cpp
            ${_diff_decorator_dir}/SimpleTreeDecorators.cpp
            ${_diff_decorator_dir}/TrunkVineDecorator.cpp
            ${_diff_decorator_dir}/AttachToLogsDecorator.cpp
            ${_diff_decorator_dir}/PlaceOnGroundDecorator.cpp)
        # 方块谓词的注册表与「是否需要加载区块」的实现住在 storage 流程里，其中一处又反向
        # 引用计分板数据管理器；计分板本身不在本探针的测试范围内，但链路上绕不开。
        target_sources(${targetName} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/server/scoreboard/storage/ScoreboardDataManager.cpp
            ${CMAKE_SOURCE_DIR}/src/server/scoreboard/storage/ScoreboardSaveData.cpp)
        # 结构定义装配会就地构造各结构类型的对象（村庄、要塞、试炼密室……），这些实现在
        # feature/ 之外的 structures 子目录，必须一并收入。
        file(GLOB_RECURSE _diff_structure_sources CONFIGURE_DEPENDS
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/structure/structures/*.cpp)
        target_sources(${targetName} PRIVATE ${_diff_structure_sources})
        target_link_libraries(${targetName} PRIVATE mc_server_storage)
    endif()

    if(WIN32)
        target_link_libraries(${targetName} PRIVATE ws2_32)
    endif()

    # 顶层把 CMAKE_RUNTIME_OUTPUT_DIRECTORY 设为 bin/，多配置生成器（Ninja Multi-Config、
    # Visual Studio）会自动再追加 <Config>/ 一层。这里不覆盖它，让所有可执行文件
    # （服务端、mc_tests、probe）都按项目统一规则落在 bin/<Config>/；编排层按同一规则查找。
    # 注意 RUNTIME_OUTPUT_DIRECTORY_<Config> 是**整体覆盖**而非叠加，设成 bin/<Config>/diff
    # 反而会让 probe 与项目其它产物的布局不一致。

    mc_copy_runtime_dlls(${targetName})
endfunction()

list(LENGTH MC_DIFF_PROBE_SUFFIXES MC_DIFF_PROBE_COUNT)
list(LENGTH MC_DIFF_PROBE_SOURCES MC_DIFF_PROBE_SOURCE_COUNT)
list(LENGTH MC_DIFF_PROBE_DOMAINS MC_DIFF_PROBE_DOMAIN_COUNT)
if(NOT MC_DIFF_PROBE_COUNT EQUAL MC_DIFF_PROBE_SOURCE_COUNT
    OR NOT MC_DIFF_PROBE_COUNT EQUAL MC_DIFF_PROBE_DOMAIN_COUNT)
    message(FATAL_ERROR "tests/diff: 三张能力清单长度不一致（后缀 ${MC_DIFF_PROBE_COUNT}，"
        "源文件 ${MC_DIFF_PROBE_SOURCE_COUNT}，宏名 ${MC_DIFF_PROBE_DOMAIN_COUNT}）")
endif()

# 聚合目标：`--target diff_probes` 一次构建全部 probe。
add_custom_target(diff_probes)

math(EXPR MC_DIFF_PROBE_LAST "${MC_DIFF_PROBE_COUNT} - 1")
foreach(index RANGE ${MC_DIFF_PROBE_LAST})
    list(GET MC_DIFF_PROBE_SUFFIXES ${index} targetSuffix)
    list(GET MC_DIFF_PROBE_SOURCES ${index} source)
    list(GET MC_DIFF_PROBE_DOMAINS ${index} domain)
    mc_add_diff_probe(${targetSuffix} ${source} ${domain})
    add_dependencies(diff_probes diff_${targetSuffix})
endforeach()
