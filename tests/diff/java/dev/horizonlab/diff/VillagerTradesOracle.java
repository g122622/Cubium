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

package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import net.minecraft.SharedConstants;
import net.minecraft.core.Holder;
import net.minecraft.core.HolderLookup;
import net.minecraft.core.RegistryAccess;
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.server.Bootstrap;
import net.minecraft.util.RandomSource;
import net.minecraft.world.entity.Entity;
import com.mojang.authlib.GameProfile;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.entity.npc.villager.VillagerDataHolder;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.Level;
import net.minecraft.world.entity.npc.villager.VillagerData;
import net.minecraft.world.entity.npc.villager.VillagerProfession;
import net.minecraft.world.entity.npc.villager.VillagerTrades;
import net.minecraft.world.entity.npc.villager.VillagerType;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.component.CustomData;
import net.minecraft.world.item.enchantment.ItemEnchantments;
import net.minecraft.world.item.trading.ItemCost;
import net.minecraft.world.item.trading.MerchantOffer;

/**
 * 原版村民交易的参照实现。
 *
 * <p>把村民交易拆成三层可独立复算的纯函数，语料对每一层单独喂输入：
 *
 * <ul>
 *   <li>{@code gen}：某职业某等级、某个随机种子下"会生成哪两条交易"。原版
 *       {@code AbstractVillager.addOffersFromItemListings} 从该等级的条目表里随机抽取
 *       {@code num} 条（抽中且 getOffer 返回非空才算数），抽取与条目自身消耗随机数交织，
 *       是整条交易链里最难对齐的一层；
 *   <li>{@code price}：给定基础代价、需求、价格乘数与特殊价格偏移，{@code getCostA()} 的
 *       数量（原版 {@code clamp(base + max(0, floor(base*demand*mult)) + special, 1, maxStack)}）；
 *   <li>{@code restock}：给定一组交易的 uses / demand / maxUses，走一遍原版
 *       {@code Villager.restock()}：先对全部交易 {@code updateDemand()}，再 {@code resetUses()}。
 * </ul>
 *
 * <p>本项目只支持 Java 版客户端，服务端不跑村民 AI 的"交易生成时机"，但交易表本身
 * （条目内容、条目抽取、价格公式、补货语义）是纯数据 + 纯函数，完全可以逐值比对。
 */
public final class VillagerTradesOracle {
    private static final Gson GSON = new GsonBuilder().serializeSpecialFloatingPointValues().create();

    /** 交易表覆盖的职业：原版 {@code VillagerTrades.TRADES} 的键，去掉 none / nitwit。 */
    private static final List<ResourceKey<VillagerProfession>> PROFESSIONS = List.of(
            VillagerProfession.ARMORER,
            VillagerProfession.BUTCHER,
            VillagerProfession.CARTOGRAPHER,
            VillagerProfession.CLERIC,
            VillagerProfession.FARMER,
            VillagerProfession.FISHERMAN,
            VillagerProfession.FLETCHER,
            VillagerProfession.LEATHERWORKER,
            VillagerProfession.LIBRARIAN,
            VillagerProfession.MASON,
            VillagerProfession.SHEPHERD,
            VillagerProfession.TOOLSMITH,
            VillagerProfession.WEAPONSMITH);

    /** 村民外观类型（决定船只、地图、盔甲等条目的分支）。 */
    private static final List<ResourceKey<VillagerType>> TYPES = List.of(
            VillagerType.DESERT,
            VillagerType.JUNGLE,
            VillagerType.PLAINS,
            VillagerType.SAVANNA,
            VillagerType.SNOW,
            VillagerType.SWAMP,
            VillagerType.TAIGA);

    /** 空职业的注册表引用（卖家替身用；交易条目只看村民类型）。 */
    private static Holder<VillagerProfession> NONE_PROFESSION = null;

