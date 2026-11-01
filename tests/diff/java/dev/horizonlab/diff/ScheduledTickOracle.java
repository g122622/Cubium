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
import java.util.List;
import net.minecraft.core.BlockPos;
import net.minecraft.world.ticks.ScheduledTick;
import net.minecraft.world.ticks.TickPriority;

/**
 * {@code ScheduledTick} 排序与去重的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ScheduledTickOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ScheduledTick#DRAIN_ORDER} /
 * {@link ScheduledTick#INTRA_TICK_DRAIN_ORDER} / {@link ScheduledTick#UNIQUE_TICK_HASH}，
 * 不重写比较逻辑。
 *
 * <p>单元表必须与 probes/scheduled_tick.cpp 逐项一致。
 */
public final class ScheduledTickOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "drain|10|NORMAL|0|20|NORMAL|0",
                    "drain|20|NORMAL|0|10|NORMAL|0",
                    "drain|10|HIGH|0|10|NORMAL|0",
                    "drain|10|NORMAL|0|10|HIGH|0",
                    "drain|10|EXTREMELY_HIGH|0|10|EXTREMELY_LOW|0",
                    "drain|10|EXTREMELY_LOW|0|10|EXTREMELY_HIGH|0",
                    "drain|10|NORMAL|5|10|NORMAL|3",
                    "drain|10|NORMAL|3|10|NORMAL|5",
                    "drain|10|NORMAL|0|10|NORMAL|0",
                    "drain|0|VERY_HIGH|0|0|HIGH|0",
                    "drain|0|HIGH|0|0|NORMAL|0",
                    "drain|0|NORMAL|0|0|LOW|0",
                    "drain|0|LOW|0|0|VERY_LOW|0",
                    "drain|0|VERY_LOW|0|0|EXTREMELY_LOW|0",
                    "intra|NORMAL|0|NORMAL|5",
                    "intra|NORMAL|5|NORMAL|0",
                    "intra|HIGH|0|NORMAL|0",
                    "intra|NORMAL|0|HIGH|0",
                    "intra|EXTREMELY_HIGH|9|EXTREMELY_LOW|0",
                    "intra|NORMAL|3|NORMAL|3",
                    "unique|fnv|1|fnv|1",
                    "unique|fnv|1|fnv|2",
                    "unique|fnv|1|fluid|1",
                    "unique|fnv|1|fluid|2",
                    "unique|block|9|block|9",
                    "unique|fluid|5|fluid|5",
                    "sort|10/NORMAL/0,5/HIGH/2,5/EXTREMELY_HIGH/5,5/NORMAL/1",
                    "sort|0/NORMAL/3,0/NORMAL/1,0/NORMAL/2",
                    "sort|100/LOW/0,50/VERY_HIGH/9,75/NORMAL/4");

    private ScheduledTickOracle() {}

    public static void main(String[] args) throws IOException {
        DataPackRegistries.load();
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
                "usage: ScheduledTickOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /**
     * 构造一条 `ScheduledTick`。
     *
     * <p>`type` 用字符串代表调度标识；`posSeed` 生成 `(seed, seed, seed)` 的坐标。
     */
    private static ScheduledTick<String> makeTick(
            long triggerTick, String priority, long subTickOrder, String type, int posSeed) {
        return new ScheduledTick<>(
                type,
                new BlockPos(posSeed, posSeed, posSeed),
                triggerTick,
                TickPriority.valueOf(priority),
                subTickOrder);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("scheduled_tick: 单元至少两段: " + unit);
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        switch (parts[0]) {
            case "drain" -> {
                if (parts.length != 7) {
                    throw new IllegalArgumentException("scheduled_tick: drain 需要 6 个参数: " + unit);
                }
                ScheduledTick<String> a =
                        makeTick(Long.parseLong(parts[1]), parts[2], Long.parseLong(parts[3]), "t", 0);
                ScheduledTick<String> b =
                        makeTick(Long.parseLong(parts[4]), parts[5], Long.parseLong(parts[6]), "t", 0);
                // 原版比较器返回的可能不是 ±1，统一归一化。
                int raw = ScheduledTick.DRAIN_ORDER.compare(a, b);
                result.addProperty("value", Integer.signum(raw));
            }
            case "intra" -> {
                if (parts.length != 5) {
                    throw new IllegalArgumentException("scheduled_tick: intra 需要 4 个参数: " + unit);
                }
                ScheduledTick<String> a = makeTick(0, parts[1], Long.parseLong(parts[2]), "t", 0);
                ScheduledTick<String> b = makeTick(0, parts[3], Long.parseLong(parts[4]), "t", 0);
                int raw = ScheduledTick.INTRA_TICK_DRAIN_ORDER.compare(a, b);
                result.addProperty("value", Integer.signum(raw));
            }
            case "unique" -> {
                if (parts.length != 5) {
                    throw new IllegalArgumentException("scheduled_tick: unique 需要 4 个参数: " + unit);
                }
                // 原版 `UNIQUE_TICK_HASH.equals` 用 `==` 比较 type（引用比较）。单元里的名字用
                // intern() 驻留，相同内容即同一对象，等价于原版「同一个 type」。
                String typeA = parts[1].intern();
                String typeB = parts[3].intern();
                ScheduledTick<String> a =
                        makeTick(0, "NORMAL", 0, typeA, Integer.parseInt(parts[2]));
                ScheduledTick<String> b =
                        makeTick(0, "NORMAL", 0, typeB, Integer.parseInt(parts[4]));
                boolean same = ScheduledTick.UNIQUE_TICK_HASH.equals(a, b);
                result.addProperty("value", same ? "same" : "diff");
            }
            case "sort" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException("scheduled_tick: sort 需要 1 个参数: " + unit);
                }
                List<ScheduledTick<String>> ticks = new ArrayList<>();
                for (String item : parts[1].split(",", -1)) {
                    String[] fields = item.split("/", -1);
                    if (fields.length != 3) {
                        throw new IllegalArgumentException(
                                "scheduled_tick: 排序项应为 tick/priority/sub: " + item);
                    }
                    ticks.add(makeTick(
                            Long.parseLong(fields[0]),
                            fields[1],
                            Long.parseLong(fields[2]),
                            "t",
                            0));
                }
                ticks.sort(ScheduledTick.DRAIN_ORDER);

                StringBuilder order = new StringBuilder();
                for (ScheduledTick<String> tick : ticks) {
                    if (order.length() > 0) {
                        order.append(',');
                    }
                    order.append(tick.triggerTick())
                            .append('/')
                            .append(tick.priority().name())
                            .append('/')
                            .append(tick.subTickOrder());
                }
                result.addProperty("value", order.toString());
            }
            default ->
                    throw new IllegalArgumentException("scheduled_tick: 无法识别的单元: " + unit);
        }

        return result;
    }
}