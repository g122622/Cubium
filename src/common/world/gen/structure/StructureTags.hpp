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

#include "StructureTag.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

namespace mc::world::gen::structure {

/**
 * @brief 内置结构标签集合
 *
 * 包含所有原版结构标签，对应 MC 1.21.11 的 StructureTags。
 *
 * 与 BiomeTags 类似，使用懒初始化模式：首次访问任一具名标签时，
 * 自动调用 initialize() 注册所有内置标签。也可在服务器启动时
 * 由 MinecraftServer::initializeRegistries() 显式调用 initialize()。
 *
 * 数据包加载（StructureTagLoader）会在 initialize() 之后执行，
 * 将数据包中的标签条目合并到内置标签中（追加或 replace）。
 *
 * 使用方法：
 * @code
 * // 初始化（在 MinecraftServer::initializeRegistries 中调用）
 * StructureTags::initialize();
 *
 * // 检查结构是否在海豚定位标签中
 * if (StructureTags::DOLPHIN_LOCATED().contains(ResourceLocation("minecraft", "shipwreck"))) {
 *     // 该结构可被海豚定位
 * }
 *
 * // 通过 ID 查询任意标签
 * StructureTag* tag = StructureTags::getTag(ResourceLocation("minecraft", "dolphin_located"));
 * @endcode
 *
 * 参考: net.minecraft.tags.StructureTags (MC 1.21.11)
 */
class StructureTags {
public:
    // ========== 玩法定位标签 ==========

    /// 末影之眼定位的结构（要塞），对应 #minecraft:eye_of_ender_located
    static StructureTag& EYE_OF_ENDER_LOCATED();
    /// 海豚定位的结构（海底废墟 + 沉船），对应 #minecraft:dolphin_located
    static StructureTag& DOLPHIN_LOCATED();

    // ========== 探险地图标签 ==========

    /// 林地探险地图指向的结构（林地府邸），对应 #minecraft:on_woodland_explorer_maps
    static StructureTag& ON_WOODLAND_EXPLORER_MAPS();
    /// 海洋探险地图指向的结构（海底神殿），对应 #minecraft:on_ocean_explorer_maps
    static StructureTag& ON_OCEAN_EXPLORER_MAPS();
    /// 热带草原村庄探险地图指向的结构，对应 #minecraft:on_savanna_village_maps
    static StructureTag& ON_SAVANNA_VILLAGE_MAPS();
    /// 沙漠村庄探险地图指向的结构，对应 #minecraft:on_desert_village_maps
    static StructureTag& ON_DESERT_VILLAGE_MAPS();
    /// 平原村庄探险地图指向的结构，对应 #minecraft:on_plains_village_maps
    static StructureTag& ON_PLAINS_VILLAGE_MAPS();
    /// 针叶林村庄探险地图指向的结构，对应 #minecraft:on_taiga_village_maps
    static StructureTag& ON_TAIGA_VILLAGE_MAPS();
    /// 雪地村庄探险地图指向的结构，对应 #minecraft:on_snowy_village_maps
    static StructureTag& ON_SNOWY_VILLAGE_MAPS();
    /// 丛林探险地图指向的结构（丛林神庙），对应 #minecraft:on_jungle_explorer_maps
    static StructureTag& ON_JUNGLE_EXPLORER_MAPS();
    /// 沼泽探险地图指向的结构（沼泽小屋），对应 #minecraft:on_swamp_explorer_maps
    static StructureTag& ON_SWAMP_EXPLORER_MAPS();
    /// 藏宝图指向的结构（埋藏宝藏），对应 #minecraft:on_treasure_maps
    static StructureTag& ON_TREASURE_MAPS();
    /// 试炼密室探险地图指向的结构，对应 #minecraft:on_trial_chambers_maps
    static StructureTag& ON_TRIAL_CHAMBERS_MAPS();

    // ========== 猫生成标签 ==========

    /// 猫可生成的结构（沼泽小屋），对应 #minecraft:cats_spawn_in
    static StructureTag& CATS_SPAWN_IN();
    /// 猫生成时为黑色变体的结构（沼泽小屋），对应 #minecraft:cats_spawn_as_black
    static StructureTag& CATS_SPAWN_AS_BLACK();

    // ========== 结构分组标签 ==========

    /// 村庄（5 个变体），对应 #minecraft:village
    static StructureTag& VILLAGE();
    /// 矿井（普通 + 恶地），对应 #minecraft:mineshaft
    static StructureTag& MINESHAFT();
    /// 沉船（普通 + 搁浅），对应 #minecraft:shipwreck
    static StructureTag& SHIPWRECK();
    /// 废弃传送门（7 个变体），对应 #minecraft:ruined_portal
    static StructureTag& RUINED_PORTAL();
    /// 海底废墟（冷水 + 温水），对应 #minecraft:ocean_ruin
    static StructureTag& OCEAN_RUIN();

    /**
     * @brief 初始化所有内置标签
     *
     * 在结构注册表初始化后调用。幂等：重复调用无副作用。
     */
    static void initialize();

    /**
     * @brief 根据 ID 获取标签
     *
     * @param id 标签资源位置
     * @return 标签指针，如果不存在返回 nullptr
     */
    [[nodiscard]] static StructureTag* getTag(const ResourceLocation& id);

    /**
     * @brief 遍历所有标签
     *
     * 用于 StructureTagLoader 在加载前清空已有标签（如果数据包 replace=true）。
     *
     * @param callback 对每个标签调用的回调
     */
    static void forEachTag(std::function<void(StructureTag&)> callback);

private:
    StructureTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<StructureTag>>& _getTags();
    static bool s_initialized;
};

} // namespace mc::world::gen::structure

// 旧命名空间兼容别名
namespace mc {
using StructureTags = ::mc::world::gen::structure::StructureTags;
} // namespace mc