    /** 全部 7 种村民外观类型：`TypeSpecificTrade` 的导出按这个顺序枚举。 */
    private static final List<ResourceKey<VillagerType>> VILLAGER_TYPES = List.of(
            VillagerType.DESERT,
            VillagerType.JUNGLE,
            VillagerType.PLAINS,
            VillagerType.SAVANNA,
            VillagerType.SNOW,
            VillagerType.SWAMP,
            VillagerType.TAIGA);

    /** 卖家替身默认使用的村民类型（`TypeSpecificTrade` 之外的条目只看物品）。 */
    private static final ResourceKey<VillagerType> VILLAGER_TYPE_PLAINS = VillagerType.PLAINS;

    private VillagerTradesOracle() {}

    // ========================================================================
    // gen：条目抽取
    // ========================================================================

    /**
     * 复刻原版 {@code AbstractVillager.addOffersFromItemListings}。
     *
     * <p>原版从 {@code ArrayList} 里 {@code remove(random.nextInt(size))} 取条目；
     * 抽中的条目 {@code getOffer} 返回 null 时**不计数**、继续抽其余条目。这里保持同一顺序，
     * 因此随机数消耗序列与原版一致。
     *
     * <p>原版把 service level 与 trader 实体一并传入 {@code getOffer}。本语料不含需要世界
     * 查询的条目（寻宝地图，见 README 的已知缺口），trader 只用到一个方法：
     * {@code getVillagerData().type()}。因此传入一个只回答这两个问题的替身，而**不**构造
     * 真正的 {@code Villager}——它的构造函数会立刻调用 {@code updateTrades} 生成交易，
     * 反而污染本次测量。
     */
    private static List<MerchantOffer> drawOffers(List<VillagerTrades.ItemListing> listings, RandomSource random,
            int num, Holder<VillagerType> type, ServerLevel serverLevel) {
        List<VillagerTrades.ItemListing> remaining = new ArrayList<>(listings);
        List<MerchantOffer> offers = new ArrayList<>();
        VillagerData data = new VillagerData(type, NONE_PROFESSION, 1);
        int drawn = 0;
        while (drawn < num && !remaining.isEmpty()) {
            VillagerTrades.ItemListing listing = remaining.remove(random.nextInt(remaining.size()));
            // 词法作用域内调用（而非先赋给 Entity 变量）：Java 条目只按 VillagerDataHolder 读取它。
            MerchantOffer offer = listing.getOffer(serverLevel, typeOnlyTrader(data, serverLevel), random);
            if (offer != null) {
                offers.add(offer);
                drawn++;
            }
        }
        return offers;
    }

    /**
     * 只回答村民类型的卖家替身。
     *
     * <p>原版把 {@code Villager} 实体传给 {@code getOffer}，条目对它的依赖只有
     * {@code ((VillagerDataHolder) trader).getVillagerData().type()}（按村庄外观分支的条目）。
     * 但**不能**构造真正的 {@code Villager}：构造函数要求非空世界，且会立刻调用
     * {@code updateTrades} 生成交易、污染本次测量。
     *
     * <p>也不能用动态代理：{@code Entity} 是类而不是接口，JVM 在 {@code (Entity) proxy}
     * 处按接口表校验必然抛 ClassCastException。因此沿用原版 GameTest 的做法
     * （{@code GameTestHelper.makeMockPlayer}）：造一个匿名 {@code Player} 子类，
     * 在实例初始化块里改写 {@code getVillagerData()}。这样 trader 是**真实的**实体对象，
     * 条目里的 {@code instanceof VillagerDataHolder} 判定与生产路径完全一致。
     */
    private static Entity typeOnlyTrader(VillagerData data, ServerLevel level) {
        return new TraderStub(level, data);
    }

    /**
     * 村民交易条目读取的卖家视图。
     *
     * <p>{@code Player} 已经实现了 {@code VillagerDataHolder}（{@code getVillagerData()} 返回
     * 默认数据、{@code setVillagerData} 是空实现），因此只要覆写这两个方法即可，不必在匿名类上
     * 再挂接口。{@code Player} 唯一的抽象方法是 {@code gameMode()}——交易条目不读它。
     */
    private static final class TraderStub extends Player implements VillagerDataHolder {
        private VillagerData traderData;

