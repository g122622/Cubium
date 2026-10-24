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
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import net.minecraft.core.Holder;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.ai.attributes.Attribute;
import net.minecraft.world.entity.ai.attributes.AttributeSupplier;
import net.minecraft.world.entity.ai.attributes.Attributes;
import net.minecraft.world.entity.ai.attributes.DefaultAttributes;

/**
 * 生物默认属性基础值的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntityAttributesOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/entity_attributes.cpp 同构：
 *
 * <pre>
 *   { "entity": "minecraft:pig", "attributes": { "generic.max_health": "0x4024000000000000", ... } }
 * </pre>
 *
 * <p>取的是实体类型的默认属性供应器（{@link DefaultAttributes}），而不是真创建一只实体：
 * 原版部分属性（如马的跳跃力）在创建时会按随机数调整，用默认供应器才能得到稳定值。
 * 浮点用位模式比较，避免 0.1 与 0.100000001 被格式化成同一串。
 *
 * <h2>与 harness 参考表的对应</h2>
 *
 * <p>{@code list} 只列出 oracle 与 harness 参考表（tests/diff/harness/harnesses/
 * entity_attributes_reference.ts）里**都有的元素属性键**：Cubium 侧只被测这些键，
 * 这样 oracle 新增的属性（SPAWN_REINFORCEMENTS_CHANCE、ATTACK_KNOCKBACK 等）
 * 不会悄悄进入比对、把「Cubium 没实现」记成「值不一致」。
 *
 * <p>{@code run} 对 harness 的 {@code oracleMissing} 清单里的生物原样输出
 * {@code {"missing": true}}：这些类型的属性由尺寸/随机数/状态在运行时改写
 * （史莱姆、恶魂、幻翼……），其默认值以参考表为准，oracle 不参与比对。
 */
public final class EntityAttributesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /**
     * 与 harness 参考表（entity_attributes_reference.ts）共用的元素属性键。
     *
     * 只在两侧都有的键上比对：oracle 的默认供应器里还有 SPAWN_REINFORCEMENTS_CHANCE、
     * ATTACK_KNOCKBACK 等 Cubium 尚未建模的属性，全量导出会把实现缺口混进值比对。
     */
    private static final List<String> TRACKED_KEYS = List.of(
            "max_health",
            "movement_speed",
            "follow_range",
            "attack_damage",
            "armor",
            "knockback_resistance",
            "armor_toughness");

    /**
     * 键名 → 属性 Holder。
     *
     * Bootstrap 之后才会解析：{@link Attributes} 的静态初始化会注册物品/事件等注册表，
     * 写在 static final 字段里会在 main 之前触发 "Not bootstrapped"。
     */
    private static Map<String, Holder<Attribute>> tracked() {
        Map<String, Holder<Attribute>> map = new LinkedHashMap<>();
        map.put("max_health", Attributes.MAX_HEALTH);
        map.put("movement_speed", Attributes.MOVEMENT_SPEED);
        map.put("follow_range", Attributes.FOLLOW_RANGE);
        map.put("attack_damage", Attributes.ATTACK_DAMAGE);
        map.put("armor", Attributes.ARMOR);
        map.put("knockback_resistance", Attributes.KNOCKBACK_RESISTANCE);
        map.put("armor_toughness", Attributes.ARMOR_TOUGHNESS);
        return map;
    }

    /**
     * 运行时改写属性基础值的生物（与参考表的 RUNTIME_DIVERGENT 一致）。
     *
     * 原版在创建实体/尺寸变化时按随机数或尺寸重设这些值，默认供应器里的数值并不等于
     * 「实体刚创建时的值」，所以 oracle 对它们只报 {@code missing}，由参考表给出基准。
     */
    private static final Set<String> RUNTIME_DIVERGENT = Set.of(
            "minecraft:slime",
            "minecraft:magma_cube",
            "minecraft:phantom");

    private EntityAttributesOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            // 只枚举「默认供应器里确有对应元素属性」的类型：非生物实体（物品展示框、
            // 船、投射物……）没有 Attributes，列出来只会制造无意义的不一致。
            Map<String, Holder<Attribute>> attributes = tracked();
            TreeSet<String> names = new TreeSet<>();
            for (Identifier id : BuiltInRegistries.ENTITY_TYPE.keySet()) {
                EntityType<?> type = BuiltInRegistries.ENTITY_TYPE.getValue(id);
                if (!DefaultAttributes.hasSupplier(type)) {
                    continue;
                }
                AttributeSupplier supplier = DefaultAttributes.getSupplier(castToLiving(type));
                for (Holder<Attribute> attribute : attributes.values()) {
                    if (supplier.hasAttribute(attribute)) {
                        names.add(id.toString());
                        break;
                    }
                }
            }
            StringBuilder text = new StringBuilder();
            for (String name : names) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EntityAttributesOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("entity").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    @SuppressWarnings("unchecked")
    private static EntityType<? extends LivingEntity> castToLiving(EntityType<?> type) {
        return (EntityType<? extends LivingEntity>) type;
    }

    /** 原版属性基础值是 float；Cubium 用 double 存，两侧都按 float 精度比较。 */
    private static String bits(double value) {
        return String.format("0x%016x", Double.doubleToRawLongBits((double) (float) value));
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("entity", name);

        EntityType<?> type = BuiltInRegistries.ENTITY_TYPE.getOptional(Identifier.parse(name)).orElse(null);
        if (type == null || !DefaultAttributes.hasSupplier(type) || RUNTIME_DIVERGENT.contains(name)) {
            // 缺失 / 非生物 / 运行时改写：都不由 oracle 判定，交给参考表。
            result.addProperty("missing", true);
            return result;
        }

        AttributeSupplier supplier = DefaultAttributes.getSupplier(castToLiving(type));
        JsonObject attributes = new JsonObject();
        for (Map.Entry<String, Holder<Attribute>> entry : tracked().entrySet()) {
            // 只导出两侧都建模的键：该生物没有的属性（例如猪没有攻击伤害）与「值为 0」
            // 是两回事，不能混为一谈。
            if (!TRACKED_KEYS.contains(entry.getKey()) || !supplier.hasAttribute(entry.getValue())) {
                continue;
            }
            attributes.addProperty("minecraft:" + entry.getKey(),
                    bits(supplier.getBaseValue(entry.getValue())));
        }
        result.add("attributes", attributes);
        return result;
    }
}