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
# 全部能力编入**同一个**可执行文件 diff_runner：每个探针源文件把自己登记进
# mc::diff 的注册表（见 probes/probe.hpp），进程入口按第一个参数选择能力。
# 每能力一个 exe 会让同一份 mc_common 被链接 257 次：可执行文件合计 1.4 GB，
# 各自的 .pdb 合计 11 GB，且改一行公共代码要重链接 257 次。合并后只有一份。

# ---------------------------------------------------------------------------
# 能力清单：新增一个差分能力时，在三张平行列表的**同一位置**各追加一项，
# 然后在 probes/<domain>.cpp 实现它（runner_main.cpp 已共享，无需改动）。
#   后缀：能力域名 diff_runner <后缀>，也是测试脚本里引用的 domain
#   源文件：实现该能力的 .cpp
#   宏名：写入基类的 MC_DIFF_PROBE_DOMAIN，供 runner_main.cpp 选择能力
# ---------------------------------------------------------------------------
set(MC_DIFF_PROBE_SUFFIXES text_encode block_state_ids item_summary item_components entity_shape enchantment_table block_properties block_movement can_harvest mob_category entity_attributes block_state_names random_sequence command_argument command_tree biome_climate damage_type block_tags item_tags loot_table loot_table_block loot_table_entity loot_container advancement biome_spawns recipe spawn_placement spawn_rules status_effect block_experience item_attribute_modifiers damage_reduction gamerule armor_material tool_material enchantment_effects attribute_range mob_experience block_destroy_progress walk_path_type path_finder fly_path_finder structure_density mansion_grid beardifier_density biome_source_sample climate_sample structure_start_single structure_blocks_single structure_starts villager_trades piston_movement experience potion potion_brewing block_support food_properties entity_type_tags jukebox_song enchantment_tags painting_variant damage_type_tags enchantment_cost enchantment_applicability item_component_nbt stack_codec loot_function crafting_match armor_trim animal_variant instrument single_item_recipe banner_pattern enchantment_provider trial_spawner_config dimension_type explosion_ray explosion_damage explosion_seen_grid entity_dimensions mob_effect_merge food_stats food_tick enchantment_selection repair_cost anvil_durability item_stackable item_same_components furnace_tick brewing_tick beacon_effects hopper_cooldown spawner_delay mth_rounding mth_modulo direction_rotate aabb_ops vec3_math blockpos_codec axis_plane mth_lerp mth_bit mth_rot mth_sin mth_approach region_sector blockpos_dist enchantment_storage collide_sweep shapes_boolean fluid_spread fluid_spread_dir blockpos_bfs scheduled_tick random_tick_pos mth_color block_transform bounding_box enchantment_compat section_pos level_height chunk_pos mth_clamp chat_formatting dye_color equipment_slot heightmap_types direction_plane rotation_info blockpos_geom vec3i_arith aabb_contains fluid_height mth_inverse_lerp mth_angle_lerp mth_waveform mth_hsv argb_ops direction_nearest mth_int_div fuel_values recipe_book_category block_sound_type crop_growth redstone_signal mth_seed mth_pack_degrees mth_degrees mth_frac mth_sqrt string_util identifier_parse identifier_path blockpos_iterate blockpos_spiral aabb_factory front_and_top structure_orient bounding_box_encap chunk_region portal_relative relative_mask position_move_rotation style_apply text_color click_event hover_event identifier_resolve plain_text_contents mth_absmax display_slot objective_criteria team_rule boss_bar sound_source difficulty permission_level light_occlusion text_formatting_color text_formatting_name style_tristate team_rule_strings render_type_strings uuid_strings game_type map_decoration_type block_state_enum_strings block_state_enum_strings_2 block_state_enum_strings_3 boolean_property_index enum_property_order enum_property_order_all face_sturdy world_border easing_type mth_interpolation legacy_random custom_stats sound_events particle_type shape_face block_shape block_shape_all axis_cycle entity_type_size dispense_behavior attribute_modifier_apply entity_pose_dimensions mob_spawn_light_level dimension_type_params beacon_pyramid_level conduit_frame_level campfire_cooking cooking_recipe_types game_event_radius container_analog_signal item_max_stack_size block_light_emission block_hardness block_map_color block_friction block_fire_info block_solidity block_instrument noteblock_pitch noteblock_trigger bell_placement bell_ring jukebox_insert lever_game_event note_block_event piston_event cake_eat lectern_book tripwire_state farmland_decay tripwire_disarm berry_growth anchor_charge vibration_frequency vibration_strength vibration_info_nbt vibration_selector sculk_sensor_step brushable_fall vibration_selector_nbt block_blast_resistance block_movement_factor block_shade_brightness block_random_ticks block_signal_source item_container_fit entity_peaceful entity_serialize_summon entity_type_flags fluid_lava_react block_replaceable block_pathfindable block_light_occlusion_shape block_skylight_propagation block_view_blocking mineshaft_pieces stronghold_pieces stronghold_small_door stronghold_room_crossing end_city_pieces jigsaw_joint_order block_spawn_query dig_speed entity_physics timeline_sample perlin_noise normal_noise density_function cubic_spline density_composite noise_density shifted_noise shift_noise weird_scaled_sampler end_islands lerp3_interpolate mth_misc ray_aabb mth_atan2 mth_map_lerp mth_out_from_origin mth_length_equal blockpos_center direction_data shape_closest shape_boxes shape_query direction_facing mth_round_ext vec3_rotate aabb_ext carving_mask density_nested shape_collide edge_backoff item_cooldowns inventory_slot_mapping inventory_add fall_damage surface_rule)
set(MC_DIFF_PROBE_SOURCES    probes/text_encode.cpp
    probes/block_state_ids.cpp
    probes/item_summary.cpp
    probes/item_components.cpp
    probes/entity_shape.cpp
    probes/enchantment_table.cpp
    probes/block_properties.cpp
    probes/block_movement.cpp
    probes/can_harvest.cpp
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
    probes/walk_path_type.cpp
    probes/path_finder.cpp
    probes/fly_path_finder.cpp
    probes/structure_density.cpp
    probes/mansion_grid.cpp
    probes/beardifier_density.cpp
    probes/biome_source_sample.cpp
    probes/climate_sample.cpp
    probes/structure_start_single.cpp
    probes/structure_blocks_single.cpp
    probes/structure_starts.cpp
    probes/villager_trades.cpp
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
    probes/enchantment_cost.cpp
    probes/enchantment_applicability.cpp
    probes/item_component_nbt.cpp
    probes/stack_codec.cpp
    probes/loot_function.cpp
    probes/crafting_match.cpp
    probes/armor_trim.cpp
    probes/animal_variant.cpp
    probes/instrument.cpp
    probes/single_item_recipe.cpp
    probes/banner_pattern.cpp
    probes/enchantment_provider.cpp
    probes/trial_spawner_config.cpp
    probes/dimension_type.cpp
    probes/explosion_ray.cpp
    probes/explosion_damage.cpp
    probes/explosion_seen_grid.cpp
    probes/entity_dimensions.cpp
    probes/mob_effect_merge.cpp
    probes/food_stats.cpp
    probes/food_tick.cpp
    probes/enchantment_selection.cpp
    probes/repair_cost.cpp
    probes/anvil_durability.cpp
    probes/item_stackable.cpp
    probes/item_same_components.cpp
    probes/furnace_tick.cpp
    probes/brewing_tick.cpp
    probes/beacon_effects.cpp
    probes/hopper_cooldown.cpp
    probes/spawner_delay.cpp
    probes/mth_rounding.cpp
    probes/mth_modulo.cpp
    probes/direction_rotate.cpp
    probes/aabb_ops.cpp
    probes/vec3_math.cpp
    probes/blockpos_codec.cpp
    probes/axis_plane.cpp
    probes/mth_lerp.cpp
    probes/mth_bit.cpp
    probes/mth_rot.cpp
    probes/mth_sin.cpp
    probes/mth_approach.cpp
    probes/region_sector.cpp
    probes/blockpos_dist.cpp
    probes/enchantment_storage.cpp
    probes/collide_sweep.cpp
    probes/shapes_boolean.cpp
    probes/fluid_spread.cpp
    probes/fluid_spread_dir.cpp
    probes/blockpos_bfs.cpp
    probes/scheduled_tick.cpp
    probes/random_tick_pos.cpp
    probes/mth_color.cpp
    probes/block_transform.cpp
    probes/bounding_box.cpp
    probes/enchantment_compat.cpp
    probes/section_pos.cpp
    probes/level_height.cpp
    probes/chunk_pos.cpp
    probes/mth_clamp.cpp
    probes/chat_formatting.cpp
    probes/dye_color.cpp
    probes/equipment_slot.cpp
    probes/heightmap_types.cpp
    probes/direction_plane.cpp
    probes/rotation_info.cpp
    probes/blockpos_geom.cpp
    probes/vec3i_arith.cpp
    probes/aabb_contains.cpp
    probes/fluid_height.cpp
    probes/mth_inverse_lerp.cpp
    probes/mth_angle_lerp.cpp
    probes/mth_waveform.cpp
    probes/mth_hsv.cpp
    probes/argb_ops.cpp
    probes/direction_nearest.cpp
    probes/mth_int_div.cpp
    probes/fuel_values.cpp
    probes/recipe_book_category.cpp
    probes/block_sound_type.cpp
    probes/crop_growth.cpp
    probes/redstone_signal.cpp
    probes/mth_seed.cpp
    probes/mth_pack_degrees.cpp
    probes/mth_degrees.cpp
    probes/mth_frac.cpp
    probes/mth_sqrt.cpp
    probes/string_util.cpp
    probes/identifier_parse.cpp
    probes/identifier_path.cpp
    probes/blockpos_iterate.cpp
    probes/blockpos_spiral.cpp
    probes/aabb_factory.cpp
    probes/front_and_top.cpp
    probes/structure_orient.cpp
    probes/bounding_box_encap.cpp
    probes/chunk_region.cpp
    probes/portal_relative.cpp
    probes/relative_mask.cpp
    probes/position_move_rotation.cpp
    probes/style_apply.cpp
    probes/text_color.cpp
    probes/click_event.cpp
    probes/hover_event.cpp
    probes/identifier_resolve.cpp
    probes/plain_text_contents.cpp
    probes/mth_absmax.cpp
    probes/display_slot.cpp
    probes/objective_criteria.cpp
    probes/team_rule.cpp
    probes/boss_bar.cpp
    probes/sound_source.cpp
    probes/difficulty.cpp
    probes/permission_level.cpp
    probes/light_occlusion.cpp
    probes/text_formatting_color.cpp
    probes/text_formatting_name.cpp
    probes/style_tristate.cpp
    probes/team_rule_strings.cpp
    probes/render_type_strings.cpp
    probes/uuid_strings.cpp
    probes/game_type.cpp
    probes/map_decoration_type.cpp
    probes/block_state_enum_strings.cpp
    probes/block_state_enum_strings_2.cpp
    probes/block_state_enum_strings_3.cpp
    probes/boolean_property_index.cpp
    probes/enum_property_order.cpp
    probes/enum_property_order_all.cpp
    probes/face_sturdy.cpp
    probes/world_border.cpp
    probes/easing_type.cpp
    probes/mth_interpolation.cpp
    probes/legacy_random.cpp
    probes/custom_stats.cpp
    probes/sound_events.cpp
    probes/particle_type.cpp
    probes/shape_face.cpp
    probes/block_shape.cpp
    probes/block_shape_all.cpp
    probes/axis_cycle.cpp
    probes/entity_type_size.cpp
    probes/dispense_behavior.cpp
    probes/attribute_modifier_apply.cpp
    probes/entity_pose_dimensions.cpp
    probes/mob_spawn_light_level.cpp
    probes/dimension_type_params.cpp
    probes/beacon_pyramid_level.cpp
    probes/conduit_frame_level.cpp
    probes/campfire_cooking.cpp
    probes/cooking_recipe_types.cpp
    probes/game_event_radius.cpp
    probes/container_analog_signal.cpp
    probes/item_max_stack_size.cpp
    probes/block_light_emission.cpp
    probes/block_hardness.cpp
    probes/block_map_color.cpp
    probes/block_friction.cpp
    probes/block_fire_info.cpp
    probes/block_solidity.cpp
    probes/block_instrument.cpp
    probes/noteblock_pitch.cpp
    probes/noteblock_trigger.cpp
    probes/bell_placement.cpp
    probes/bell_ring.cpp
    probes/jukebox_insert.cpp
    probes/lever_game_event.cpp
    probes/note_block_event.cpp
    probes/piston_event.cpp
    probes/cake_eat.cpp
    probes/lectern_book.cpp
    probes/tripwire_state.cpp
    probes/farmland_decay.cpp
    probes/tripwire_disarm.cpp
    probes/berry_growth.cpp
    probes/anchor_charge.cpp
    probes/vibration_frequency.cpp
    probes/vibration_strength.cpp
    probes/vibration_info_nbt.cpp
    probes/vibration_selector.cpp
    probes/sculk_sensor_step.cpp
    probes/brushable_fall.cpp
    probes/vibration_selector_nbt.cpp
    probes/block_blast_resistance.cpp
    probes/block_movement_factor.cpp
    probes/block_shade_brightness.cpp
    probes/block_random_ticks.cpp
    probes/block_signal_source.cpp
    probes/item_container_fit.cpp
    probes/entity_peaceful.cpp
    probes/entity_serialize_summon.cpp
    probes/entity_type_flags.cpp
    probes/fluid_lava_react.cpp
    probes/block_replaceable.cpp
    probes/block_pathfindable.cpp
    probes/block_light_occlusion_shape.cpp
    probes/block_skylight_propagation.cpp
    probes/block_view_blocking.cpp
    probes/mineshaft_pieces.cpp
    probes/stronghold_pieces.cpp
    probes/stronghold_small_door.cpp
    probes/stronghold_room_crossing.cpp
    probes/end_city_pieces.cpp
    probes/jigsaw_joint_order.cpp
    probes/block_spawn_query.cpp
    probes/dig_speed.cpp
    probes/entity_physics.cpp
    probes/timeline_sample.cpp
    probes/perlin_noise.cpp
    probes/normal_noise.cpp
    probes/density_function.cpp
    probes/cubic_spline.cpp
    probes/density_composite.cpp
    probes/noise_density.cpp
    probes/shifted_noise.cpp
    probes/shift_noise.cpp
    probes/weird_scaled_sampler.cpp
    probes/end_islands.cpp
    probes/lerp3_interpolate.cpp
    probes/mth_misc.cpp
    probes/ray_aabb.cpp
    probes/mth_atan2.cpp
    probes/mth_map_lerp.cpp
    probes/mth_out_from_origin.cpp
    probes/mth_length_equal.cpp
    probes/blockpos_center.cpp
    probes/direction_data.cpp
    probes/shape_closest.cpp
    probes/shape_boxes.cpp
    probes/shape_query.cpp
    probes/direction_facing.cpp
    probes/mth_round_ext.cpp
    probes/vec3_rotate.cpp
    probes/aabb_ext.cpp
    probes/carving_mask.cpp
    probes/density_nested.cpp
    probes/shape_collide.cpp
    probes/edge_backoff.cpp
    probes/item_cooldowns.cpp
    probes/inventory_slot_mapping.cpp
    probes/inventory_add.cpp
    probes/fall_damage.cpp
    probes/surface_rule.cpp
)
set(MC_DIFF_PROBE_DOMAINS text_encode block_state_ids item_summary item_components entity_shape enchantment_table block_properties block_movement can_harvest mob_category entity_attributes block_state_names random_sequence command_argument command_tree biome_climate damage_type block_tags item_tags loot_table loot_table_block loot_table_entity loot_container advancement biome_spawns recipe spawn_placement spawn_rules status_effect block_experience item_attribute_modifiers damage_reduction gamerule armor_material tool_material enchantment_effects attribute_range mob_experience block_destroy_progress walk_path_type path_finder fly_path_finder structure_density mansion_grid beardifier_density biome_source_sample climate_sample structure_start_single structure_blocks_single structure_starts villager_trades piston_movement experience potion potion_brewing block_support food_properties entity_type_tags jukebox_song enchantment_tags painting_variant damage_type_tags enchantment_cost enchantment_applicability item_component_nbt stack_codec loot_function crafting_match armor_trim animal_variant instrument single_item_recipe banner_pattern enchantment_provider trial_spawner_config dimension_type explosion_ray explosion_damage explosion_seen_grid entity_dimensions mob_effect_merge food_stats food_tick enchantment_selection repair_cost anvil_durability item_stackable item_same_components furnace_tick brewing_tick beacon_effects hopper_cooldown spawner_delay mth_rounding mth_modulo direction_rotate aabb_ops vec3_math blockpos_codec axis_plane mth_lerp mth_bit mth_rot mth_sin mth_approach region_sector blockpos_dist enchantment_storage collide_sweep shapes_boolean fluid_spread fluid_spread_dir blockpos_bfs scheduled_tick random_tick_pos mth_color block_transform bounding_box enchantment_compat section_pos level_height chunk_pos mth_clamp chat_formatting dye_color equipment_slot heightmap_types direction_plane rotation_info blockpos_geom vec3i_arith aabb_contains fluid_height mth_inverse_lerp mth_angle_lerp mth_waveform mth_hsv argb_ops direction_nearest mth_int_div fuel_values recipe_book_category block_sound_type crop_growth redstone_signal mth_seed mth_pack_degrees mth_degrees mth_frac mth_sqrt string_util identifier_parse identifier_path blockpos_iterate blockpos_spiral aabb_factory front_and_top structure_orient bounding_box_encap chunk_region portal_relative relative_mask position_move_rotation style_apply text_color click_event hover_event identifier_resolve plain_text_contents mth_absmax display_slot objective_criteria team_rule boss_bar sound_source difficulty permission_level light_occlusion text_formatting_color text_formatting_name style_tristate team_rule_strings render_type_strings uuid_strings game_type map_decoration_type block_state_enum_strings block_state_enum_strings_2 block_state_enum_strings_3 boolean_property_index enum_property_order enum_property_order_all face_sturdy world_border easing_type mth_interpolation legacy_random custom_stats sound_events particle_type shape_face block_shape block_shape_all axis_cycle entity_type_size dispense_behavior attribute_modifier_apply entity_pose_dimensions mob_spawn_light_level dimension_type_params beacon_pyramid_level conduit_frame_level campfire_cooking cooking_recipe_types game_event_radius container_analog_signal item_max_stack_size block_light_emission block_hardness block_map_color block_friction block_fire_info block_solidity block_instrument noteblock_pitch noteblock_trigger bell_placement bell_ring jukebox_insert lever_game_event note_block_event piston_event cake_eat lectern_book tripwire_state farmland_decay tripwire_disarm berry_growth anchor_charge vibration_frequency vibration_strength vibration_info_nbt vibration_selector sculk_sensor_step brushable_fall vibration_selector_nbt block_blast_resistance block_movement_factor block_shade_brightness block_random_ticks block_signal_source item_container_fit entity_peaceful entity_serialize_summon entity_type_flags fluid_lava_react block_replaceable block_pathfindable block_light_occlusion_shape block_skylight_propagation block_view_blocking mineshaft_pieces stronghold_pieces stronghold_small_door stronghold_room_crossing end_city_pieces jigsaw_joint_order block_spawn_query dig_speed entity_physics timeline_sample perlin_noise normal_noise density_function cubic_spline density_composite noise_density shifted_noise shift_noise weird_scaled_sampler end_islands lerp3_interpolate mth_misc ray_aabb mth_atan2 mth_map_lerp mth_out_from_origin mth_length_equal blockpos_center direction_data shape_closest shape_boxes shape_query direction_facing mth_round_ext vec3_rotate aabb_ext carving_mask density_nested shape_collide edge_backoff item_cooldowns inventory_slot_mapping inventory_add fall_damage surface_rule)
function(mc_add_diff_probe targetSuffix source domain)
    # 全部能力共用一个 target：这里只登记源文件，可执行文件在文件末尾创建一次。
    set(targetName "diff_runner")
    # 附加源文件登记进全局属性，末尾统一去重后再 target_sources：多个能力声明的是
    # **同一批**文件（command_tree 与 structure_starts 都通配 src/server 的部分子树，
    # 又都补上模板/谓词/地物），重复登记会让同一个 .cpp 编译两次、链接期 duplicate symbol。
    set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES ${source})

    target_include_directories(${targetName} PRIVATE
        # probes/ 为根：probe 源码以 #include "<domain>.hpp" 表达私有头。
        ${CMAKE_CURRENT_SOURCE_DIR}/probes
        ${CMAKE_SOURCE_DIR}/src
    )

    target_compile_definitions(${targetName} PRIVATE
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
        set(MC_DIFF_NEEDS_ERRORLIMIT TRUE)
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
            # 方块谓词目录只登记两个**聚合单元**：BlockPredicate.cpp 包含其余全部谓词
            # 实现，MatchingFluidsPredicate.cpp 不详。若连同成员文件一起登记（目录通配
            # 会这么做），同一批函数会被编译两份，链接期全部报 duplicate symbol。
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
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES ${_diff_cmd_sources})
        set(MC_DIFF_NEEDS_SERVER_STORAGE TRUE PARENT_SCOPE)
        # 模板目录整体排除（见上）；其符号由占位文件兜底。
    endif()

    # 战利品表要按资源名解析「乐器标签」（`set_instrument` 函数），乐器与乐器标签的
    # 数据包加载器在 src/server/item/instrument 下。那里只有这两个加载器，不牵出
    # 服务端其它子系统，因此只给战利品探针单独补上，避免所有探针都链接服务端。
    if(targetSuffix MATCHES "^loot_table" OR targetSuffix STREQUAL "loot_container" OR targetSuffix STREQUAL "loot_function")
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES
            ${CMAKE_SOURCE_DIR}/src/server/item/instrument/InstrumentLoader.cpp
            ${CMAKE_SOURCE_DIR}/src/server/item/instrument/InstrumentTagLoader.cpp
        )
    endif()

    # 结构地形适配（Beardifier）的实现位于 mc_gen（OBJECT 库，会把整个世界生成的目标文件
    # 无条件塞进链接行）。这里只补上探针真正用到的那个编译单元：Beardifier 自身只依赖
    # mc_common 的包围盒与 JigsawJunction，不牵出区块生成器。
    if(targetSuffix STREQUAL "structure_density")
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/density/Beardifier.cpp
        )
    endif()

    # 结构起点（STRUCTURE_STARTS）要跑真实的世界生成管线：生成器、群系源、结构集装配、
    # 拼图模板。这些都住在 mc_gen（OBJECT 库）里，因此按 command_tree 的办法重新收集
    # 世界生成的源码，而不是链接该 OBJECT 库——后者会把雕刻、装饰、后处理的整条管线
    # 一并塞进链接行，其中不少编译单元单独编译有既有的重定义问题。
    # 乐器：注册表本体在 mc_common 里，只有数据包加载器在服务端目录下。
    # 这里只补这一个源文件，而不是通配整个 src/server——后者会把整条服务端管线
    # 拉进链接行，构建代价远超本能力所需。
    if(targetSuffix STREQUAL "instrument")
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES
            ${CMAKE_SOURCE_DIR}/src/server/item/instrument/InstrumentLoader.cpp)
    endif()

