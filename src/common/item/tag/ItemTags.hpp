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
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <nlohmann/json_fwd.hpp>
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
/**
 * @brief 原版物品标签表的一项（生成文件 VanillaItemTags.gen.cpp 提供）。
 *
 * 成员与原版数据包逐条对应：以 `#` 开头的是嵌套标签引用，其余是物品名。
 */
struct VanillaItemTagEntry {
    const char* name;          ///< 标签名（无 minecraft: 前缀）
    std::size_t count;         ///< 成员个数
    const char* const* values; ///< 成员（物品名，或 `#` 开头的嵌套标签名，均无 minecraft: 前缀）
};

/**
 * @brief 取原版物品标签表（静态存储，进程内指针恒定）。
 * @param count 输出表项数
 */
const VanillaItemTagEntry* vanillaItemTagTable(std::size_t& count) noexcept;

class ItemTags {
public:
    // ========== 内置物品标签 ==========

    /**
     * @brief 花朵标签
     *
     * 包含所有可用于蜜蜂繁殖和授粉的花朵物品。
     */
    static ItemTag& FLOWERS();
    /** @brief 信标支付物品标签，支持数据包覆写。 */
    static ItemTag& BEACON_PAYMENT_ITEMS();

    /**
     * @brief 音符盒顶部乐器物品标签（各类头颅）
     *
     * 对齐 vanilla ItemTags.NOTE_BLOCK_TOP_INSTRUMENTS（数据包键 "noteblock_top_instruments"）。
     * 手持这些物品时右键音符盒顶面，交互降级为"放置"而非"升调"，
     * 使头颅落在音符盒上方以决定其乐器音色。
     */
    static ItemTag& NOTE_BLOCK_TOP_INSTRUMENTS();

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
     * @brief 骷髅偏好武器标签（弓）
     *
     * 对齐 vanilla ItemTags.SKELETON_PREFERRED_WEAPONS（数据包键 "skeleton_preferred_weapons"）。
     * 骷髅拾取主手物品时，属于该标签的物品优先于其它物品（即使其它物品攻击伤害更高）。
     */
    static ItemTag& SKELETON_PREFERRED_WEAPONS();

    /**
     * @brief 掠夺者偏好武器标签（弩）
     *
     * 对齐 vanilla ItemTags.PILLAGER_PREFERRED_WEAPONS。
     */
    static ItemTag& PILLAGER_PREFERRED_WEAPONS();

    /**
     * @brief 溺尸偏好武器标签（三叉戟）
     *
     * 对齐 vanilla ItemTags.DROWNED_PREFERRED_WEAPONS。
     */
    static ItemTag& DROWNED_PREFERRED_WEAPONS();

    /**
     * @brief 猪灵偏好武器标签（弩、金矛）
     *
     * 对齐 vanilla ItemTags.PIGLIN_PREFERRED_WEAPONS。
     */
    static ItemTag& PIGLIN_PREFERRED_WEAPONS();

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
     * @brief 嗅探兽食物标签
     *
     * 包含可用于诱引、繁殖嗅探兽及加速幼体成长的物品。
     * 对应 MC 原版标签 minecraft:sniffer_food（数据包键 "sniffer_food"）。
     *
     * 参考: net.minecraft.world.entity.animal.sniffer.Sniffer#isFood
     * （MC 1.21.11 通过 ItemStack.is(ItemTags.SNIFFER_FOOD) 判断）
     */
    static ItemTag& SNIFFER_FOOD();

    /**
     * @brief 蜜蜂食物标签
     *
     * 包含可诱惑蜜蜂的物品（蒲公英、牧草、火把花、瓶草花、樱花花瓣等）。
     * 对应 MC 原版标签 minecraft:bee_food（数据包键 "bee_food"）。
     */
    static ItemTag& BEE_FOOD();

    /**
     * @brief 蛙类食物标签
     *
     * 包含可诱引、繁殖青蛙并加速蝌蚪变态的物品（黏液球）。
     * 对应 MC 原版标签 minecraft:frog_food（数据包键 "frog_food"）。
     *
     * 参考: net.minecraft.world.entity.animal.frog.Frog#isFood
     * （MC 1.21.11 通过 ItemStack.is(ItemTags.FROG_FOOD) 判断）
     */
    static ItemTag& FROG_FOOD();

    /**
     * @brief 快乐恶魂食物标签
     *
     * 包含可喂食、诱引快乐恶魂的物品（雪球）。
     * 对应 MC 原版标签 minecraft:happy_ghast_food（数据包键 "happy_ghast_food"）。
     *
     * 参考: net.minecraft.world.entity.animal.happyghast.HappyGhast#isFood
     * （MC 1.21.11 通过 ItemStack.is(ItemTags.HAPPY_GHAST_FOOD) 判断）
     */
    static ItemTag& HAPPY_GHAST_FOOD();

    /**
     * @brief 骆驼食物标签
     *
     * 包含可喂食、诱引骆驼繁殖的物品（仙人掌）。
     * 对应 MC 原版标签 minecraft:camel_food。
     *
     * 参考: net.minecraft.world.entity.animal.camel.Camel#isFood
     */
    static ItemTag& CAMEL_FOOD();

