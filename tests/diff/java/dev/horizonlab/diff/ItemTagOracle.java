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
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.tags.TagKey;
import net.minecraft.world.item.Item;

/**
 * 物品标签展开成员的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemTagOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/item_tags.cpp 同构：
 *
 * <pre>
 *   { "tag": "minecraft:planks", "values": ["minecraft:acacia_planks", ...] }
 * </pre>
 *
 * <p>values 是**展开全部 #tag 引用之后**的物品集合（原版 {@code Tag#getValues()} 的口径），
 * 顺序按字符串排序后与 probe 一致。
 */
public final class ItemTagOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ItemTagOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            var tags = registries.lookup.lookupOrThrow(Registries.ITEM).listTags().toList();
            List<String> names = new ArrayList<>();
            for (var tag : tags) {
                names.add(tag.key().location().toString());
            }
            Collections.sort(names);
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
        System.err.println("usage: ItemTagOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var items = registries.lookup.lookupOrThrow(Registries.ITEM);

        List<JsonObject> results = new ArrayList<>();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("tag").getAsString();
            results.add(summarise(items, name));
        }
        results.sort((left, right) -> left.get("tag").getAsString().compareTo(right.get("tag").getAsString()));

        StringBuilder text = new StringBuilder();
        for (JsonObject result : results) {
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(net.minecraft.core.HolderLookup.RegistryLookup<Item> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("tag", name);
        TagKey<Item> key = TagKey.create(Registries.ITEM, Identifier.parse(name));
        var holders = registry.get(key).orElse(null);
        if (holders == null) {
            result.addProperty("missing", true);
            return result;
        }
        List<String> values = new ArrayList<>();
        for (var holder : holders) {
            values.add(holder.unwrapKey().orElseThrow().identifier().toString());
        }
        Collections.sort(values);
        JsonArray array = new JsonArray();
        values.forEach(array::add);
        result.add("values", array);
        return result;
    }

}