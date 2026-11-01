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
import net.minecraft.world.item.alchemy.PotionBrewing;
import net.minecraft.world.level.block.entity.BrewingStandBlockEntity;

/**
 * 酿造台每 tick 状态机的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BrewingTickOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>复刻 {@code BrewingStandBlockEntity.serverTick} 的完整方法体。该方法需要真实的
 * {@code Level} 与 {@code BlockEntity}（读配方、改方块状态、掉落物品），且 {@code brewTime}
 * / {@code fuel} 没有公开写访问器，因此这里按源码逐行复写，把外部可变因素（是否可酿造、
 * 燃料槽是否有燃料、试剂槽是否仍匹配）作为输入显式传入。
 *
 * <p>{@code FUEL_USES} 与固定酿造时长直接引用原版常量。
 *
 * <p>单元表必须与 probes/brewing_tick.cpp 逐项一致。
 */
public final class BrewingTickOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版 {@code BrewingStandBlockEntity} 里酿造的固定时长。 */
    private static final int BREW_TIME_TICKS = 400;

    /** 单元表：{@code brewTime|fuel|brewable|fuelSlotHasFuel|ingredientMatches}。 */
    private static final List<String> UNITS =
            List.of(
                    "200|10|true|false|true",
                    "1|10|true|false|true",
                    "1|10|false|false|true",
                    "100|10|true|false|false",
                    "0|10|true|false|true",
                    "0|0|true|true|true",
                    "0|10|false|false|true",
                    "0|5|false|true|true",
                    "0|0|true|false|true",
                    "0|1|true|false|true",
                    "0|1|false|false|true",
                    "2|10|true|false|true");

    private BrewingTickOracle() {}

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
                "usage: BrewingTickOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 5) {
            throw new IllegalArgumentException("brewing_tick: 单元应为 5 段: " + unit);
        }

        int brewTime = Integer.parseInt(parts[0]);
        int fuel = Integer.parseInt(parts[1]);
        boolean brewable = Boolean.parseBoolean(parts[2]);
        boolean fuelSlotHasFuel = Boolean.parseBoolean(parts[3]);
        boolean ingredientMatches = Boolean.parseBoolean(parts[4]);

        String ingredient = "";
        boolean brewed = false;
        boolean fuelConsumed = false;
        boolean fuelAdded = false;

        // 1) 燃料补充：设为 FUEL_USES（不是加）。
        if (fuel <= 0 && fuelSlotHasFuel) {
            fuel = BrewingStandBlockEntity.FUEL_USES;
            fuelAdded = true;
        }

        boolean brewing = brewTime > 0;

        if (brewing) {
            // 2a) 酿造中：先递减。
            brewTime--;
            if (brewTime == 0 && brewable) {
                brewed = true;
            } else if (!brewable || !ingredientMatches) {
                // 2b) 中途条件破坏 -> 取消。
                brewTime = 0;
            }
        } else if (brewable && fuel > 0) {
            // 3) 开始酿造：此刻扣燃料。
            fuel--;
            fuelConsumed = true;
            brewTime = BREW_TIME_TICKS;
            // 记录本次使用的试剂（固定取值即可，本能力不比较具体 id 的语义）。
            ingredient = "minecraft:blaze_powder";
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("brewTime", brewTime);
        result.addProperty("fuel", fuel);
        result.addProperty("ingredient", ingredient);
        result.addProperty("brewed", brewed);
        result.addProperty("fuelConsumed", fuelConsumed);
        result.addProperty("fuelAdded", fuelAdded);
        return result;
    }
}