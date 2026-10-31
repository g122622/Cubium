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
import java.util.Optional;
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.food.FoodProperties;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;

/**
 * 食物属性的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.FoodPropertiesOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/food_properties.cpp 同构：比较饥饿值、饱和度修正、能否饱腹后继续吃。
 * 这三项在原版来自 `DataComponents.FOOD`（`FoodProperties` record）。
 */
public final class FoodPropertiesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private FoodPropertiesOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (var item : BuiltInRegistries.ITEM) {
                if (foodOf(item).isPresent()) {
                    names.add(BuiltInRegistries.ITEM.getKey(item).toString());
                }
            }
            names.sort(Comparator.naturalOrder());
            Files.writeString(Path.of(args[1]), String.join("\n", names) + "\n", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: FoodPropertiesOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("item").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("item", name);

        var optional = BuiltInRegistries.ITEM.get(Identifier.parse(name));
        if (optional.isEmpty()) {
            result.addProperty("missing", true);
            return result;
        }

        Optional<FoodProperties> food = foodOf(optional.get().value());
        if (food.isEmpty()) {
            result.addProperty("food", false);
            return result;
        }

        FoodProperties properties = food.get();
        result.addProperty("food", true);
        result.addProperty("nutrition", properties.nutrition());
        // 比较**修正系数**本身。
        //
        // 原版 `FoodProperties.saturation` 存的是「已经换算过的饱和值」
        // （`FoodConstants.saturationByModifier` 在食用时才乘 `nutrition × 2`，
        // 而 1.21 的组件化改动把这个乘积直接写进了字段）。Cubium 的
        // `Food::getSaturationModifier()` 存的是系数。这里反算回系数，
        // 避免在探针侧正算引入 `2.4f vs 2.4000001f` 这种运算顺序噪声。
        int nutrition = properties.nutrition();
        float modifier = nutrition == 0 ? 0.0f : properties.saturation() / (nutrition * 2.0f);
        // 按固定 6 位有效数字输出字符串：f32 的 0.3f 若直接给 Gson，会被当成 double 打印成
        // 0.3，而 C++ 侧 nlohmann 会把同一个 f32 精确展开成 0.30000001192092896。
        // 两者是同一个数值，差别只在打印精度，统一口径后可逐字节比较。
        //
        // 注意必须**去掉尾随零**：C++ 的 `%.6g` 会把 0.3 打成 "0.3"，而 Java 的
        // `%.6g` 打成 "0.300000"。两侧都用「6 位有效数字再去掉尾随零」的规则，即
        // Java 的 `Float.toString` 口径。
        result.addProperty("saturationModifier", trimTrailingZeros(modifier));
        result.addProperty("canAlwaysEat", properties.canAlwaysEat());
        return result;
    }

    /**
     * 取物品的食物属性。
     *
     * <p>`FoodProperties` 在 1.21 之后挂在数据组件 `DataComponents.FOOD` 上，而不再由
     * `Item` 直接暴露 getter。用一个默认堆叠读取，与原版食用判定走同一条路径。
     */
    private static Optional<FoodProperties> foodOf(Item item) {
        return Optional.ofNullable(new ItemStack(item).get(DataComponents.FOOD));
    }

    /**
     * 6 位有效数字后去掉尾随零，与 C++ 侧 `%.6g` 的输出对齐。
     *
     * <p>例如 0.3f → {@code "0.3"}、1.2f → {@code "1.2"}、0.35f → {@code "0.35"}。
     */
    private static String trimTrailingZeros(float value) {
        String text = String.format(java.util.Locale.ROOT, "%.6g", value);
        if (text.indexOf('e') >= 0 || text.indexOf('E') >= 0) {
            return text;
        }
        if (text.indexOf('.') < 0) {
            return text;
        }
        int end = text.length();
        while (end > 0 && text.charAt(end - 1) == '0') {
            end--;
        }
        if (end > 0 && text.charAt(end - 1) == '.') {
            end--;
        }
        return text.substring(0, end);
    }
}