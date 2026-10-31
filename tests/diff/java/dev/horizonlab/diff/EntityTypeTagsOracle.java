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
import net.minecraft.world.entity.EntityType;

/**
 * 实体类型标签的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntityTypeTagsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/entity_type_tags.cpp 同构：列出每个标签包含哪些实体类型。成员排序后输出，
 * 原版标签是集合，顺序无语义。
 *
 * <p>标签内容来自数据包（`data/minecraft/tags/entity_type/*.json`），必须走
 * {@link DataPackRegistries} 加载之后才非空；`BuiltInRegistries.ENTITY_TYPE.getTags()`
 * 在只有 Bootstrap 的情况下只返回少数几个内联标签，不能用它当全集。
 */
public final class EntityTypeTagsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EntityTypeTagsOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            var tags = registries.lookup.lookupOrThrow(Registries.ENTITY_TYPE).listTags().toList();
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
        System.err.println("usage: EntityTypeTagsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var entityTypes = registries.lookup.lookupOrThrow(Registries.ENTITY_TYPE);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("tag").getAsString();
            text.append(GSON.toJson(summarise(entityTypes, name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(
            net.minecraft.core.HolderLookup.RegistryLookup<EntityType<?>> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("tag", name);

        TagKey<EntityType<?>> key = TagKey.create(Registries.ENTITY_TYPE, Identifier.parse(name));
        var holders = registry.get(key).orElse(null);
        if (holders == null) {
            result.addProperty("missing", true);
            return result;
        }

        List<String> members = new ArrayList<>();
        for (var holder : holders) {
            var unwrapped = holder.unwrapKey().orElse(null);
            if (unwrapped != null) {
                members.add(unwrapped.identifier().toString());
            }
        }
        Collections.sort(members);

        JsonArray array = new JsonArray();
        members.forEach(array::add);
        result.add("members", array);
        return result;
    }
}