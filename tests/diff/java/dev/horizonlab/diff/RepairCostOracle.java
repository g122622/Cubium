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
import net.minecraft.world.inventory.AnvilMenu;

/**
 * 铁砧修复代价的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RepairCostOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>翻倍那一步直接调原版 {@link AnvilMenu#calculateIncreasedRepairCost}。
 * 「取 max」与「`k != i || k == 0` 才增长」两段在 {@code createResult} 内部、与槽位强耦合，
 * 这里按其源码逐行复写——这两段没有独立的方法可以调用。
 *
 * <p>单元表必须与 probes/repair_cost.cpp 逐项一致。
 */
public final class RepairCostOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表：{@code primaryCost|secondaryCost|repairCount|xpCost}。 */
    private static final List<String> UNITS =
            List.of(
                    "0|0|0|1",
                    "3|0|2|5",
                    "3|0|2|2",
                    "0|0|0|0",
                    "10|0|5|5",
                    "3|9|1|4",
                    "9|3|1|4",
                    "5|5|1|4",
                    "4|0|0|7",
                    "1073741823|0|1|9",
                    "2147483647|0|1|9",
                    "2147483647|0|4|4",
                    "100|0|20|20",
                    "100|0|20|21");

    private RepairCostOracle() {}

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
        System.err.println("usage: RepairCostOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            throw new IllegalArgumentException("repair_cost: 单元应为 4 段: " + unit);
        }

        int primaryCost = Integer.parseInt(parts[0]);
        int secondaryCost = Integer.parseInt(parts[1]);
        int repairCount = Integer.parseInt(parts[2]);
        int xpCost = Integer.parseInt(parts[3]);

        // 原版：if (i3 < itemstack2.getOrDefault(REPAIR_COST, 0)) i3 = ...;
        int cost = Math.max(primaryCost, secondaryCost);

        // 原版：if (k != i || k == 0) i3 = calculateIncreasedRepairCost(i3);
        boolean shouldIncrease = (xpCost != repairCount) || (xpCost == 0);
        int nextCost = shouldIncrease ? AnvilMenu.calculateIncreasedRepairCost(cost) : cost;

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("primaryCost", primaryCost);
        result.addProperty("secondaryCost", secondaryCost);
        result.addProperty("repairCount", repairCount);
        result.addProperty("xpCost", xpCost);
        result.addProperty("nextCost", nextCost);
        result.addProperty("increased", shouldIncrease);
        return result;
    }
}