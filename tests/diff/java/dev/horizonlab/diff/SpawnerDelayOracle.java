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
 * 刷怪笼生成延迟状态机的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.SpawnerDelayOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>{@code BaseSpawner.delay} 与 {@code serverTick} 都是 private / 需要 {@code Level}
 * （取随机数、判断附近玩家、实际生成实体），且 {@code spawnDelay} 没有公开写访问器，
 * 因此这里按源码逐行复写延迟这一层，把随机数结果（`roll`）、玩家范围、开关都作为输入
 * 显式传入。
 *
 * <p>单元表必须与 probes/spawner_delay.cpp 逐项一致。
 */
public final class SpawnerDelayOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "delay|200|800|0",
                    "delay|200|800|599",
                    "delay|200|200|0",
                    "delay|300|200|0",
                    "delay|0|1|0",
                    "delay|0|10|0",
                    "tick|20|true|true|200|800|0",
                    "tick|1|true|true|200|800|0",
                    "tick|0|true|true|200|800|0",
                    "tick|-1|true|true|200|800|0",
                    "tick|-1|true|true|250|250|0",
                    "tick|20|false|true|200|800|0",
                    "tick|20|true|false|200|800|0",
                    "tick|-1|false|true|200|800|0",
                    "tick|0|false|true|200|800|0");

    private SpawnerDelayOracle() {}

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
                "usage: SpawnerDelayOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length == 0) {
            throw new IllegalArgumentException("spawner_delay: 空单元");
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (parts[0].equals("delay") && parts.length == 4) {
            int minSpawnDelay = Integer.parseInt(parts[1]);
            int maxSpawnDelay = Integer.parseInt(parts[2]);
            int roll = Integer.parseInt(parts[3]);

            result.addProperty("mode", "delay");
            result.addProperty("spawnDelay", computeDelay(minSpawnDelay, maxSpawnDelay, roll));
            result.addProperty("reset", false);
            result.addProperty("decremented", false);
            result.addProperty("shouldSpawn", false);
            return result;
        }

        if (parts[0].equals("tick") && parts.length == 7) {
            int spawnDelay = Integer.parseInt(parts[1]);
            boolean nearPlayer = Boolean.parseBoolean(parts[2]);
            boolean enabled = Boolean.parseBoolean(parts[3]);
            int minSpawnDelay = Integer.parseInt(parts[4]);
            int maxSpawnDelay = Integer.parseInt(parts[5]);
            int roll = Integer.parseInt(parts[6]);

            boolean reset = false;
            boolean decremented = false;
            boolean shouldSpawn = false;

            // 前置条件不满足：整个 tick 什么都不做。
            if (nearPlayer && enabled) {
                if (spawnDelay == -1) {
                    spawnDelay = computeDelay(minSpawnDelay, maxSpawnDelay, roll);
                    reset = true;
                }
                if (spawnDelay > 0) {
                    spawnDelay--;
                    decremented = true;
                } else {
                    shouldSpawn = true;
                }
            }

            result.addProperty("mode", "tick");
            result.addProperty("spawnDelay", spawnDelay);
            result.addProperty("reset", reset);
            result.addProperty("decremented", decremented);
            result.addProperty("shouldSpawn", shouldSpawn);
            return result;
        }

        throw new IllegalArgumentException("spawner_delay: 无法识别的单元: " + unit);
    }

    /** 复刻 {@code BaseSpawner.delay}：{@code roll} 对应 {@code random.nextInt(max - min)}。 */
    private static int computeDelay(int minSpawnDelay, int maxSpawnDelay, int roll) {
        if (maxSpawnDelay <= minSpawnDelay) {
            return minSpawnDelay;
        }
        return minSpawnDelay + roll;
    }
}