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
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.ResourceKey;
import net.minecraft.util.RandomSource;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.enchantment.ConditionalEffect;
import net.minecraft.world.item.enchantment.Enchantment;
import net.minecraft.world.item.enchantment.EnchantmentEffectComponents;
import net.minecraft.world.item.enchantment.effects.EnchantmentAttributeEffect;
import net.minecraft.world.item.enchantment.effects.EnchantmentValueEffect;

/**
 * 附魔在各等级下效果数值的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EnchantmentEffectsOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.EnchantmentEffectsOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/enchantment_effects.cpp 同构：
 * <ul>
 *   <li>{@code damageBonus}：{@code DAMAGE} 组件各条 {@code LevelBasedValue} 之和；</li>
 *   <li>{@code protection}：{@code DAMAGE_PROTECTION} 各条的效果值，按条件里的伤害类型标签
 *       归类（无条件的那条记作 {@code none}——「不论什么伤害都给」的通用保护）；</li>
 *   <li>{@code attributes}：{@code ATTRIBUTES} 组件里的属性修饰符（与等级无关）。</li>
 * </ul>
 *
 * <p>保护的条件是 {@code LootItemCondition}，构造 {@code LootContext} 需要真实伤害事件；
 * 这里改为读条件本身的 <code>type_specific/damage_type/tags</code> 字段，等价且不依赖运行期。
 */
