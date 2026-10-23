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
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.Registry;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.food.FoodProperties;
import net.minecraft.world.item.Item;

/**
 * 物品注册表与食物属性的原版 oracle。
 *
 * <p>与 C++ probe（tests/diff/probes/item_summary.cpp）同构，读同一份输入、写同样格式的输出，
 * 因此两侧可以逐字节比对。数据直接来自原版 jar 的运行时注册表，不经过任何第三方转写。
 *
 * <p>用法：
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemSummaryOracle list
 *   java dev.horizonlab.diff.ItemSummaryOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 */
public final class ItemSummaryOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ItemSummaryOracle() {}

    public static void main(String[] args) throws IOException {
        // 必须先确定版本：Util.fetchChoiceType 会拿 SharedConstants.getCurrentVersion()
        // 的 dataVersion 去 DataFixers 查 schema，未设置时初始化直接失败。
        net.minecraft.SharedConstants.tryDetectVersion();

        // 原版注册表在静态初始化时会断言 bootstrap 已执行；Bootstrap.bootStrap() 负责
        // 构建全部内置注册表（物品、方块、实体……）。不走这一步，任何 BuiltInRegistries
        // 访问都会抛 "Not bootstrapped"。
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length >= 1 && args[0].equals("list")) {
            writeList();
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: ItemSummaryOracle list | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void writeList() throws IOException {
        List<String> names = new ArrayList<>();
        for (Identifier id : BuiltInRegistries.ITEM.keySet()) {
            names.add(id.toString());
        }
        names.sort(Comparator.naturalOrder());
        BufferedWriter writer =
                new BufferedWriter(new OutputStreamWriter(System.out, StandardCharsets.UTF_8));
        for (String name : names) {
            writer.write(name);
            writer.write('\n');
        }
        writer.flush();
    }

    private static void run(Path input, Path output) throws IOException {
        try (BufferedReader reader = Files.newBufferedReader(input, StandardCharsets.UTF_8);
                BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.isBlank()) {
                    continue;
                }
                JsonObject request = JsonParser.parseString(line).getAsJsonObject();
                writer.write(GSON.toJson(summarise(request.get("item").getAsString())));
                writer.write('\n');
            }
        }
    }

    /** 单个物品的摘要，字段与 C++ probe 完全对应。 */
    private static JsonObject summarise(String itemId) {
        JsonObject result = new JsonObject();
        result.addProperty("item", itemId);

        Item item = BuiltInRegistries.ITEM.getOptional(Identifier.parse(itemId)).orElse(null);
        if (item == null) {
            result.addProperty("missing", true);
            return result;
        }

        // vanilla 1.20.5+ 用数据组件表达堆叠上限。MAX_STACK_SIZE 是 Optional<Integer>，
        // 缺省时用 Item 的 getDefaultMaxStackSize()（等价于 64）。
        int stackSize = item.components()
                .getOrDefault(
                        net.minecraft.core.component.DataComponents.MAX_STACK_SIZE,
                        item.getDefaultMaxStackSize());
        result.addProperty("maxStackSize", stackSize);

        FoodProperties food = item.components().get(net.minecraft.core.component.DataComponents.FOOD);
        result.addProperty("isFood", food != null);
        if (food != null) {
            // FoodProperties#nutrition() 是饥饿值。
            // FoodProperties#saturation() 在 1.21.11 里的含义是「每点饥饿值贡献的饱和度」，
            // 已经等于 vanilla FoodData 公式里的 `saturationModifier * 2`（苹果 0.3 × 2 = 0.6？
            // 不是：实测原版返回 2.4 = 4 × 0.3 × 2，即它对整个食物已乘过 nutrition）。
            // 因此原始修正值要除掉 nutrition × 2 才能还原成 Food 对象里的 modifier。
            result.addProperty("hunger", food.nutrition());
            float rawSaturation = food.saturation();
            float modifier = food.nutrition() == 0 ? rawSaturation : rawSaturation / (food.nutrition() * 2.0F);
            result.addProperty("saturationModifier", floatBits(modifier));
            result.addProperty("saturation", floatBits(rawSaturation));
        }
        return result;
    }

    /** 与 C++ floatBits 一致：Java float 的位模式，8 位小写十六进制。 */
    private static String floatBits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }

    /** 供未来 harness 使用：把注册表整体导出成 JSON 数组。 */
    @SuppressWarnings("unused")
    static JsonArray exportAll() {
        JsonArray array = new JsonArray();
        for (Identifier id : BuiltInRegistries.ITEM.keySet()) {
            array.add(summarise(id.toString()));
        }
        return array;
    }

    @SuppressWarnings("unused")
    private static Registry<Item> items() {
        return BuiltInRegistries.ITEM;
    }
}