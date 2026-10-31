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
import net.minecraft.world.level.block.entity.trialspawner.TrialSpawnerConfig;

/**
 * 试炼刷怪笼配置的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.TrialSpawnerConfigOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/trial_spawner_config.cpp 同构：比较四个浮点字段的原值、两个整数字段、
 * 生成潜力与战利品表条目数，以及两个派生结果
 * {@code calculateTargetTotalMobs(n)} / {@code calculateTargetSimultaneousMobs(n)}。
 *
 * <p>派生方法在原版是
 * {@code (int) Math.floor(totalMobs + totalMobsAddedPerPlayer * n)}，
 * 全程 f32 运算后 floor。这里直接调用原版方法，不做任何重写——这正是本能力要钉住的
 * 语义：字段一旦被当成整数，结果就会少一只。
 */
public final class TrialSpawnerConfigOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private TrialSpawnerConfigOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            List<String> names = new ArrayList<>();
            registries
                    .lookup
                    .lookupOrThrow(Registries.TRIAL_SPAWNER_CONFIG)
                    .listElements()
                    .forEach(holder -> names.add(holder.key().identifier().toString()));
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
        System.err.println(
                "usage: TrialSpawnerConfigOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var configs = registries.lookup.lookupOrThrow(Registries.TRIAL_SPAWNER_CONFIG);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            var obj = JsonParser.parseString(line).getAsJsonObject();
            String name = obj.get("config").getAsString();
            int playerCount = obj.get("playerCount").getAsInt();
            text.append(GSON.toJson(summarise(configs, name, playerCount))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(
            net.minecraft.core.HolderLookup.RegistryLookup<TrialSpawnerConfig> registry,
            String name,
            int playerCount) {
        JsonObject result = new JsonObject();
        result.addProperty("config", name);

        var holder =
                registry.get(ResourceKey.create(Registries.TRIAL_SPAWNER_CONFIG, Identifier.parse(name)))
                        .orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }

        TrialSpawnerConfig config = holder.value();
        result.addProperty("playerCount", playerCount);
        result.addProperty("spawnRange", config.spawnRange());
        result.addProperty("totalMobs", formatFloat(config.totalMobs()));
        result.addProperty("simultaneousMobs", formatFloat(config.simultaneousMobs()));
        result.addProperty("totalMobsAddedPerPlayer", formatFloat(config.totalMobsAddedPerPlayer()));
        result.addProperty(
                "simultaneousMobsAddedPerPlayer", formatFloat(config.simultaneousMobsAddedPerPlayer()));
        result.addProperty("ticksBetweenSpawn", config.ticksBetweenSpawn());
        result.addProperty("spawnPotentialCount", config.spawnPotentialsDefinition().unwrap().size());
        result.addProperty("lootTableCount", config.lootTablesToEject().unwrap().size());
        // 直接调用原版派生方法，不做重写。
        result.addProperty("targetTotalMobs", config.calculateTargetTotalMobs(playerCount));
        result.addProperty(
                "targetSimultaneousMobs", config.calculateTargetSimultaneousMobs(playerCount));
        return result;
    }

    private static String formatFloat(float value) {
        String text = String.format("%.6g", (double) value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }
}