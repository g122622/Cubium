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
import java.util.Map;
import net.minecraft.world.item.equipment.ArmorMaterial;
import net.minecraft.world.item.equipment.ArmorMaterials;
import net.minecraft.world.item.equipment.ArmorType;

/**
 * 护甲材质的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ArmorMaterialOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.ArmorMaterialOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/armor_material.cpp 同构：每个材质报五个槽位的耐久与护甲值、附魔能力、
 * 韧性与击退抗性。
 *
 * <p>原版 {@code ArmorMaterial} 的字段本身就是公开的 record 组件（耐久系数、护甲值表、
 * 附魔能力、韧性、击退抗性），直接读即可；只有「每槽位的耐久」需要按 {@code ArmorType}
 * 的倍率算一次，同样调用原版 {@code ArmorType#getDurability} 而不是抄公式。
 */
public final class ArmorMaterialOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /**
     * 单元名 → 原版材质；名字与探针的清单一致。
     *
     * 必须延迟到 bootstrap 之后才求值：{@code ArmorMaterials} 的静态初始化会注册音效，
     * 而注册表在 {@code Bootstrap.bootStrap()} 之前一碰就抛异常。
     */
    private static Map<String, ArmorMaterial> materialsByName() {
        return MATERIALS;
    }

    private static Map<String, ArmorMaterial> MATERIALS;

    private static final ArmorType[] TYPES = {
        ArmorType.BOOTS, ArmorType.LEGGINGS, ArmorType.CHESTPLATE, ArmorType.HELMET, ArmorType.BODY
    };

    private static final String[] TYPE_NAMES = {"boots", "leggings", "chestplate", "helmet", "body"};

    private ArmorMaterialOracle() {}

    private static Map<String, ArmorMaterial> materials() {
        Map<String, ArmorMaterial> map = new LinkedHashMap<>();
        map.put("leather", ArmorMaterials.LEATHER);
        map.put("copper", ArmorMaterials.COPPER);
        map.put("chainmail", ArmorMaterials.CHAINMAIL);
        map.put("iron", ArmorMaterials.IRON);
        map.put("gold", ArmorMaterials.GOLD);
        map.put("diamond", ArmorMaterials.DIAMOND);
        map.put("turtle_scute", ArmorMaterials.TURTLE_SCUTE);
        map.put("netherite", ArmorMaterials.NETHERITE);
        map.put("armadillo_scute", ArmorMaterials.ARMADILLO_SCUTE);
        return map;
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
        MATERIALS = materials();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String name : materialsByName().keySet()) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: ArmorMaterialOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("material").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("material", name);

        ArmorMaterial material = materialsByName().get(name);
        if (material == null) {
            result.addProperty("missing", true);
            return result;
        }

        result.addProperty("enchantability", material.enchantmentValue());
        result.addProperty("toughness", material.toughness());
        result.addProperty("knockbackResistance", material.knockbackResistance());

        JsonObject slots = new JsonObject();
        for (int index = 0; index < TYPES.length; ++index) {
            ArmorType type = TYPES[index];
            JsonObject slot = new JsonObject();
            // 耐久 = 材质系数 × 槽位倍率，倍率挂在 ArmorType 上（原版就这么算）。
            slot.addProperty("durability", type.getDurability(material.durability()));
            slot.addProperty("defense", material.defense().getOrDefault(type, 0));
            slots.add(TYPE_NAMES[index], slot);
        }
        result.add("slots", slots);
        return result;
    }
}