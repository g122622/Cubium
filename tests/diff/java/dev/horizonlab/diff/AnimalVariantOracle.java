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
import net.minecraft.core.HolderSet;
import net.minecraft.core.Registry;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.entity.variant.PriorityProvider;
import net.minecraft.world.entity.variant.SpawnCondition;
import net.minecraft.world.entity.variant.BiomeCheck;
import net.minecraft.world.entity.variant.MoonBrightnessCheck;
import net.minecraft.world.entity.variant.StructureCheck;
import net.minecraft.world.entity.variant.SpawnPrioritySelectors;
import net.minecraft.world.entity.animal.feline.CatVariant;
import net.minecraft.world.entity.animal.chicken.ChickenVariant;
import net.minecraft.world.entity.animal.cow.CowVariant;
import net.minecraft.world.entity.animal.frog.FrogVariant;
import net.minecraft.world.entity.animal.pig.PigVariant;
import net.minecraft.world.entity.animal.wolf.WolfVariant;

/**
 * 动物变体及其出生条件的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AnimalVariantOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/animal_variant.cpp 同构：比较贴图资源、模型名，以及每条出生选择器的
 * 优先级与条件（类型 + 引用的条目 + 月相范围）。
 * 六类变体都来自数据包，必须走 {@link DataPackRegistries} 加载之后才非空。
 */