        TraderStub(ServerLevel level, VillagerData data) {
            super(level, new GameProfile(java.util.UUID.randomUUID(), "diff-trader"));
            this.traderData = data;
        }

        @Override
        public GameType gameMode() {
            return GameType.SURVIVAL;
        }

        @Override
        public boolean isClientAuthoritative() {
            return false;
        }

        @Override
        public VillagerData getVillagerData() {
            return this.traderData;
        }

        @Override
        public void setVillagerData(VillagerData updated) {
            this.traderData = updated;
        }
    }

    // ========================================================================
    // price / restock
    // ========================================================================

    /**
     * 原版 {@code MerchantOffer.getModifiedCostCount}。
     *
     * <p>{@code floor} 作用在 f64 乘积上；{@code special}（玩家声誉 / 英雄效果）可以为负，
     * 与需求加成相互抵消后再夹到 {@code [1, maxStack]}。
     */
    private static int modifiedCostCount(int base, int demand, float priceMultiplier, int special, int maxStack) {
        int demandBonus = Math.max(0, (int) Math.floor(base * demand * priceMultiplier));
        return Math.clamp((long) base + demandBonus + special, 1, Math.max(1, maxStack));
    }

    /** 原版 {@code MerchantOffer.updateDemand()}：demand += uses - (maxUses - uses)。 */
    private static int nextDemand(int demand, int uses, int maxUses) {
        return demand + uses - (maxUses - uses);
    }

    // ========================================================================
    // 摘要
    // ========================================================================

    private static String itemId(ItemStack stack) {
        if (stack == null || stack.isEmpty()) {
            return "";
        }
        return BuiltInRegistries.ITEM.getKey(stack.getItem()).toString();
    }

    /** 物品堆摘要：只输出非平凡字段（空堆时 count 为 0、item 为空串）。 */
    private static JsonObject stackSummary(ItemStack stack) {
        JsonObject object = new JsonObject();
        object.addProperty("item", itemId(stack));
        object.addProperty("count", stack == null ? 0 : stack.getCount());
        if (stack != null && !stack.isEmpty() && stack.getMaxDamage() > 0) {
            object.addProperty("damage", stack.getDamageValue());
        }
        ItemEnchantments enchantments = stack == null ? null : stack.get(DataComponents.ENCHANTMENTS);
        if (enchantments != null && !enchantments.isEmpty()) {
            JsonObject enchantmentObject = new JsonObject();
            enchantments.entrySet().forEach(entry -> enchantmentObject.addProperty(
                    entry.getKey().unwrapKey().orElseThrow().identifier().toString(), entry.getIntValue()));
            object.add("enchantments", enchantmentObject);
        }
        CustomData custom = stack == null ? null : stack.get(DataComponents.CUSTOM_DATA);
        if (custom != null) {
            // 寻宝地图把目标结构写在 custom_data 里；按键名排序输出，避免 NBT 顺序差异。
            JsonObject copied = new JsonObject();
            custom.copyTag().keySet().stream().sorted().forEach(key -> {
                copied.addProperty(key, custom.copyTag().get(key).toString());
            });
            object.add("customData", copied);
        }
        return object;
    }

    private static JsonObject offerSummary(MerchantOffer offer) {
        JsonObject object = new JsonObject();
        object.add("buyA", stackSummary(offer.getCostA()));
        object.add("buyB", stackSummary(offer.getCostB()));
        object.add("sell", stackSummary(offer.getResult()));
        object.addProperty("maxUses", offer.getMaxUses());
        object.addProperty("xp", offer.getXp());
        // 按 f32 提升到 f64 输出：探针侧存的是 f32，两侧用同一表示才能逐位比较。
        object.addProperty("priceMultiplier", (double) offer.getPriceMultiplier());
        object.addProperty("demand", offer.getDemand());
        return object;
    }

