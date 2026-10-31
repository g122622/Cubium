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
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import net.minecraft.core.HolderLookup;
import net.minecraft.core.registries.Registries;
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.contents.TranslatableContents;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.item.equipment.trim.MaterialAssetGroup;
import net.minecraft.world.item.equipment.trim.TrimMaterial;
import net.minecraft.world.item.equipment.trim.TrimPattern;

/**
 * 盔甲纹饰（{@code trim_pattern} 与 {@code trim_material}）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ArmorTrimOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/armor_trim.cpp 同构：比较图案的贴图 ID / 描述翻译键 / 是否贴花，
 * 以及材料的贴图后缀 / 描述翻译键 / 描述颜色 / 覆盖装备材质表。
 * 两类注册表都来自数据包，必须走 {@link DataPackRegistries} 加载之后才非空。
 */
public final class ArmorTrimOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ArmorTrimOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            List<String> units = new ArrayList<>();
            registries.lookup
                    .lookupOrThrow(Registries.TRIM_PATTERN)
                    .listElements()
                    .forEach(holder -> units.add("pattern/" + holder.key().identifier()));
            registries.lookup
                    .lookupOrThrow(Registries.TRIM_MATERIAL)
                    .listElements()
                    .forEach(holder -> units.add("material/" + holder.key().identifier()));
            Collections.sort(units);
            StringBuilder text = new StringBuilder();
            for (String unit : units) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: ArmorTrimOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var patterns = registries.lookup.lookupOrThrow(Registries.TRIM_PATTERN);
        var materials = registries.lookup.lookupOrThrow(Registries.TRIM_MATERIAL);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject row = JsonParser.parseString(line).getAsJsonObject();
            String unit = row.get("unit").getAsString();
            JsonObject result;
            if (unit.startsWith("pattern/")) {
                result = summarisePattern(patterns, row.get("pattern").getAsString(), unit);
            } else if (unit.startsWith("material/")) {
                result = summariseMaterial(materials, row.get("material").getAsString(), unit);
            } else {
                result = new JsonObject();
                result.addProperty("unit", unit);
                result.addProperty("error", "未知单元前缀");
            }
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarisePattern(
            HolderLookup.RegistryLookup<TrimPattern> registry, String name, String unit) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        ResourceKey<TrimPattern> key = ResourceKey.create(Registries.TRIM_PATTERN, Identifier.parse(name));
        var holder = registry.get(key).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }

        TrimPattern pattern = holder.value();
        result.addProperty("assetId", pattern.assetId().toString());
        result.addProperty("descriptionKey", translateKey(pattern.description()));
        result.addProperty("decal", pattern.decal());
        return result;
    }

    private static JsonObject summariseMaterial(
            HolderLookup.RegistryLookup<TrimMaterial> registry, String name, String unit) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        ResourceKey<TrimMaterial> key = ResourceKey.create(Registries.TRIM_MATERIAL, Identifier.parse(name));
        var holder = registry.get(key).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }

        TrimMaterial material = holder.value();
        result.addProperty("assetName", material.assets().base().suffix());
        result.addProperty("descriptionKey", translateKey(material.description()));

        String color = descriptionColor(material.description());
        result.addProperty("descriptionColor", color);

        // 覆盖表在两侧都是无序结构：按装备材质键排序后输出，保证逐字节可比。
        List<Map.Entry<ResourceKey<?>, ?>> overrides = new ArrayList<>();
        material.assets().overrides().forEach((asset, info) -> overrides.add(Map.entry(asset, info)));
        JsonArray overrideArray = new JsonArray();
        overrides.sort((a, b) -> a.getKey().toString().compareTo(b.getKey().toString()));
        for (var entry : overrides) {
            JsonObject item = new JsonObject();
            item.addProperty("asset", entry.getKey().identifier().toString());
            item.addProperty("suffix", ((MaterialAssetGroup.AssetInfo) entry.getValue()).suffix());
            overrideArray.add(item);
        }
        result.add("overrides", overrideArray);
        return result;
    }

    /** 提取文本组件的翻译键；非可翻译组件返回渲染后的文本。 */
    private static String translateKey(Component component) {
        if (component.getContents() instanceof TranslatableContents translatable) {
            return translatable.getKey();
        }
        return component.getString();
    }

    /** 提取描述里的颜色；没有颜色时返回空串（与 C++ 侧的空串约定一致）。 */
    private static String descriptionColor(Component component) {
        var color = component.getStyle().getColor();
        return color == null ? "" : color.serialize();
    }
}