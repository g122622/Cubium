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
 * 铁砧合并耐久度的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AnvilDurabilityOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>这段逻辑在 {@code AnvilMenu.createResult} 内部，与槽位强耦合、没有可独立调用的
 * 方法，因此按其源码逐行复写。用到的都是 {@code int} 运算，与原版保持一致。
 *
 * <p>单元表必须与 probes/anvil_durability.cpp 逐项一致。
 */
public final class AnvilDurabilityOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 合并时副物品额外贡献的耐久占主物品上限的百分比。 */
    private static final int DURABILITY_BONUS_PERCENT = 12;

    /** 合并成功时增加的经验代价。 */
    private static final int MERGE_COST = 2;

    /** 单元表：{@code primaryMax|primaryDamage|secondaryMax|secondaryDamage}。 */
    private static final List<String> UNITS =
            List.of(
                    "1561|800|1561|800",
                    "1561|0|1561|800",
                    "1561|1500|1561|0",
                    "1561|1560|1561|1560",
                    "1561|800|1561|1561",
                    "100|50|100|50",
                    "99|50|99|50",
                    "10|5|10|5",
                    "1|0|1|0",
    // 顺序敏感且不会钳到 0：正确得 newDamage=1152，先除后乘得 1159
                    "1561|1500|1561|1400",
                    "2031|1000|2031|1000",
                    "432|200|432|200",
                    "250|100|250|100",
                    "178956970|1000|178956970|1000",
                    "178956971|1000|178956971|1000",
                    "2000000000|1000|2000000000|1000");

    private AnvilDurabilityOracle() {}

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
        System.err.println(
                "usage: AnvilDurabilityOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 4) {
            throw new IllegalArgumentException("anvil_durability: 单元应为 4 段: " + unit);
        }

        int primaryMax = Integer.parseInt(parts[0]);
        int primaryDamage = Integer.parseInt(parts[1]);
        int secondaryMax = Integer.parseInt(parts[2]);
        int secondaryDamage = Integer.parseInt(parts[3]);

        // 原版 createResult 里合并耐久那一段，逐行对应。
        int primaryDurability = primaryMax - primaryDamage;
        int secondaryDurability = secondaryMax - secondaryDamage;
        // 先乘后除：这是原版 int 运算的顺序。
        int bonus = secondaryDurability + primaryMax * DURABILITY_BONUS_PERCENT / 100;
        int totalDurability = primaryDurability + bonus;
        int newDamage = primaryMax - totalDurability;
        if (newDamage < 0) {
            newDamage = 0;
        }

        boolean repaired = newDamage < primaryDamage;
        int finalDamage = repaired ? newDamage : primaryDamage;
        int costAdded = repaired ? MERGE_COST : 0;

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("primaryMax", primaryMax);
        result.addProperty("primaryDamage", primaryDamage);
        result.addProperty("secondaryMax", secondaryMax);
        result.addProperty("secondaryDamage", secondaryDamage);
        result.addProperty("newDamage", finalDamage);
        result.addProperty("costAdded", costAdded);
        result.addProperty("repaired", repaired);
        return result;
    }
}
