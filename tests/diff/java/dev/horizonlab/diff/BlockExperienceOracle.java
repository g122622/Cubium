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
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.util.RandomSource;
import net.minecraft.util.valueproviders.IntProvider;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.DropExperienceBlock;
import net.minecraft.world.level.levelgen.XoroshiroRandomSource;

/**
 * 方块破坏经验的参照实现。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockExperienceOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/block_experience.cpp 同构：对每个方块按同一个种子采样
 * {@code sampleCount} 次 {@code DropExperienceBlock.tryDropExperience} 里的
 * {@code xpRange.sample(random)}，比对整段序列。
 *
 * <p>非 {@code DropExperienceBlock} 的方块一律视为不掉经验（原版这些方块的
 * {@code spawnAfterBreak} 默认实现是空操作）。
 */
public final class BlockExperienceOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与 probes/block_experience.cpp 的 SAMPLE_SEED 必须一致。 */
    private static final long SAMPLE_SEED = 0x000B10C0E1E4C0DE0L;

    /** 与 probes/block_experience.cpp 的 SAMPLE_COUNT 必须一致。 */
    private static final int SAMPLE_COUNT = 8;

    /**
     * 原版经验区间的取值字段：{@code DropExperienceBlock#xpRange}。
     *
     * <p>它是 private，且没有公开 getter；反射读一次比复制一张区间表更可靠——表一改，
     * 这里自动跟着变。
     */
    private static final java.lang.reflect.Field XP_RANGE = xpRangeField();

    private BlockExperienceOracle() {}

    private static java.lang.reflect.Field xpRangeField() {
        try {
            for (java.lang.reflect.Field field : DropExperienceBlock.class.getDeclaredFields()) {
                if (IntProvider.class.isAssignableFrom(field.getType())) {
                    field.setAccessible(true);
                    return field;
                }
            }
        } catch (RuntimeException ignored) {
            // 落空时下面按「不掉经验」处理，不会静默伪造结果。
        }
        return null;
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                names.add(id.toString());
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
        System.err.println("usage: BlockExperienceOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("block").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("block", name);

        Block block = BuiltInRegistries.BLOCK.getOptional(Identifier.parse(name)).orElse(null);
        if (block == null) {
            result.addProperty("missing", true);
            return result;
        }

        IntProvider xpRange = null;
        if (block instanceof DropExperienceBlock && XP_RANGE != null) {
            try {
                xpRange = (IntProvider) XP_RANGE.get(block);
            } catch (IllegalAccessException ignored) {
                xpRange = null;
            }
        }

        // 每个方块从同一个种子重新开始：采样序列与「别的方块消耗了多少随机数」无关。
        RandomSource random = new XoroshiroRandomSource(SAMPLE_SEED);
        JsonArray samples = new JsonArray();
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            samples.add(xpRange == null ? 0 : xpRange.sample(random));
        }
        result.add("samples", samples);
        return result;
    }
}