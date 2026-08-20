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

#include "ItemTag.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {
namespace item::tag {

/**
 * @brief 物品标签注册表
 *
 * 负责集中管理 ItemTag 的创建、查询和遍历。
 */
class ItemTags {
public:
    // ========== 内置物品标签 ==========

    /**
     * @brief 花朵标签
     *
     * 包含所有可用于蜜蜂繁殖和授粉的花朵物品。
     */
    static ItemTag& FLOWERS();

    /**
     * @brief 地毯标签
     *
     * 包含所有颜色的地毯物品。
     * 用于羊驼装饰槽位判断。
     */
    static ItemTag& CARPETS();

    /**
     * @brief 床物品标签
     *
     * 包含所有颜色的床物品。
     */
    static ItemTag& BEDS();

    /**
     * @brief 减振物品标签
     *
     * 包含所有羊毛物品和地毯物品。
     * 掉落的羊毛物品不会触发振动信号。
     */
    static ItemTag& DAMPENS_VIBRATIONS();

    /**
     * @brief 防火物品标签
     *
     * 包含所有防火物品（下界合金锭、下界合金碎片、远古残骸、下界星等）。
     * 掉落的防火物品实体免疫火焰和岩浆伤害。
     */
    static ItemTag& FIRE_RESISTANT();

    /**
     * @brief 陶片物品标签
     *
     * 包含所有陶片物品（1.20 考古学陶片 + 1.21 试炼密室陶片）。
     * 用于饰纹陶罐合成配方中判断物品是否可作为陶罐面材料。
     * 对应 MC 原版标签 minecraft:decorated_pot_sherds。
     */
    static ItemTag& DECORATED_POT_SHERDS();

    /**
     * @brief 饰纹陶罐原料标签
     *
     * 包含所有可用于合成饰纹陶罐的物品，即陶片 + 砖块。
     * 砖块作为空白面使用（对应 Blank 图案）。
     * 对应 MC 原版标签 minecraft:decorated_pot_ingredients。
     */
    static ItemTag& DECORATED_POT_INGREDIENTS();

    /**
     * @brief 剑标签
     *
     * 包含所有材质的剑物品。
     * 对应 MC 原版标签 minecraft:swords。
     */
    static ItemTag& SWORDS();

    /**
     * @brief 斧标签
     *
     * 包含所有材质的斧物品。
     * 对应 MC 原版标签 minecraft:axes。
     */
    static ItemTag& AXES();

    /**
     * @brief 镐标签
     *
     * 包含所有材质的镐物品。
     * 对应 MC 原版标签 minecraft:pickaxes。
     */
    static ItemTag& PICKAXES();

    /**
     * @brief 铲标签
     *
     * 包含所有材质的铲物品。
     * 对应 MC 原版标签 minecraft:shovels。
     */
    static ItemTag& SHOVELS();

    /**
     * @brief 锄标签
     *
     * 包含所有材质的锄物品。
     * 对应 MC 原版标签 minecraft:hoes。
     */
    static ItemTag& HOES();

    /**
     * @brief 长矛标签
     *
     * 包含所有材质的长矛物品（木/石/铜/铁/金/钻石/下界合金）。
     * 对应 MC 原版标签 minecraft:spears。
     */
    static ItemTag& SPEARS();

    /**
     * @brief 破坏饰纹陶罐标签
     *
     * 包含所有会破坏饰纹陶罐的物品（剑、斧、镐、铲、锄 + 三叉戟 + 重锤）。
     * 手持这些物品破坏陶罐时，陶罐会被设为 CRACKED 状态并掉落陶片而非完整陶罐。
     * 对应 MC 原版标签 minecraft:breaks_decorated_pots。
     */
    static ItemTag& BREAKS_DECORATED_POTS();

    /**
     * @brief 冰冻免疫穿戴物标签
     *
     * 包含所有可以使穿戴者免疫冰冻效果的物品。
     * 穿戴任意一件皮革护甲即可免疫细雪冰冻。
     * 对应 MC 原版标签 minecraft:freeze_immune_wearables。
     *
     * 包含：皮革头盔、皮革胸甲、皮革护腿、皮革靴子、皮革马铠
     */
    static ItemTag& FREEZE_IMMUNE_WEARABLES();

