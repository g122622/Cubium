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
import java.util.Comparator;
import java.util.List;
import net.minecraft.world.entity.MobCategory;

/**
 * 实体分类（原版 {@link MobCategory}）生成参数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MobCategoryOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/mob_category.cpp 同构：
 *
 * <pre>
 *   { "category": "minecraft:monster", "maxCount": 70, "peaceful": false, "persistent": false,
 *     "despawnDistance": 128, "randomDespawnDistance": 32 }
 * </pre>
 */
public final class MobCategoryOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private MobCategoryOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (MobCategory category : MobCategory.values()) {
                names.add("minecraft:" + category.getName());
            }
            names.sort(Comparator.naturalOrder());
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
        System.err.println("usage: MobCategoryOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("category").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("category", name);
        String bare = name.startsWith("minecraft:") ? name.substring("minecraft:".length()) : name;

        MobCategory category = null;
        for (MobCategory candidate : MobCategory.values()) {
            if (candidate.getName().equals(bare)) {
                category = candidate;
                break;
            }
        }
        if (category == null) {
            result.addProperty("missing", true);
            return result;
        }

        result.addProperty("maxCount", category.getMaxInstancesPerChunk());
        result.addProperty("peaceful", category.isFriendly());
        result.addProperty("persistent", category.isPersistent());
        // 原版没有「消失距离」字段：怪物 128 格立即消失、其余持久/非持久分类各自
        // 由 despawnDistance 规则处理。这里只导出 oracle 真有的字段，其余由
        // Cubium 侧单独维护（README 已记为未对齐项）。
        return result;
    }
}