public final class AnimalVariantOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private AnimalVariantOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            List<String> units = new ArrayList<>();
            collect(registries, Registries.WOLF_VARIANT, "wolf_variant", units);
            collect(registries, Registries.CAT_VARIANT, "cat_variant", units);
            collect(registries, Registries.FROG_VARIANT, "frog_variant", units);
            collect(registries, Registries.COW_VARIANT, "cow_variant", units);
            collect(registries, Registries.CHICKEN_VARIANT, "chicken_variant", units);
            collect(registries, Registries.PIG_VARIANT, "pig_variant", units);
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
        System.err.println("usage: AnimalVariantOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static <T> void collect(
            DataPackRegistries registries,
            ResourceKey<Registry<T>> registryKey,
            String kind,
            List<String> units) {
        registries.lookup
                .lookupOrThrow(registryKey)
                .listElements()
                .forEach(holder -> units.add(kind + "/" + holder.key().identifier()));
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject row = JsonParser.parseString(line).getAsJsonObject();
            String unit = row.get("unit").getAsString();
            String kind = row.get("kind").getAsString();
            String name = row.get("variant").getAsString();
            JsonObject result;
            switch (kind) {
                case "wolf_variant" -> result = summariseWolf(registries, name, unit);
                case "cat_variant" -> result = summariseCat(registries, name, unit);
                case "frog_variant" -> result = summariseFrog(registries, name, unit);
                case "cow_variant" -> result = summariseCow(registries, name, unit);
                case "chicken_variant" -> result = summariseChicken(registries, name, unit);
                case "pig_variant" -> result = summarisePig(registries, name, unit);
                default -> {
                    result = new JsonObject();
                    result.addProperty("unit", unit);
                    result.addProperty("error", "未知变体类别");
                }
            }
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject missing(String unit) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("missing", true);
        return result;
    }

    private static JsonObject summariseWolf(DataPackRegistries registries, String name, String unit) {
        var lookup = registries.lookup.lookupOrThrow(Registries.WOLF_VARIANT);
        var holder = lookup.get(ResourceKey.create(Registries.WOLF_VARIANT, Identifier.parse(name))).orElse(null);
        if (holder == null) {
            return missing(unit);
        }
        WolfVariant variant = holder.value();
        // 按原版 AssetInfo(wild, tame, angry) 的字段顺序输出，而不是按对象遍历顺序。
        JsonArray assets = new JsonArray();
        addAsset(assets, "wild", variant.assetInfo().wild().texturePath().toString());
        addAsset(assets, "tame", variant.assetInfo().tame().texturePath().toString());
        addAsset(assets, "angry", variant.assetInfo().angry().texturePath().toString());
        return build(unit, assets, "", variant.spawnConditions());
    }

    private static JsonObject summariseCat(DataPackRegistries registries, String name, String unit) {
        var lookup = registries.lookup.lookupOrThrow(Registries.CAT_VARIANT);
        var holder = lookup.get(ResourceKey.create(Registries.CAT_VARIANT, Identifier.parse(name))).orElse(null);
        if (holder == null) {
            return missing(unit);
        }
        CatVariant variant = holder.value();
        JsonArray assets = new JsonArray();
        addAsset(assets, "", variant.assetInfo().texturePath().toString());
        return build(unit, assets, "", variant.spawnConditions());
    }

    private static JsonObject summariseFrog(DataPackRegistries registries, String name, String unit) {
        var lookup = registries.lookup.lookupOrThrow(Registries.FROG_VARIANT);
        var holder = lookup.get(ResourceKey.create(Registries.FROG_VARIANT, Identifier.parse(name))).orElse(null);
        if (holder == null) {
            return missing(unit);
        }
        FrogVariant variant = holder.value();
        JsonArray assets = new JsonArray();
        addAsset(assets, "", variant.assetInfo().texturePath().toString());
        return build(unit, assets, "", variant.spawnConditions());
    }

    private static JsonObject summariseCow(DataPackRegistries registries, String name, String unit) {
        var lookup = registries.lookup.lookupOrThrow(Registries.COW_VARIANT);
        var holder = lookup.get(ResourceKey.create(Registries.COW_VARIANT, Identifier.parse(name))).orElse(null);
        if (holder == null) {
            return missing(unit);
        }
        CowVariant variant = holder.value();
        JsonArray assets = new JsonArray();
        addAsset(assets, "", variant.modelAndTexture().asset().texturePath().toString());
        return build(unit, assets, variant.modelAndTexture().model().getSerializedName(), variant.spawnConditions());
    }

    private static JsonObject summariseChicken(
            DataPackRegistries registries, String name, String unit) {
        var lookup = registries.lookup.lookupOrThrow(Registries.CHICKEN_VARIANT);
        var holder =
                lookup.get(ResourceKey.create(Registries.CHICKEN_VARIANT, Identifier.parse(name))).orElse(null);
        if (holder == null) {
            return missing(unit);
        }
        ChickenVariant variant = holder.value();
        JsonArray assets = new JsonArray();
        addAsset(assets, "", variant.modelAndTexture().asset().texturePath().toString());
        return build(unit, assets, variant.modelAndTexture().model().getSerializedName(), variant.spawnConditions());
    }

    private static JsonObject summarisePig(DataPackRegistries registries, String name, String unit) {
        var lookup = registries.lookup.lookupOrThrow(Registries.PIG_VARIANT);
        var holder = lookup.get(ResourceKey.create(Registries.PIG_VARIANT, Identifier.parse(name))).orElse(null);
        if (holder == null) {
            return missing(unit);
        }
        PigVariant variant = holder.value();
        JsonArray assets = new JsonArray();
        addAsset(assets, "", variant.modelAndTexture().asset().texturePath().toString());
        return build(unit, assets, variant.modelAndTexture().model().getSerializedName(), variant.spawnConditions());
    }

    private static void addAsset(JsonArray assets, String name, String id) {
        JsonObject asset = new JsonObject();
        asset.addProperty("name", name);
        asset.addProperty("id", id);
        assets.add(asset);
    }

    private static JsonObject build(
            String unit, JsonArray assets, String model, SpawnPrioritySelectors selectors) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.add("assets", assets);
        result.addProperty("model", model);

        // 选择器的顺序在数组里有语义（优先级链的书写顺序），必须原样保留。
        JsonArray selectorArray = new JsonArray();
        for (PriorityProvider.Selector<?, ?> selector : selectors.selectors()) {
            JsonObject item = new JsonObject();
            item.addProperty("priority", selector.priority());
            var condition = selector.condition().orElse(null);
            if (condition == null) {
                item.add("condition", null);
            } else {
                item.add("condition", conditionToJson((SpawnCondition) condition));
            }
            selectorArray.add(item);
        }
        result.add("selectors", selectorArray);
        return result;
    }

    private static JsonObject conditionToJson(SpawnCondition condition) {
        JsonObject result = new JsonObject();
        JsonArray entries = new JsonArray();

        if (condition instanceof BiomeCheck check) {
            result.addProperty("type", "biome");
            addHolderSet(entries, check.requiredBiomes());
        } else if (condition instanceof StructureCheck check) {
            result.addProperty("type", "structure");
            addHolderSet(entries, check.requiredStructures());
        } else if (condition instanceof MoonBrightnessCheck check) {
            result.addProperty("type", "moon_brightness");
            check.range().min().ifPresent(min -> result.addProperty("rangeMin", min));
            check.range().max().ifPresent(max -> result.addProperty("rangeMax", max));
        } else {
            result.addProperty("type", "unknown");
        }

        result.add("entries", entries);
        return result;
    }

    /**
     * 把一个 {@link HolderSet} 展开成条目列表。
     *
     * <p>标签引用保留 {@code #} 前缀：原版标签与内联元素在语义上不等价
     * （标签会随数据包内容变化），比对时必须能区分。
     */
    private static void addHolderSet(JsonArray entries, HolderSet<?> holderSet) {
        holderSet.unwrap().ifLeft(tagKey -> entries.add("#" + tagKey.location()))
                .ifRight(list -> list.forEach(holder -> entries.add(holder.unwrapKey()
                        .map(key -> key.identifier().toString())
                        .orElse(""))));
    }
}