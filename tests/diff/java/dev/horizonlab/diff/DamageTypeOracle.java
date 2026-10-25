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
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.damagesource.DamageType;

/**
 * 伤害类型数据包字段的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DamageTypeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/damage_type.cpp 同构：
 *
 * <pre>
 *   { "damageType": "minecraft:in_fire", "scaling": "when_caused_by_living_non_player" }
 * </pre>
 *
 * <p>数据来自 `data/minecraft/damage_type/*.json`，走 {@link DataPackRegistries} 加载。
 */
public final class DamageTypeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private DamageTypeOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            var types = registries.lookup.lookupOrThrow(Registries.DAMAGE_TYPE);
            List<String> names = new ArrayList<>();
            types.listElements().map(holder -> holder.key().identifier().toString()).forEach(names::add);
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
        System.err.println("usage: DamageTypeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var types = registries.lookup.lookupOrThrow(Registries.DAMAGE_TYPE);

        List<JsonObject> results = new ArrayList<>();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("damageType").getAsString();
            results.add(summarise(types, name));
        }
        results.sort((left, right) -> left.get("damageType").getAsString().compareTo(right.get("damageType").getAsString()));

        StringBuilder text = new StringBuilder();
        for (JsonObject result : results) {
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(net.minecraft.core.HolderLookup.RegistryLookup<DamageType> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("damageType", name);
        var key = ResourceKey.create(Registries.DAMAGE_TYPE, Identifier.parse(name));
        var holder = registry.get(key).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }
        DamageType type = holder.value();
        result.addProperty("scaling", type.scaling().getSerializedName());
        result.addProperty("exhaustion", Double.parseDouble(String.format(java.util.Locale.ROOT, "%.5f", type.exhaustion())));
        result.addProperty("effects", type.effects().getSerializedName());
        return result;
    }
}