public final class EnchantmentEffectsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 属性效果按这些槽位展开（顺序与探针的 EquipmentSlot 枚举一致）。 */
    private static final String[] ATTRIBUTE_SLOT_NAMES = {"mainhand", "offhand", "feet", "legs", "chest", "head", "body"};

    /** 与探针的类别清单一致。 */
    private static final String[] CATEGORY_NAMES = {"none", "fire", "fall", "explosion", "projectile"};

    /** 探针的 DamageFlags（fire/fall/explosion/projectile）对应原版的伤害类型标签。 */
    private static final String[] CATEGORY_TAGS = {"", "is_fire", "is_fall", "is_explosion", "is_projectile"};

    private EnchantmentEffectsOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            // 附魔是数据包注册表，只能从 DataPackRegistries 取（BuiltInRegistries 里没有）。
            var enchantments = DataPackRegistries.load().lookup.lookupOrThrow(Registries.ENCHANTMENT);
            StringBuilder text = new StringBuilder();
            enchantments.listElements()
                    .map(entry -> entry.key().identifier().toString())
                    .sorted()
                    .forEach(name -> text.append(name).append('\n'));
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EnchantmentEffectsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var enchantments = DataPackRegistries.load().lookup.lookupOrThrow(Registries.ENCHANTMENT);
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("enchantment").getAsString();
            text.append(GSON.toJson(summarise(enchantments, name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(net.minecraft.core.HolderLookup.RegistryLookup<Enchantment> registry,
            String name) {
        JsonObject result = new JsonObject();
        result.addProperty("enchantment", name);

        var key = ResourceKey.create(Registries.ENCHANTMENT, Identifier.parse(name));
        var holder = registry.get(key).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }
        Enchantment enchantment = holder.value();

        JsonArray levels = new JsonArray();
        for (int level = 1; level <= enchantment.getMaxLevel(); ++level) {
            JsonObject entry = new JsonObject();
            entry.addProperty("level", level);
            entry.addProperty("damageBonus", sumValues(enchantment, EnchantmentEffectComponents.DAMAGE, level));
            entry.add("protection", protectionByCategory(enchantment, level));
            entry.add("attributes", attributeModifiers(enchantment, level));
            levels.add(entry);
        }
        result.add("levels", levels);
        return result;
    }

    /**
     * 累加 DAMAGE 组件在当前等级下的值。
     *
     * 每条是 {@code ConditionalEffect<EnchantmentValueEffect>}（带「对什么目标生效」的条件），
     * 这里只取其效果值——条件本身（如「只对亡灵生效」）由附魔的适用对象表达，不参与数值比对。
     */
    private static float sumValues(Enchantment enchantment, Object componentKey, int level) {
        @SuppressWarnings("unchecked")
        List<ConditionalEffect<EnchantmentValueEffect>> effects = enchantment.effects().get(
                (net.minecraft.core.component.DataComponentType<List<ConditionalEffect<EnchantmentValueEffect>>>) componentKey);
        if (effects == null) {
            return 0.0F;
        }
        float total = 0.0F;
        for (ConditionalEffect<EnchantmentValueEffect> effect : effects) {
            total += effect.effect().process(level, RandomSource.create(0L), 0.0F);
        }
        return total;
    }

    /**
     * 每条保护效果按「它接受哪些伤害类别」归类后求和。
     *
     * <p>条件为空 = 对任何伤害都有效 → {@code none}；条件里的标签命中某一类 → 归该类。
     */
    private static JsonObject protectionByCategory(Enchantment enchantment, int level) {
        JsonObject result = new JsonObject();
        for (String name : CATEGORY_NAMES) {
            result.addProperty(name, 0);
        }

        List<ConditionalEffect<EnchantmentValueEffect>> effects =
                enchantment.effects().get(EnchantmentEffectComponents.DAMAGE_PROTECTION);
        if (effects == null) {
            return result;
        }
        for (ConditionalEffect<EnchantmentValueEffect> effect : effects) {
            float value = effect.effect().process(level, RandomSource.create(0L), 0.0F);
            String category = categoryOf(effect);
            if (category == null) {
                continue;
            }
            result.addProperty(category, result.get(category).getAsInt() + (int) value);
        }
        return result;
    }

    /**
     * 条件 → 伤害类别的名字。
     *
     * <ul>
     *   <li>没有条件，或条件不限定伤害类别（如 protection 的「不绕过无敌」）→ {@code none}，
     *       即「对任何伤害都有效」；</li>
     *   <li>条件里点名了某一类伤害标签 → 归入该类；</li>
     *   <li>其它（暂时没有实例）→ null。</li>
     * </ul>
     */
    private static String categoryOf(ConditionalEffect<EnchantmentValueEffect> effect) {
        var requirements = effect.requirements();
        // 没有条件、或条件里没点名任何伤害类别标签（如 protection 只限制「不绕过无敌」），
        // 都是「对任何伤害有效」→ none。Cubium 用「掩码为 0」表达同一件事，而且它的掩码
        // 是前缀匹配，泛保护对每一类都会命中——两边都是「无类别限定」这一个概念。
        if (requirements.isEmpty() || !namesDamageCategory(requirements.get())) {
            return "none";
        }
        // 条件体是 LootItemCondition：用它的 DIRECT_CODEC 序列化回 JSON，再按标签名归类。
        String json = GSON.toJson(net.minecraft.world.level.storage.loot.predicates.LootItemCondition.DIRECT_CODEC
                .encodeStart(com.mojang.serialization.JsonOps.INSTANCE, requirements.get())
                .getOrThrow());
        for (int index = 1; index < CATEGORY_TAGS.length; ++index) {
            if (json.contains(CATEGORY_TAGS[index])) {
                return CATEGORY_NAMES[index];
            }
        }
        return null;
    }

    /** 附魔在某个等级下给出的属性修饰符；数量与等级相关（如水下呼吸每级 +1 秒）。 */
    /** 条件里是否点名了某一类伤害标签（is_fire / is_fall / ...）。 */
    private static boolean namesDamageCategory(net.minecraft.world.level.storage.loot.predicates.LootItemCondition condition) {
        String json = GSON.toJson(net.minecraft.world.level.storage.loot.predicates.LootItemCondition.DIRECT_CODEC
                .encodeStart(com.mojang.serialization.JsonOps.INSTANCE, condition)
                .getOrThrow());
        for (int index = 1; index < CATEGORY_TAGS.length; ++index) {
            if (json.contains(CATEGORY_TAGS[index])) {
                return true;
            }
        }
        return false;
    }

    private static JsonArray attributeModifiers(Enchantment enchantment, int level) {
        JsonArray attributes = new JsonArray();
        List<EnchantmentAttributeEffect> effects = enchantment.effects().get(EnchantmentEffectComponents.ATTRIBUTES);
        if (effects == null) {
            return attributes;
        }
        // 原版一条效果覆盖一个 EquipmentSlotGroup；Cubium 没有槽位组，用「逐槽位各一条」
        // 表达同样语义。这里按槽位展开，两侧条目一一对应。
        // 「对哪些槽位生效」由附魔定义顶层的 slots 声明，对全部效果统一适用。
        java.util.Set<String> slots = new java.util.HashSet<>();
        for (var slotGroup : enchantment.definition().slots()) {
            for (String slotName : ATTRIBUTE_SLOT_NAMES) {
                if (slotGroup.test(net.minecraft.world.entity.EquipmentSlot.byName(slotName))) {
                    slots.add(slotName);
                }
            }
        }
        for (EnchantmentAttributeEffect effect : effects) {
            for (String slotName : ATTRIBUTE_SLOT_NAMES) {
                if (!slots.contains(slotName)) {
                    continue;
                }
                JsonObject row = new JsonObject();
                row.addProperty("attribute", effect.attribute().unwrapKey().orElseThrow().identifier().toString());
                row.addProperty("id", effect.id().toString());
                row.addProperty("operation", effect.operation().getSerializedName());
                row.addProperty("amount", effect.amount().calculate(level));
                row.addProperty("slot", slotName);
                attributes.add(row);
            }
        }
        return attributes;
    }
}