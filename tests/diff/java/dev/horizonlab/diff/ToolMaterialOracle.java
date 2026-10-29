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
import net.minecraft.world.item.ToolMaterial;

/**
 * 工具材质的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ToolMaterialOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.ToolMaterialOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/tool_material.cpp 同构：每个材质报耐久、挖掘效率、攻击伤害加成、
 * 附魔能力。
 *
 * <p>「采集等级」不比：原版用 {@code INCORRECT_FOR_*_TOOL} 方块标签表达「挖不动什么」，
 * 没有等级这个数；Cubium 的数字等级是它自己的实现细节，硬把两者对齐只会制造噪声。
 * 方块标签本身由 {@code block_tags} harness 覆盖。
 */
public final class ToolMaterialOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final Map<String, ToolMaterial> MATERIALS = materials();

    private ToolMaterialOracle() {}

    private static Map<String, ToolMaterial> materials() {
        Map<String, ToolMaterial> map = new LinkedHashMap<>();
        map.put("wood", ToolMaterial.WOOD);
        map.put("stone", ToolMaterial.STONE);
        map.put("copper", ToolMaterial.COPPER);
        map.put("iron", ToolMaterial.IRON);
        map.put("diamond", ToolMaterial.DIAMOND);
        map.put("gold", ToolMaterial.GOLD);
        map.put("netherite", ToolMaterial.NETHERITE);
        return map;
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String name : MATERIALS.keySet()) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: ToolMaterialOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        ToolMaterial material = MATERIALS.get(name);
        if (material == null) {
            result.addProperty("missing", true);
            return result;
        }

        result.addProperty("durability", material.durability());
        result.addProperty("speed", material.speed());
        result.addProperty("attackDamageBonus", material.attackDamageBonus());
        result.addProperty("enchantmentValue", material.enchantmentValue());
        return result;
    }
}