    /**
     * @brief 骆驼尸壳食物标签
     *
     * 包含可喂食骆驼尸壳的物品（兔子脚）。
     * 对应 MC 原版标签 minecraft:camel_husk_food。
     *
     * 参考: net.minecraft.world.entity.animal.camel.CamelHusk#isFood
     */
    static ItemTag& CAMEL_HUSK_FOOD();

    /**
     * @brief 快乐恶魂诱惑物品标签
     *
     * 包含可诱引"未装备挽具的成年快乐恶魂"的物品（食物 + 挽具），
     * 对应 MC 原版标签 minecraft:happy_ghast_tempt_items。
     *
     * 参考: HappyGhast#registerGoals 的 TemptGoal.ForNonPathfinders 判据
     */
    static ItemTag& HAPPY_GHAST_TEMPT_ITEMS();

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
     * 必须在 ItemRegistry 初始化之后调用。标签的生成表按原版数据包导出，嵌套引用
     * （`#minecraft:logs` → `#minecraft:oak_logs` 等）以 `#` 开头的成员原样保留，
     * 由 {@link linkAndExpand} 展开——原版标签的成员集合本来就是传递闭包。
     */
    static void initialize();

    /**
     * @brief 清空标签表
     *
     * 数据包是标签表的唯一来源：服务端启动时在载入数据包标签之前清空内置快照。
     */
    static void clear();

    /**
     * @brief 把标签 JSON 编译成「同一标签的多数据包合并结果」
     *
     * 多数据包语义：`versions` 按数据包优先级从高到低排列，合并时从低到高遍历——
     * 默认追加成员，`replace` 为真的版本丢弃此前合并的内容。结果仍是标签 JSON
     * （`replace` + `values`），供 {@link applyTagJson} 写入标签表。
     *
     * 内置标签与数据包标签共用这一份编译逻辑，避免两处对「追加 / replace」的理解漂移。
     *
     * @param versions 同一标签在各数据包里的 JSON 文本（优先级从高到低）
     * @param location 标签资源位置（仅用于错误信息）
     * @return 合并后的标签 JSON
     */
    [[nodiscard]] static Result<nlohmann::json> mergeTagVersions(
        const std::vector<std::string>& versions, const ResourceLocation& location);

    /**
     * @brief 解析一份标签 JSON（物品名、`#tag` 引用、`{id, required}` 对象）
     *
     * @param json 标签 JSON
     * @param location 标签资源位置
     * @return 含 `replace` 与 `values` 的规范化 JSON；结构非法时返回错误
     */
    [[nodiscard]] static Result<nlohmann::json> compileTagJson(
        const std::string& json, const ResourceLocation& location);

    /**
     * @brief 把编译后的标签内容写入标签表
     *
     * `replace` 为真时先清空标签；成员是物品的直接加入，`#tag` 成员记为嵌套引用
     * （由 {@link linkAndExpand} 展开）。未知物品按原版 `required` 语义处理：必需成员
     * 记警告并跳过，`required: false` 静默跳过。
     *
     * @param contents {@link compileTagJson} / {@link mergeTagVersions} 的结果
     * @param location 标签资源位置
     * @return 成功，或未注册的必需物品 / 未知标签引用的错误
     */
    [[nodiscard]] static Result<void> applyTagJson(const nlohmann::json& contents, const ResourceLocation& location);

    /**
     * @brief 把编译后的标签内容写入给定标签（不经过标签表）
     *
     * 用于尚未注册进表的临时标签（`ItemTagLoader::loadFromJson`）。
     *
     * @param contents {@link compileTagJson} / {@link mergeTagVersions} 的结果
     * @param tag 目标标签
     * @return 成功，或未注册的必需物品的错误
     */
    [[nodiscard]] static Result<void> applyTagJson(const nlohmann::json& contents, ItemTag& tag);

    /**
     * @brief 把嵌套引用展开成传递闭包
     *
     * 原版标签的成员是传递闭包：`#minecraft:logs` 引用 `#minecraft:oak_logs` 等，展开后
     * 包含全部子标签的物品。这里按「被引用方先展开」的顺序把子标签物品并入引用方；
     * 引用不存在的标签、圈形引用返回错误（与 `DataPack::reload` 同一口径：标签表有
     * 结构性错误时重载失败）。
     *
     * 依赖顺序即展开后的成员数从小到大：被引用方的成员集合包含在引用方里，因此不会
     * 更大；仍想引用一个尚未展开、成员数又不小于自己的标签，只可能是环。
     *
     * `initialize` 末尾会调用一次；数据包标签加载（含 replace 清空）之后必须再调用一次。
     *
     * @return 成功，或首个未知引用 / 环引用的错误
     */
    [[nodiscard]] static Result<void> linkAndExpand();

    /**
     * @brief 检查物品标签系统是否已初始化
     *
     * 在初始化之前调用标签访问方法会导致未定义行为。
     * 可用于安全检查避免在初始化前访问标签。
     *
     * @return 是否已初始化
     */
    [[nodiscard]] static bool isInitialized()
    {
        return s_initialized;
    }

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

    /// `applyTagJson` 的共同实现：`replace` 清空后逐条写入物品与引用。
    [[nodiscard]] static Result<void> writeTagJson(const nlohmann::json& contents, ItemTag& tag);

    static std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>>& tags();
    static bool s_initialized;
};

} // namespace item::tag
} // namespace mc