    /**
     * @brief 锁链物品标签
     *
     * 包含铁锁链和所有铜锁链物品（含氧化和涂蜡变种）。
     * 对应 MC 原版标签 minecraft:chains。
     */
    static ItemTag& CHAINS();

    /**
     * @brief 栏杆物品标签
     *
     * 包含铁栏杆和所有铜栏杆物品（含氧化和涂蜡变种）。
     * 对应 MC 原版标签 minecraft:bars。
     */
    static ItemTag& BARS();

    /**
     * @brief 木门物品标签
     *
     * 包含所有木门物品（橡木、云杉、白桦、丛林、金合欢、深色橡木、苍白橡木、
     * 绯红木、诡异木、红树木、竹木、樱花木）。
     * 对应 MC 原版标签 minecraft:wooden_doors。
     */
    static ItemTag& WOODEN_DOORS();

    /**
     * @brief 门物品标签
     *
     * 包含所有门物品（木门 + 铁门 + 铜门及其变种）。
     * 对应 MC 原版标签 minecraft:doors。
     */
    static ItemTag& DOORS();

    /**
     * @brief 木活板门物品标签
     *
     * 包含所有木活板门物品（橡木、云杉、白桦、丛林、金合欢、深色橡木、苍白橡木、
     * 绯红木、诡异木、红树木、竹木、樱花木）。
     * 对应 MC 原版标签 minecraft:wooden_trapdoors。
     */
    static ItemTag& WOODEN_TRAPDOORS();

    /**
     * @brief 活板门物品标签
     *
     * 包含所有活板门物品（木活板门 + 铁活板门 + 铜活板门及其变种）。
     * 对应 MC 原版标签 minecraft:trapdoors。
     */
    static ItemTag& TRAPDOORS();

    /**
     * @brief 不可燃木材物品标签
     *
     * 包含所有不可燃烧的木材物品（绯红木和诡异木系列的木板、台阶、楼梯、
     * 栅栏、栅栏门、门、活板门、按钮、压力板、告示牌、书架等）。
     * 对应 MC 原版标签 minecraft:non_flammable_wood。
     */
    static ItemTag& NON_FLAMMABLE_WOOD();

    /**
     * @brief 木质书架物品标签
     *
     * 包含所有木质书架物品（橡木、云杉、白桦、丛林、金合欢、深色橡木、
     * 红树木、樱花木、苍白橡木、竹木、绯红木、诡异木）。
     * 对应 MC 原版标签 minecraft:wooden_shelves。
     */
    static ItemTag& WOODEN_SHELVES();

    /**
     * @brief 潜影盒物品标签
     *
     * 包含无色潜影盒和 16 色潜影盒物品。
     * 用于判断物品是否为潜影盒（防止嵌套放置）。
     * 对应 MC 原版标签 minecraft:shulker_boxes。
     */
    static ItemTag& SHULKER_BOXES();

    /**
     * @brief 欢乐诡鬼装备物品标签
     *
     * 包含所有 16 色马铠物品（white_harness..black_harness）。
     * 用于装备 HappyGhast 实体。
     * 对应 MC 原版标签 minecraft:harnesses (MC 1.21.11)。
     */
    static ItemTag& HARNESSES();

    /**
     * @brief 收纳袋物品标签
     *
     * 包含无色收纳袋和 16 色收纳袋物品（共 17 个变体）。
     * 用于判断物品是否为收纳袋（嵌套权重计算、内容物限制等）。
     * 对应 MC 原版标签 minecraft:bundles (MC 1.21.11)。
     */
    static ItemTag& BUNDLES();

    /**
     * @brief 修复狼铠材料标签
     *
     * 包含可用于修复狼铠的物品（犰狳鳞甲）。
     * 对应 MC 原版标签 minecraft:repairs_wolf_armor。
     */
    static ItemTag& REPAIRS_WOLF_ARMOR();

    /**
     * @brief 可染色物品标签
     *
     * 包含所有可以使用染料染色的物品。
     * 皮革盔甲、皮革马铠和狼铠可通过染色配方改变颜色。
     * 对应 MC 原版标签 minecraft:dyeable。
     */
    static ItemTag& DYEABLE();

    /**
     * @brief 铜傀儡雕像物品标签
     *
     * 包含所有 8 个铜傀儡雕像物品变体（未涂蜡/涂蜡 × 4 个氧化等级）。
     * 对应 MC 原版标签 minecraft:copper_golem_statues (MC 1.21.11)。
     */
    static ItemTag& COPPER_GOLEM_STATUES();

