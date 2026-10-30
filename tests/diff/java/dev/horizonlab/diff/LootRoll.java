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
import java.util.List;
import net.minecraft.util.RandomSource;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.level.levelgen.XoroshiroRandomSource;
import net.minecraft.world.level.storage.loot.LootParams;
import net.minecraft.world.level.storage.loot.LootTable;
import net.minecraft.world.level.storage.loot.parameters.LootContextParamSets;

/**
 * 三个 loot oracle 共用的「读输入 → 掷骰 → 写输出」流程。
 *
 * <p>关键约束：**掷骰语义必须与 C++ 探针逐位一致**。
 *
 * <ul>
 *   <li>随机源：{@code new XoroshiroRandomSource(seed)}，与 Cubium 的
 *       {@code Xoroshiro128ppRandom} 同一算法；
 *   <li>样本间推进：第 i 个样本先 {@code nextLong()} {@code i × 64} 次再掷骰；
 *   <li>原版对外入口只有 {@link LootTable#getRandomItems(LootParams, RandomSource)}；
 *       它把参数封进 {@code LootContext} 后走与私有重载完全相同的生成器，所以这里
 *       传显式随机源，保证「种子固定、每次重放」。
 * </ul>
 *
 * <p>输出字段：{@code samples} 是长度固定的数组，每项是按物品名、再按数量排序后的
 * {@code [{id, count}, ...]}。
 */
final class LootRoll {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与 probes/loot_probe_common.hpp 的 ROLL_SEED 必须一致。 */
    static final long ROLL_SEED = 0x0005EED2026A11CE0L;

    /** 与 probes/loot_probe_common.hpp 的 ROLL_SAMPLES 必须一致。 */
    static final int ROLL_SAMPLES = 4;

    /** 与 probes/loot_probe_common.hpp 的 SAMPLE_STRIDE 必须一致。 */
    static final int SAMPLE_STRIDE = 64;

    /** 一个单元要掷的表与上下文构造方式。 */
    interface ContextFactory {
        /**
         * 用给定随机源构造 {@code LootParams}；返回 null 表示该单元无法构造上下文。
         *
         * @param random 每次掷骰的随机源（已按样本推进过）
         */
        LootParams create(RandomSource random);
    }

    /** 按 {@link ContextFactory} 掷满 {@link #ROLL_SAMPLES} 个样本。 */
    static JsonArray samples(LootTable table, ContextFactory factory) {
        JsonArray samples = new JsonArray();
        for (int sample = 0; sample < ROLL_SAMPLES; ++sample) {
            final RandomSource random = randomForSample(sample);
            final JsonArray stacks = new JsonArray();
            LootParams params = null;
            final RandomSource randomForRoll = random;
            try {
                params = factory.create(random);
            } catch (IllegalArgumentException missing) {
                // 参数集校验失败（缺必需参数）：两侧都按「该样本无产出」处理，
                // 差异由条目/条件层面暴露，而不是让整个 oracle 崩掉。
            }
            List<ItemStack> items = new ArrayList<>();
            if (params != null) {
                // 与仓库里原来的写法一致：显式传随机源。
                items.addAll(table.getRandomItems(params, randomForRoll));
            }
            for (ItemStack stack : items) {
                if (stack.isEmpty()) {
                    continue;
                }
                JsonObject entry = new JsonObject();
                entry.addProperty("id", stack.getItemHolder().unwrapKey().orElseThrow().identifier().toString());
                entry.addProperty("count", stack.getCount());
                stacks.add(entry);
            }
            sortStacks(stacks);
            samples.add(stacks);
        }
        return samples;
    }

    /**
     * 按原版 `LootTable.createStackSplitter` 的口径合并物品堆：超出一组上限的大堆会被
     * 拆成多组。C++ 探针的 `LootTable::generate` 用的是同一口径，两侧因此可比。
     *
     * <p>与 `getRandomItems` 的唯一区别是不在这里按堆排序：排序由调用方按需做。
     */
    static List<ItemStack> splitIntoStacks(List<ItemStack> stacks) {
        List<ItemStack> result = new ArrayList<>();
        for (ItemStack stack : stacks) {
            if (stack.isEmpty()) {
                continue;
            }
            if (stack.getCount() < stack.getMaxStackSize()) {
                result.add(stack);
                continue;
            }
            int remaining = stack.getCount();
            while (remaining > 0) {
                ItemStack piece = stack.copyWithCount(Math.min(stack.getMaxStackSize(), remaining));
                remaining -= piece.getCount();
                result.add(piece);
            }
        }
        return result;
    }

    /** 第 index 个样本使用的随机源：同一种子起始，再推进 index × SAMPLE_STRIDE 步。 */
    static RandomSource randomForSample(int index) {
        RandomSource random = new XoroshiroRandomSource(ROLL_SEED);
        for (int i = 0; i < index * SAMPLE_STRIDE; ++i) {
            random.nextLong();
        }
        return random;
    }

    /**
     * 稳定排序：物品名升序，名字相同按数量升序。
     *
     * <p>Cubium 侧在 C++ 里做了同样的排序，两边必须对同一输入给出同一顺序。
     */
    private static void sortStacks(JsonArray stacks) {
        List<JsonObject> items = new ArrayList<>();
        stacks.forEach(element -> items.add(element.getAsJsonObject()));
        items.sort((left, right) -> {
            int byId = left.get("id").getAsString().compareTo(right.get("id").getAsString());
            if (byId != 0) {
                return byId;
            }
            return Integer.compare(left.get("count").getAsInt(), right.get("count").getAsInt());
        });
        for (int i = 0; i < items.size(); ++i) {
            stacks.set(i, items.get(i));
        }
    }

    /** 读输入文件、逐行掷骰、写输出文件。 */
    static void run(Path input, Path output, String key, Handler handler) throws IOException {
        LootServerData data = LootServerData.get();
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String id = request.get(key).getAsString();
            JsonObject result = new JsonObject();
            result.addProperty(key, id);
            // 单元键可以带场景后缀（`表#场景`）：表名只取 `#` 之前的部分，键原样回写。
            int separator = id.lastIndexOf('#');
            LootTable table = data.table(separator < 0 ? id : id.substring(0, separator));
            if (table == null) {
                result.addProperty("missing", true);
            } else {
                handler.handle(table, request, result);
            }
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /** 单个单元的处理方式：往 {@code result} 里填 {@code samples}。 */
    interface Handler {
        void handle(LootTable table, JsonObject request, JsonObject result);
    }
}