    private static Optional<ResourceKey<VillagerProfession>> professionKey(String id) {
        Identifier identifier = Identifier.parse(id);
        return PROFESSIONS.stream().filter(key -> key.identifier().equals(identifier)).findFirst();
    }

    private static Optional<ResourceKey<VillagerType>> typeKey(String id) {
        Identifier identifier = Identifier.parse(id);
        return TYPES.stream().filter(key -> key.identifier().equals(identifier)).findFirst();
    }

    // ========================================================================
    // 单元
    // ========================================================================

    /**
     * 导出交易表本身（`list` 模式）。
     *
     * <p>语料键是职业名，输出是「等级 → 条目列表」。条目只导出**不随随机数变化**的部分，
     * 随机部分导出它的输入参数（附魔书允许的标签与等级区间等）。这样交易表的数据错误
     * 可以被直接定位到具体职业的具体等级，而不必看随机结果撞运气。
     *
     * <p>两种记录 {@code gen} 与 {@code list} 都只重放 {@code getOffer} 一次，因此
     * `list` 比 `gen` 便宜得多（不需要固定种子、不需要通过抽签）。
     */
    private static JsonObject runList(JsonObject input, HolderLookup.Provider lookup, ServerLevel world) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", input.has("unit") ? input.get("unit").getAsString() : "");