# structure_starts 与 mineshaft_pieces 都要跑真实的世界生成管线（前者看整座结构起点，
    # 后者把矿井拆到构件级），链接的源文件完全相同。合并成单 target 后，这里只需累积
    # 附加源与全局开关，链接选项在文件末尾统一施加。
    if(targetSuffix STREQUAL "mansion_grid"
        OR targetSuffix STREQUAL "beardifier_density" OR targetSuffix STREQUAL "biome_source_sample" OR targetSuffix STREQUAL "climate_sample"
        OR targetSuffix STREQUAL "structure_start_single" OR targetSuffix STREQUAL "structure_blocks_single"
        OR targetSuffix STREQUAL "structure_starts"
        OR targetSuffix STREQUAL "mineshaft_pieces"
        OR targetSuffix STREQUAL "stronghold_pieces" OR targetSuffix STREQUAL "stronghold_room_crossing"
        OR targetSuffix STREQUAL "jigsaw_joint_order" OR targetSuffix STREQUAL "end_city_pieces")
        set(MC_DIFF_NEEDS_ERRORLIMIT TRUE)
        file(GLOB_RECURSE _diff_gen_sources CONFIGURE_DEPENDS
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/*.cpp)
        # 与 command_tree 相同的排除：feature 子树只登记在服务端自己的源清单里，目录通配会把它
        # 的依赖一起拉进来又编译不成。结构装配会经由片段回调间接引用地物、规则测试与方块
        # 谓词，因此下面按服务端清单补上「结构装配真正会调用」的那一批。
        list(FILTER _diff_gen_sources EXCLUDE REGEX "/src/server/world/gen/feature/")
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES ${_diff_gen_sources})
        # 拼图装配与模板池：结构定义加载时就要读 `.nbt` 模板，缺了它们结构全部退化。
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES
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
        #   1) 聚合单元（形如 `#include "Xxx.cpp"` 的 .cpp），它们把兄弟实现直接包含进来；
        #   2) 反过来被聚合单元包含的兄弟文件。
        # 聚合单元与成员文件同时登记就会重复定义（ee 见 dup2）；只留其一即可。这里保留
        # 成员文件、剔除聚合单元，理由有二：成员文件数量稳定，新增一个 placer 只需加文件；
        # 而「看起来像聚合单元」的名字（*Placers/*Decorators/*Generators）不可靠——
        # `CloudPlacerHelper.cpp` 这类只剩成员的目录里根本没有聚合单元。
        #
        # 聚合单元的识别靠扫描内容而非文件名：先找出所有含 `#include "*.cpp"` 的文件，
        # 它们就是聚合单元；成员文件则取自这些 include 语句本身。
        set(_diff_aggregate_units)
        set(_diff_aggregated_members)
        foreach(_diff_candidate ${_diff_feature_sources})
            file(STRINGS ${_diff_candidate} _diff_candidate_lines REGEX "#include \"[A-Za-z0-9_]+\\.cpp\"")
            if(_diff_candidate_lines)
                list(APPEND _diff_aggregate_units ${_diff_candidate})
                foreach(_diff_line ${_diff_candidate_lines})
                    string(REGEX MATCH "\"([A-Za-z0-9_]+\\.cpp)\"" _diff_member "${_diff_line}")
                    string(REPLACE "\"" "" _diff_member "${_diff_member}")
                    get_filename_component(_diff_member_dir ${_diff_candidate} DIRECTORY)
                    list(APPEND _diff_aggregated_members ${_diff_member_dir}/${_diff_member})
                endforeach()
            endif()
        endforeach()
        list(REMOVE_ITEM _diff_feature_sources ${_diff_aggregate_units} ${_diff_aggregated_members})
        # 聚合单元整体登记一次：它们把成员实现包含进来，是这个目录唯一的「实现入口」。
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES ${_diff_aggregate_units})
        set(_diff_decorator_dir ${CMAKE_SOURCE_DIR}/src/server/world/gen/feature/tree/decorator)
        list(REMOVE_ITEM _diff_feature_sources
            ${_diff_decorator_dir}/AttachToLogsDecorator.cpp
            ${_diff_decorator_dir}/PlaceOnGroundDecorator.cpp
            ${_diff_decorator_dir}/SimpleTreeDecorators.cpp)
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES ${_diff_feature_sources})
        # 地物类型的注册表在解析树配置时会按名构造装饰器，因此装饰器目录必须整体编入。
        # 这三个文件不在任何聚合单元里，是独立的实现单元。
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES
            ${_diff_decorator_dir}/TreeDecorator.cpp
            ${_diff_decorator_dir}/SimpleTreeDecorators.cpp
            ${_diff_decorator_dir}/TrunkVineDecorator.cpp
            ${_diff_decorator_dir}/AttachToLogsDecorator.cpp
            ${_diff_decorator_dir}/PlaceOnGroundDecorator.cpp)
        # 方块谓词的注册表与「是否需要加载区块」的实现住在 storage 流程里，其中一处又反向
        # 引用计分板数据管理器；计分板本身不在本探针的测试范围内，但链路上绕不开。
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES
            ${CMAKE_SOURCE_DIR}/src/server/scoreboard/storage/ScoreboardDataManager.cpp
            ${CMAKE_SOURCE_DIR}/src/server/scoreboard/storage/ScoreboardSaveData.cpp)
        # 结构定义装配会就地构造各结构类型的对象（村庄、要塞、试炼密室……），这些实现在
        # feature/ 之外的 structures 子目录，必须一并收入。
        file(GLOB_RECURSE _diff_structure_sources CONFIGURE_DEPENDS
            ${CMAKE_SOURCE_DIR}/src/server/world/gen/structure/structures/*.cpp)
        set_property(GLOBAL APPEND PROPERTY MC_DIFF_EXTRA_SOURCES ${_diff_structure_sources})
        set(PROBE_NEEDS_SERVER_STORAGE TRUE CACHE INTERNAL "probe needs mc_server_storage")
    endif()

endfunction()

list(LENGTH MC_DIFF_PROBE_SUFFIXES MC_DIFF_PROBE_COUNT)
list(LENGTH MC_DIFF_PROBE_SOURCES MC_DIFF_PROBE_SOURCE_COUNT)
list(LENGTH MC_DIFF_PROBE_DOMAINS MC_DIFF_PROBE_DOMAIN_COUNT)
if(NOT MC_DIFF_PROBE_COUNT EQUAL MC_DIFF_PROBE_SOURCE_COUNT
    OR NOT MC_DIFF_PROBE_COUNT EQUAL MC_DIFF_PROBE_DOMAIN_COUNT)
    message(FATAL_ERROR "tests/diff: 三张能力清单长度不一致（后缀 ${MC_DIFF_PROBE_COUNT}，"
        "源文件 ${MC_DIFF_PROBE_SOURCE_COUNT}，宏名 ${MC_DIFF_PROBE_DOMAIN_COUNT}）")
endif()

# 光校验长度不够：下面按**相同索引**取后缀与源文件，两张列表一旦顺序错开，可执行文件
# 就会链接到别人的探针源文件——不报编译错误、不报警告，只表现为「某个能力报了一个与它
# 无关的错误」。历史上已经因此让 6 个能力静默失效过，这里逐索引把后缀与源文件名主干
# 比一遍，把这类错位在配置阶段就拦下来。
set(MC_DIFF_PROBE_INDEX 0)
foreach(probeSuffix ${MC_DIFF_PROBE_SUFFIXES})
    list(GET MC_DIFF_PROBE_SOURCES ${MC_DIFF_PROBE_INDEX} probeSource)
    list(GET MC_DIFF_PROBE_DOMAINS ${MC_DIFF_PROBE_INDEX} probeDomain)
    get_filename_component(probeSourceStem "${probeSource}" NAME_WE)
    if(NOT probeSourceStem STREQUAL probeSuffix OR NOT probeDomain STREQUAL probeSuffix)
        message(FATAL_ERROR
            "tests/diff: 能力清单第 ${MC_DIFF_PROBE_INDEX} 项顺序错位——"
            "后缀 '${probeSuffix}' 对应的源文件是 '${probeSource}'、宏名是 '${probeDomain}'。"
            "三张清单必须逐项同序。")
    endif()
    math(EXPR MC_DIFF_PROBE_INDEX "${MC_DIFF_PROBE_INDEX} + 1")
endforeach()

# 唯一可执行文件：全部能力编入同一个进程。runner_main.cpp 按第一个参数选择能力，
# 各探针源文件在静态初始化期登记自己（probes/probe.hpp 的 MC_DIFF_DEFINE_PROBE）。
add_executable(diff_runner
    probes/runner_main.cpp
    probes/probe_registry.cpp
)
target_include_directories(diff_runner PRIVATE
    # probes/ 为根：probe 源码以 #include "<domain>.hpp" 表达私有头。
    ${CMAKE_CURRENT_SOURCE_DIR}/probes
    ${CMAKE_SOURCE_DIR}/src
)
target_compile_definitions(diff_runner PRIVATE MC_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

math(EXPR MC_DIFF_PROBE_LAST "${MC_DIFF_PROBE_COUNT} - 1")
foreach(index RANGE ${MC_DIFF_PROBE_LAST})
    list(GET MC_DIFF_PROBE_SUFFIXES ${index} targetSuffix)
    list(GET MC_DIFF_PROBE_SOURCES ${index} source)
    list(GET MC_DIFF_PROBE_DOMAINS ${index} domain)
    mc_add_diff_probe(${targetSuffix} ${source} ${domain})
endforeach()

# 附加源文件去重后一次性登记。同一文件被多个能力同时声明是常态（command_tree 与
# structure_starts 都会通配 src/server 子树），重复进 target_sources 会让它编译两次、
# 链接期 duplicate symbol。

# 浮点精度不追求与 Java 逐位一致：项目规范明确不对齐浮点精度、`-ffast-math`、
# 溢出与精度累积，以免为此损失性能。因此这里沿用与生产构建相同的浮点选项，
# 让差分测试测到的是**真正发布出去的代码路径**。
#
# 由此带来的后果是浮点结果可能有 ulp 级差异，不能要求逐位相等：需要比浮点数的能力
# 必须在声明里给出 `normalise`（见 harness/core/comparison.ts 的 CompareTransform），
# 由编排层按容差判定；整型、枚举、字符串、布尔与结构形状仍然逐位严格比对。

get_property(MC_DIFF_EXTRA_SOURCES GLOBAL PROPERTY MC_DIFF_EXTRA_SOURCES)
if(MC_DIFF_EXTRA_SOURCES)
    list(REMOVE_DUPLICATES MC_DIFF_EXTRA_SOURCES)
    target_sources(diff_runner PRIVATE ${MC_DIFF_EXTRA_SOURCES})
endif()

# 链接只做一次。哪些能力需要额外的库/编译选项，在各自的分支里累积：
#   MC_DIFF_NEEDS_SERVER_STORAGE —— command_tree 与 structure_starts 的世界生成链路
#   MC_DIFF_NEEDS_ERRORLIMIT     —— 同上，链接错误过多时仍要看到全部符号
target_link_libraries(diff_runner PRIVATE
    mc_common
    mc_bedrock_addon
    mc_offset_allocator
    # 探针会实例化 mc_common 头文件里的 absl 容器（NoiseChunk.hpp 的 flat_hash_map 等）。
    # mc_common 自身不链 absl（只有 mc_gen 以 PUBLIC 链 absl::flat_hash_map），diff_runner
    # 又不链 mc_gen，故必须在这里显式补上，否则 lld 报 absl::container_internal 未定义。
    absl::abseil_dll
    nlohmann_json::nlohmann_json
)
# command_tree 与 structure_starts 都要跑真实的服务端链路，storage 的 OBJECT 库绕不开。
if(MC_DIFF_NEEDS_SERVER_STORAGE OR MC_DIFF_NEEDS_ERRORLIMIT)
    target_link_libraries(diff_runner PRIVATE mc_server_storage)
endif()
if(WIN32)
    target_link_libraries(diff_runner PRIVATE ws2_32)
endif()
if(MC_DIFF_NEEDS_ERRORLIMIT)
    target_link_options(diff_runner PRIVATE "-Xlinker" "/ERRORLIMIT:0")
endif()

# 顶层把 CMAKE_RUNTIME_OUTPUT_DIRECTORY 设为 bin/，多配置生成器会自动再追加 <Config>/
# 一层；编排层按同一规则查找。RUNTIME_OUTPUT_DIRECTORY_<Config> 是整体覆盖，不要用。
mc_copy_runtime_dlls(diff_runner)

# 旧聚合目标：`--target diff_probes` 仍然可用（等价于构建 diff_runner）。
add_custom_target(diff_probes)
add_dependencies(diff_probes diff_runner)
