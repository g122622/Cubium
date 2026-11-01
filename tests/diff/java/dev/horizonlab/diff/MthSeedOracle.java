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
import net.minecraft.core.Vec3i;
import net.minecraft.util.Mth;

/**
 * {@code Mth.getSeed} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthSeedOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#getSeed(int, int, int)} 与 {@link Mth#getSeed(Vec3i)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>输出是**有符号 64 位整数**，按**字符串**给出（避免 JSON 数字精度问题）。
 *
 * <p>单元表必须与 probes/mth_seed.cpp 逐项一致。
 */
@SuppressWarnings("deprecation")
public final class MthSeedOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "seed|0|0|0",
                    "seed|1|0|0",
                    "seed|0|1|0",
                    "seed|0|0|1",
                    "seed|-1|0|0",
                    "seed|0|-1|0",
                    "seed|0|0|-1",
                    "seed|5|3|7",
                    "seed|7|3|5",
                    "seed|5|7|3",
                    "seed|5|3|7",
                    "seed|100|200|300",
                    "seed|300|200|100",
                    "seed|200|100|300",
                    "seed|2147483647|0|0",
                    "seed|-2147483648|0|0",
                    "seed|1000|0|0",
                    "seed|0|64|0",
                    "seed|128|-64|-256",
                    "seed|-16|70|32",
                    "vec|5|3|7",
                    "vec|0|0|0",
                    "vec|128|-64|-256");

    private MthSeedOracle() {}

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
                "usage: MthSeedOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 4) {
            throw new IllegalArgumentException("mth_seed: 单元需要 4 段: " + unit);
        }

        int x = Integer.parseInt(parts[1]);
        int y = Integer.parseInt(parts[2]);
        int z = Integer.parseInt(parts[3]);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "seed" -> out.addProperty("value", Long.toString(Mth.getSeed(x, y, z)));
            case "vec" -> out.addProperty("value",
                    Long.toString(Mth.getSeed(new Vec3i(x, y, z))));
            default -> throw new IllegalArgumentException("mth_seed: 无法识别的单元: " + unit);
        }

        return out;
    }
}