    /**
     * @brief 可从铜傀儡剪切的物品标签
     *
     * 包含可放置在铜傀儡天线槽（EquipmentSlot::Saddle）并通过剪刀剪下的物品。
     * MC 1.21.11 原版仅包含 poppy（虞美人），由铁傀儡的 OfferFlowerGoal 赠予铜傀儡。
     * 对应 MC 原版标签 minecraft:shearable_from_copper_golem (MC 1.21.11)。
     *
     * CopperGolemEntity::isShearable() 通过此标签判断铜傀儡是否可被剪切。
     */
    static ItemTag& SHEARABLE_FROM_COPPER_GOLEM();

    /**
     * @brief 铜箱子物品标签
     *
     * 包含所有 8 个铜箱子物品变体（未涂蜡/涂蜡 × 4 个氧化等级）。
     * 对应 MC 原版标签 minecraft:copper_chests (MC 1.21.11)。
     */
    static ItemTag& COPPER_CHESTS();

    /**
     * @brief 金矿石物品标签
     *
     * 包含所有金矿石物品（金矿石、下界金矿石、深板岩金矿石）。
     * 对应 MC 原版标签 minecraft:gold_ores。
     */
    static ItemTag& GOLD_ORES();

    /**
     * @brief 猪灵喜爱物品标签
     *
     * 包含猪灵会捡起和欣赏的所有物品。
     * 金相关物品、钟、金鹦鹉螺铠甲等。
     * 对应 MC 原版标签 minecraft:piglin_loved。
     */
    static ItemTag& PIGLIN_LOVED();

    /**
     * @brief 村民可种植种子标签
     *
     * 包含农民村民可以在耕地上种植的所有种子物品。
     * 对应 MC 原版标签 minecraft:villager_plantable_seeds。
     *
     * 包含：小麦种子、胡萝卜、马铃薯、甜菜种子、火把花种子、瓶草荚果
     *
     * 参考: net.minecraft.world.entity.ai.behavior.HarvestFarmland#plantCrop
     * （MC 1.21.11 通过 ItemStack.is(ItemTags.VILLAGER_PLANTABLE_SEEDS) 判断）
     */
    static ItemTag& VILLAGER_PLANTABLE_SEEDS();

    /**
     * @brief 苦力怕点燃器标签
     *
     * 包含可用于点燃苦力怕的物品（打火石、火焰弹）。
     * 对应 MC 原版标签 minecraft:creeper_igniters。
     *
     * 参考: net.minecraft.world.entity.monster.Creeper#mobInteract
     * （MC 1.21.11 通过 itemstack.is(ItemTags.CREEPER_IGNITERS) 判断手持物品能否点燃苦力怕）
     */
    static ItemTag& CREEPER_IGNITERS();

    /**
     * @brief 初始化所有内置物品标签
     *
     * 必须在 ItemRegistry 初始化之后调用。
     * 注册所有花朵物品到 FLOWERS 标签。
     */
    static void initialize();

    /**
     * @brief 检查物品标签系统是否已初始化
     *
     * 在初始化之前调用标签访问方法会导致未定义行为。
     * 可用于安全检查避免在初始化前访问标签。
     *
     * @return 是否已初始化
     */
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

    /**
     * @brief 注册或获取指定ID的标签。
     * @param id 标签ID。
     * @return 标签引用。
     */
    static ItemTag& registerTag(const ResourceLocation& id);

    /**
     * @brief 根据ID获取标签。
     * @param id 标签ID。
     * @return 标签指针，不存在返回 nullptr。
     */
    [[nodiscard]] static ItemTag* getTag(const ResourceLocation& id);

    /**
     * @brief 根据ID字符串获取标签。
     * @param id 标签ID字符串（namespace:path）。
     * @return 标签指针，不存在返回 nullptr。
     */
    [[nodiscard]] static ItemTag* getTag(const std::string& id);

    /**
     * @brief 遍历全部标签。
     * @param callback 回调函数。
     */
    static void forEachTag(std::function<void(ItemTag&)> callback);

private:
    ItemTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>>& tags();
    static bool s_initialized;
};

} // namespace item::tag
} // namespace mc