        JsonObject byLevel = new JsonObject();
        Optional<ResourceKey<VillagerProfession>> profession = professionKey(input.get("profession").getAsString());
        if (profession.isPresent()) {
            var levelMap = VillagerTrades.TRADES.get(profession.get());
            if (levelMap != null) {
                for (int level = 1; level <= 5; level++) {
                    VillagerTrades.ItemListing[] listings = levelMap.get(level);
                    if (listings == null || listings.length == 0) {
                        continue;
                    }
                    JsonArray entries = new JsonArray();
                    for (VillagerTrades.ItemListing listing : listings) {
                        entries.add(listingSummary(listing, world));
                    }
                    byLevel.add(String.valueOf(level), entries);
                }
            }
        }
        result.add("byLevel", byLevel);
        return result;
    }

    /**
     * 条目摘要：只重放一次 {@code getOffer}，再剥掉随随机数变化的部分。
     *
     * <p>原版条目没有「描述自身」的接口（它们是匿名 record / class），因此只能先生成一条
     * 交易再还原静态字段。随机字段的还原规则按条目类型区分：
     *
     * <ul>
     *   <li>{@code EnchantedItemForEmeralds} / {@code EnchantBookForEmeralds}：成交物带随机
     *       附魔，只保留物品 id 与「附魔来源标签 + 等级区间」（区间由条目内部字段决定，
     *       这里通过反射读出，避免为一个条目类型泄漏到公开 API）；
     *   <li>{@code DyedArmorForEmeralds}：保留物品 id，颜色标记为随机；
     *   <li>{@code TreasureMapForEmeralds}：保留目标结构标签与装饰类型；
     *   <li>{@code SuspiciousStewForEmerald}：保留效果与时长（虽是固定值，但写在组件里）；
     *   <li>其余：代价、成交物、次数、经验与价格乘数全部是静态值。
     * </ul>
     */
    private static JsonObject listingSummary(VillagerTrades.ItemListing listing, ServerLevel world) {
        JsonObject object = new JsonObject();
        String type = listing.getClass().getSimpleName();
        object.addProperty("type", type);

        // 寻宝地图要按卖家位置搜索真实结构：语料世界里没有结构，任何一次重放都返回 null，
        // 无法重放出静态字段。改为直接导出条目自身的构造参数（结构与装饰类型是官方 API）。
        if (type.equals("TreasureMapForEmeralds")) {
            return treasureMapSummary(listing);
        }

        // `EmeraldsForVillagerTypeItem`（渔夫的船）同样是一张「村民类型 → 物品」表：逐个展开。
        if (type.equals("EmeraldsForVillagerTypeItem")) {
            return typeTableSummary(listing, "trades", world);
        }

        // TypeSpecificTrade 记录的是「村民类型 → 条目」表：逐个类型展开，而不是只重放一次。
        // 它是包的私有 record（没有公开访问器），只能通过反射读 `trades` 字段。
        if (type.equals("TypeSpecificTrade")) {
            return typeTableSummary(listing, "trades", world);
        }
        return mergeListing(offerOf(listing, world, VILLAGER_TYPE_PLAINS, object), object);
    }

    /** 以指定村民类型重放条目并摘要（TypeSpecificTrade 的分支用）。 */
    private static JsonObject listingSummaryForType(
            VillagerTrades.ItemListing listing, ServerLevel world, ResourceKey<VillagerType> type) {
        // 地图等需要世界查询的条目在任何语料世界里都返回 null，直接按构造参数导出。
        if (listing.getClass().getSimpleName().equals("TreasureMapForEmeralds")) {
            return treasureMapSummary(listing);
        }
        JsonObject object = new JsonObject();
        object.addProperty("type", listing.getClass().getSimpleName());
        return mergeListing(offerOf(listing, world, type, object), object);
    }

    /** 重放一次 getOffer 并写入摘要字段；无法生成时写 `skipped`。 */
    private static JsonObject offerOf(VillagerTrades.ItemListing listing, ServerLevel world,
            ResourceKey<VillagerType> type, JsonObject object) {
        VillagerData data = new VillagerData(
                BuiltInRegistries.VILLAGER_TYPE.get(type).orElseThrow(), NONE_PROFESSION, 1);
        MerchantOffer offer = listing.getOffer(world, typeOnlyTrader(data, world), RandomSource.create(0L));
        if (offer == null) {
            object.addProperty("skipped", true);
            return object;
        }
        if (listing.getClass().getSimpleName().equals("EnchantBookForEmeralds")) {
            // 附魔书的成交价 = 2 + nextInt(5 + l*10) + 3*l（再按 DOUBLE_TRADE_PRICE 翻倍、封顶 64），
            // 完全取决于随机抽中的附魔与等级，无法用一次重放表达"静态字段"。
            // 表数据里属于静态的部分是「第二支付恒为 1 本书」与附魔池 + 等级区间，
            // 因此只导出这三项，成交价留待 gen 层验证。
            JsonObject buyB = new JsonObject();
            buyB.addProperty("item", "minecraft:book");
            buyB.addProperty("count", 1);
            object.add("buyB", buyB);
        } else {
            object.add("buyA", costSummary(offer.getItemCostA()));
            offer.getItemCostB().ifPresent(costB -> object.add("buyB", costSummary(costB)));
        }
        object.add("sell", staticSellSummary(offer.getResult(), listing));
        object.addProperty("maxUses", offer.getMaxUses());
        object.addProperty("xp", offer.getXp());
        // 提升为 f64 输出：Gson 对 float 只写最短十进制表示（0.05），而探针侧由 C++
        // 的 f32 提升后写 0.05000000074505806，两侧会因表示方式而不是数值产生差异。
        object.addProperty("priceMultiplier", (double) offer.getPriceMultiplier());
        return object;
    }

    /** {@code offerOf} 与分支摘要共用：把已写好的对象原样返回。 */
    private static JsonObject mergeListing(JsonObject source, JsonObject target) {
        for (var entry : source.entrySet()) {
            target.add(entry.getKey(), entry.getValue());
        }
        return target;
    }

    /** 成交物摘要：剥掉随机附魔 / 随机染料，保留"随机从哪里来"的参数。 */
    /**
     * 「村民类型 → 条目」类条目的通用展开：逐个类型重放并给出分支摘要。
     *
     * <p>两种条目共用同一张表结构，但字段名不同（`TypeSpecificTrade#trades` 是
     * `Map<ResourceKey<VillagerType>, ItemListing>`，`EmeraldsForVillagerTypeItem#trades`
     * 是 `Map<ResourceKey<VillagerType>, Item>`），因此按字段名反射读取。
     *
     * <p>只遍历实际存在的分支（不同条目覆盖的类型集合不同），顺序固定为 7 种村民类型的
     * 枚举顺序，避免类型注册表的遍历顺序进入差分结果。
     */
    private static JsonObject typeTableSummary(Object listing, String fieldName, ServerLevel world) {
        @SuppressWarnings("unchecked")
        Map<Object, Object> branches = (Map<Object, Object>) fieldOf(listing, fieldName);
        JsonObject object = new JsonObject();
        object.addProperty("type", listing.getClass().getSimpleName());
        JsonArray byType = new JsonArray();
        for (ResourceKey<VillagerType> key : VILLAGER_TYPES) {
            if (!branches.containsKey(key)) {
                continue;
            }
            Object branch = branches.get(key);
            JsonObject branchObject = new JsonObject();
            branchObject.addProperty("type", key.identifier().toString());
            if (branch instanceof VillagerTrades.ItemListing itemListing) {
                branchObject.add("listing", listingSummaryForType(itemListing, world, key));
            } else if (branch instanceof Item item) {
                // 直接给物品的条目（船只）：只重放一次保持一致。
                JsonObject sell = new JsonObject();
                Object cost = fieldOf(listing, "cost");
                Object maxUses = fieldOf(listing, "maxUses");
                Object villagerXp = fieldOf(listing, "villagerXp");
                JsonObject buyA = new JsonObject();
                buyA.addProperty("item", BuiltInRegistries.ITEM.getKey(item).toString());
                buyA.addProperty("count", (Integer) cost);
                sell.add("buyA", buyA);
                JsonObject sells = new JsonObject();
                sells.addProperty("item", "minecraft:emerald");
                sells.addProperty("count", 1);
                sell.add("sell", sells);
                sell.addProperty("maxUses", (Integer) maxUses);
                sell.addProperty("xp", (Integer) villagerXp);
                sell.addProperty("priceMultiplier", (double) 0.05F);
                branchObject.add("listing", sell);
            } else {
                branchObject.addProperty("unsupported", true);
            }
            byType.add(branchObject);
        }
        object.add("byType", byType);
        return object;
    }

    private static JsonObject staticSellSummary(ItemStack stack, VillagerTrades.ItemListing listing) {
        JsonObject object = new JsonObject();
        object.addProperty("item", itemId(stack));
        object.addProperty("count", stack.getCount());
        String type = listing.getClass().getSimpleName();
        if (type.equals("EnchantedItemForEmeralds")) {
            object.addProperty("enchantmentTag", "minecraft:on_traded_equipment");
        } else if (type.equals("DyedArmorForEmeralds")) {
            object.addProperty("dyed", true);
        } else if (type.equals("EnchantBookForEmeralds")) {
            // `type` 同时给出条目类名，等级区间原样导出（0 / Integer.MAX_VALUE 表示不限）。
            Object minLevel = fieldOf(listing, "minLevel");
            Object maxLevel = fieldOf(listing, "maxLevel");
            object.addProperty("enchantmentTag", "minecraft:" + saleableEnchantmentTagOf(listing));
            object.addProperty("minLevel", (Integer) minLevel == Integer.MAX_VALUE ? 0 : (Integer) minLevel);
            object.addProperty("maxLevel", (Integer) maxLevel == Integer.MAX_VALUE ? 0 : (Integer) maxLevel);
        } else if (type.equals("SuspiciousStewForEmerald")) {
            object.addProperty("effect", effectIdOf(stack));
        }
        return object;
    }

    /** 反射读条目字段（原版条目没有统一的访问器）。 */
    private static Object fieldOf(Object instance, String name) {
        try {
            java.lang.reflect.Field field = instance.getClass().getDeclaredField(name);
            field.setAccessible(true);
            return field.get(instance);
        } catch (ReflectiveOperationException exception) {
            throw new IllegalStateException("VillagerTradesOracle: cannot read " + name, exception);
        }
    }

    /** 附魔书条目的附魔标签短名（`#minecraft:tradeable` → `tradeable`）。 */
    private static String saleableEnchantmentTagOf(Object listing) {
        return tagShortName(fieldOf(listing, "tradeableEnchantments"));
    }

    /**
     * 寻宝地图条目：按构造参数导出。
     *
     * <p>它要按卖家位置搜索真实结构，语料世界里没有结构，任何一次重放都返回 null，
     * 拿不到静态字段；因此直接读条目自身保存的构造参数（结构标签、名称键、装饰类型、
     * 价格、次数与经验都是原版公开语义）。
     */
    private static JsonObject treasureMapSummary(Object listing) {
        JsonObject object = new JsonObject();
        object.addProperty("type", listing.getClass().getSimpleName());

        JsonObject buyA = new JsonObject();
        buyA.addProperty("item", "minecraft:emerald");
        buyA.addProperty("count", (Integer) fieldOf(listing, "emeraldCost"));
        object.add("buyA", buyA);

        JsonObject sell = new JsonObject();
        sell.addProperty("item", "minecraft:filled_map");
        sell.addProperty("count", 1);
        sell.addProperty("targetTag", "minecraft:" + tagShortName(fieldOf(listing, "destination")));
        sell.addProperty("displayName", (String) fieldOf(listing, "displayName"));
        sell.addProperty("decorationType", decorationTypeOf(listing));
        object.add("sell", sell);

        object.addProperty("maxUses", (Integer) fieldOf(listing, "maxUses"));
        object.addProperty("xp", (Integer) fieldOf(listing, "villagerXp"));
        object.addProperty("priceMultiplier", (double) 0.05F);
        return object;
    }

    /** 寻宝地图条目的装饰类型资源位置。 */
    private static String decorationTypeOf(Object listing) {
        Object holder = fieldOf(listing, "destinationType");
        if (holder instanceof Holder<?> value) {
            return value.unwrapKey().map(key -> key.identifier().toString()).orElse("");
        }
        String text = holder.toString();
        int open = text.indexOf('[');
        if (open >= 0 && text.endsWith("]")) {
            text = text.substring(open + 1, text.length() - 1);
        }
        return text.contains(":") ? text : "minecraft:" + text;
    }

    /** 标签对象（`TagKey` / `HolderSet.Named`）的短名，例如 `#minecraft:on_taiga_village_maps` → `on_taiga_village_maps`。 */
    private static String tagShortName(Object tag) {
        // `TagKey.toString()` 形如 `TagKey[minecraft:on_ocean_explorer_maps]`；
        // 先剥掉 record 的外壳，再取资源路径的最后一段。
        String text = tag.toString();
        int open = text.indexOf('[');
        if (open >= 0 && text.endsWith("]")) {
            text = text.substring(open + 1, text.length() - 1);
        }
        int slash = Math.max(text.lastIndexOf('/'), Math.max(text.lastIndexOf(':'), text.lastIndexOf('.')));
        return slash < 0 ? text : text.substring(slash + 1);
    }

    /** 迷之炖菜的效果 id（`suspicious_stew_effects` 组件的第一个效果）。 */
    private static String effectIdOf(ItemStack stack) {
        var effects = stack.get(net.minecraft.core.component.DataComponents.SUSPICIOUS_STEW_EFFECTS);
        if (effects == null || effects.effects().isEmpty()) {
            return "";
        }
        return BuiltInRegistries.MOB_EFFECT
                .getKey(effects.effects().get(0).effect().value())
                .toString();
    }

    private static JsonObject costSummary(ItemCost cost) {
        JsonObject object = new JsonObject();
        object.addProperty("item", itemId(cost.itemStack()));
        object.addProperty("count", cost.count());
        return object;
    }

    private static JsonObject runGen(JsonObject input, HolderLookup.Provider lookup, ServerLevel world) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", input.has("unit") ? input.get("unit").getAsString() : "");
        int level = input.get("level").getAsInt();
        int num = input.get("num").getAsInt();
        long seed = input.get("seed").getAsLong();

        JsonArray offers = new JsonArray();
        Optional<ResourceKey<VillagerProfession>> profession = professionKey(input.get("profession").getAsString());
        Optional<ResourceKey<VillagerType>> type = typeKey(input.get("type").getAsString());
        if (profession.isPresent() && type.isPresent()) {
            var levelMap = VillagerTrades.TRADES.get(profession.get());
            if (levelMap != null && levelMap.containsKey(level)) {
                Holder<VillagerType> typeHolder =
                        lookup.lookupOrThrow(Registries.VILLAGER_TYPE).getOrThrow(type.get());
                RandomSource random = RandomSource.create(seed);
                for (MerchantOffer offer : drawOffers(List.of(levelMap.get(level)), random, num, typeHolder, world)) {
                    offers.add(offerSummary(offer));
                }
            }
        }
        result.add("offers", offers);
        return result;
    }

    private static JsonObject runPrice(JsonObject input) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", input.has("unit") ? input.get("unit").getAsString() : "");
        result.addProperty("count",
                modifiedCostCount(input.get("base").getAsInt(),
                        input.get("demand").getAsInt(),
                        input.get("multiplier").getAsFloat(),
                        input.get("special").getAsInt(),
                        input.get("maxStack").getAsInt()));
        return result;
    }

    private static JsonObject runRestock(JsonObject input) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", input.has("unit") ? input.get("unit").getAsString() : "");
        JsonArray offers = new JsonArray();
        for (JsonElement element : input.getAsJsonArray("offers")) {
            JsonObject offer = element.getAsJsonObject();
            int uses = offer.get("uses").getAsInt();
            int demand = offer.get("demand").getAsInt();
            int maxUses = offer.get("maxUses").getAsInt();
            JsonObject next = new JsonObject();
            next.addProperty("demand", nextDemand(demand, uses, maxUses));
            next.addProperty("uses", 0);
            offers.add(next);
        }
        result.add("offers", offers);
        return result;
    }

    private static JsonObject runSample(String mode, JsonObject input, HolderLookup.Provider lookup, ServerLevel level) {
        return switch (mode) {
            case "list" -> runList(input, lookup, level);
            case "gen" -> runGen(input, lookup, level);
            case "price" -> runPrice(input);
            case "restock" -> runRestock(input);
            default -> throw new IllegalArgumentException("VillagerTradesOracle: unknown mode " + mode);
        };
    }

    private static void runFile(String mode, Path input, Path output, HolderLookup.Provider lookup, ServerLevel level)
            throws Exception {
        StringBuilder builder = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            builder.append(GSON.toJson(runSample(mode, JsonParser.parseString(line).getAsJsonObject(), lookup, level)))
                    .append('\n');
        }
        Files.writeString(output, builder.toString(), StandardCharsets.UTF_8);
    }

    public static void main(String[] args) throws Exception {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
        // 编排层统一按「<oracle> run <input> <output> [追加参数...]」调用，模式走追加参数。
        if (args.length != 4 || !args[0].equals("run")
                || !(args[3].equals("list") || args[3].equals("gen") || args[3].equals("price")
                        || args[3].equals("restock"))) {
            throw new IllegalArgumentException(
                    "usage: VillagerTradesOracle run <input> <output> <list|gen|price|restock>");
        }
        DataPackRegistries registries = DataPackRegistries.load();
        NONE_PROFESSION = registries.lookup.lookupOrThrow(Registries.VILLAGER_PROFESSION).getOrThrow(VillagerProfession.NONE);
                // 条目（附魔书、寻宝地图）会读世界注册表与高度图，因此需要一个真实的服务端世界。
        // 复用战利品 oracle 已经建好的"最小服务端"：装载数据包、建主世界，不跑 tick、不监听端口。
        runFile(args[3], Path.of(args[1]), Path.of(args[2]), registries.lookup, LootLevel.get().level);
    }
}