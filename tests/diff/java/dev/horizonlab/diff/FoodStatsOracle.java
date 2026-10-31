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
import java.util.List;
import net.minecraft.util.Mth;
import net.minecraft.world.food.FoodConstants;

/**
 * 饥饿值与饱和度的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.FoodStatsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>复刻 {@code FoodData} 的三处纯计算：{@code add}、{@code eat}、{@code addExhaustion}。
 *
 * <p>为什么不用原版的 {@code FoodData} 实例：它没有公开的无参构造器，且
 * {@code exhaustionLevel} 只有私有字段、没有公开读访问器（只有 NBT 读写路径）。
 * 为了能把中间状态喂进去，这里按原版公式逐行复算——公式与
 * {@code FoodData.java} 的对应行完全一致。
 *
 * <p>单元表必须与 probes/food_stats.cpp 逐项一致。
 */
public final class FoodStatsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版 {@code FoodData} 的饥饿值上限。 */
    private static final int MAX_FOOD_LEVEL = 20;

    /** 原版 {@code addExhaustion} 的上限。 */
    private static final float MAX_EXHAUSTION = 40.0F;

    /** 单元表，与 C++ 侧 UNITS 逐项一致。 */
    private static final List<String> UNITS =
            List.of(
                    "add|4|2.4|10|3",
                    "add|8|9.6|20|5",
                    "add|1|10|19|0",
                    "add|-25|0|10|3",
                    "add|-3|0|10|3",
                    "add|0|-10|10|3",
                    "add|100|100|0|0",
                    "add|-20|5|10|3",
                    "eat|4|0.6|10|3",
                    "eat|8|0.8|5|0",
                    "eat|1|0.1|19|0",
                    "eat|20|0.5|0|0",
                    "eat|6|1|14|2",
                    "eat|2|0|10|3",
                    "exhaust|4|0",
                    "exhaust|4|3.5",
                    "exhaust|1.5|3",
                    "exhaust|100|0",
                    "exhaust|0.25|39.9",
                    "exhaust|0|7");

    private FoodStatsOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: FoodStatsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|");
        int foodLevel;
        float saturationLevel;
        float exhaustionLevel = 0.0F;
        String mode;

        if (parts.length == 5 && (parts[0].equals("add") || parts[0].equals("eat"))) {
            mode = parts[0];
            foodLevel = Integer.parseInt(parts[3]);
            saturationLevel = Float.parseFloat(parts[4]);
            if (mode.equals("add")) {
                int food = Integer.parseInt(parts[1]);
                float saturation = Float.parseFloat(parts[2]);
                // 原版 add(food, saturation)：
                //   foodLevel       = clamp(food + foodLevel, 0, 20)
                //   saturationLevel = clamp(saturation + saturationLevel, 0, foodLevel)  <- 用新值
                foodLevel = Mth.clamp(food + foodLevel, 0, MAX_FOOD_LEVEL);
                saturationLevel = Mth.clamp(saturation + saturationLevel, 0.0F, (float) foodLevel);
            } else {
                int nutrition = Integer.parseInt(parts[1]);
                float modifier = Float.parseFloat(parts[2]);
                float saturation = FoodConstants.saturationByModifier(nutrition, modifier);
                foodLevel = Mth.clamp(nutrition + foodLevel, 0, MAX_FOOD_LEVEL);
                saturationLevel = Mth.clamp(saturation + saturationLevel, 0.0F, (float) foodLevel);
            }
        } else if (parts.length == 3 && parts[0].equals("exhaust")) {
            mode = parts[0];
            float delta = Float.parseFloat(parts[1]);
            exhaustionLevel = Float.parseFloat(parts[2]);
            // 原版 addExhaustion：Math.min(level + e, 40.0F)
            exhaustionLevel = Math.min(exhaustionLevel + delta, MAX_EXHAUSTION);
            foodLevel = MAX_FOOD_LEVEL;
            saturationLevel = 5.0F;
        } else {
            throw new IllegalArgumentException("food_stats: 无法识别的单元: " + unit);
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("mode", mode);
        result.addProperty("foodLevel", foodLevel);
        result.addProperty("saturationLevel", formatFloat(saturationLevel));
        result.addProperty("exhaustionLevel", formatFloat(exhaustionLevel));
        return result;
    }

    private static String formatFloat(double value) {
        String text = String.format("%.6g", value);
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