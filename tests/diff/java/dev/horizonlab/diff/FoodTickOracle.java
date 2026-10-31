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

/**
 * 饥饿值每 tick 状态机的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.FoodTickOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>复刻 {@code FoodData.tick(ServerPlayer)} 的完整方法体。原版 tick 需要真实的
 * {@code ServerPlayer}（读难度、游戏规则、受伤状态、生命值，并调用 heal/hurtServer），
 * 且 {@code exhaustionLevel} 没有公开写访问器，因此这里按原版方法体逐行复算，
 * 把玩家侧的可变因素（难度 / 自然再生 / 是否受伤 / 生命值）作为输入显式传入。
 *
 * <p>单元表必须与 probes/food_tick.cpp 逐项一致。
 */
public final class FoodTickOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版 {@code addExhaustion} 的上限。 */
    private static final float MAX_EXHAUSTION = 40.0F;

    /** 单元表：{@code food|sat|exh|timer|difficulty|regen|hurt|health}。 */
    private static final List<String> UNITS =
            List.of(
                    "20|3|0|9|normal|true|true|10",
                    "20|3|0|8|normal|true|true|10",
                    "20|10|0|9|normal|true|true|10",
                    "19|3|0|9|normal|true|true|10",
                    "20|3|0|9|normal|true|false|10",
                    "20|3|0|9|normal|false|true|10",
                    "18|0|0|79|normal|true|true|10",
                    "18|0|0|78|normal|true|true|10",
                    "17|0|0|79|normal|true|true|10",
                    "0|0|0|79|easy|true|false|20",
                    "0|0|0|79|hard|true|false|10",
                    "0|0|0|79|normal|true|false|10",
                    "0|0|0|79|normal|true|false|1",
                    "0|0|0|79|hard|true|false|1",
                    "0|0|0|79|easy|true|false|5",
                    "20|3|5|0|normal|true|false|20",
                    "20|0|5|0|normal|true|false|20",
                    "20|0|5|0|peaceful|true|false|20",
                    "20|3|4|0|normal|true|false|20",
                    "15|2|0|50|normal|true|false|20",
                    "20|8|3|9|normal|true|true|10");

    private FoodTickOracle() {}

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
        System.err.println("usage: FoodTickOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 8) {
            throw new IllegalArgumentException("food_tick: 单元应为 8 段: " + unit);
        }

        int foodLevel = Integer.parseInt(parts[0]);
        float saturationLevel = Float.parseFloat(parts[1]);
        float exhaustionLevel = Float.parseFloat(parts[2]);
        int tickTimer = Integer.parseInt(parts[3]);
        String difficulty = parts[4];
        boolean regeneration = Boolean.parseBoolean(parts[5]);
        boolean hurt = Boolean.parseBoolean(parts[6]);
        float health = Float.parseFloat(parts[7]);

        float heal = 0.0F;
        float damage = 0.0F;

        // 1) exhaustion 消耗：任何分支之前，只判定一次。
        if (exhaustionLevel > 4.0F) {
            exhaustionLevel -= 4.0F;
            if (saturationLevel > 0.0F) {
                saturationLevel = Math.max(saturationLevel - 1.0F, 0.0F);
            } else if (!difficulty.equals("peaceful")) {
                foodLevel = Math.max(foodLevel - 1, 0);
            }
        }

        // 2) 四个互斥分支。
        if (regeneration && saturationLevel > 0.0F && hurt && foodLevel >= 20) {
            tickTimer++;
            if (tickTimer >= 10) {
                float f = Math.min(saturationLevel, 6.0F);
                heal = f / 6.0F;
                exhaustionLevel = Math.min(exhaustionLevel + f, MAX_EXHAUSTION);
                tickTimer = 0;
            }
        } else if (regeneration && foodLevel >= 18 && hurt) {
            tickTimer++;
            if (tickTimer >= 80) {
                heal = 1.0F;
                exhaustionLevel = Math.min(exhaustionLevel + 6.0F, MAX_EXHAUSTION);
                tickTimer = 0;
            }
        } else if (foodLevel <= 0) {
            tickTimer++;
            if (tickTimer >= 80) {
                if (health > 10.0F
                        || difficulty.equals("hard")
                        || (health > 1.0F && difficulty.equals("normal"))) {
                    damage = 1.0F;
                }
                tickTimer = 0;
            }
        } else {
            tickTimer = 0;
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("foodLevel", foodLevel);
        result.addProperty("saturationLevel", formatFloat(saturationLevel));
        result.addProperty("exhaustionLevel", formatFloat(exhaustionLevel));
        result.addProperty("tickTimer", tickTimer);
        result.addProperty("heal", formatFloat(heal));
        result.addProperty("damage", formatFloat(damage));
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