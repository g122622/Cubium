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
import net.minecraft.world.scores.criteria.ObjectiveCriteria;

/**
 * {@code ObjectiveCriteria.RenderType} 字符串映射的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RenderTypeStringsOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ObjectiveCriteria.RenderType#getSerializedName()} 与
 * {@link ObjectiveCriteria.RenderType#byId(String)}。
 *
 * <p><b>本能力的探针对接 Cubium 的真实实现</b>（{@code renderTypeToString} /
 * {@code renderTypeParse}）。
 *
 * <p>原版 {@code byId} 对未知输入返回 {@code null}——本 oracle 输出字面量 {@code null}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/render_type_strings.cpp 逐项一致。
 */
public final class RenderTypeStringsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "count",
                    "names",
                    "toString|INTEGER",
                    "toString|HEARTS",
                    "parse|integer",
                    "parse|hearts",
                    "parse|dunno",
                    "parse|",
                    "parse|INTEGER",
                    "parse|HEARTS",
                    "parse|health");

    private RenderTypeStringsOracle() {}

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
        System.err.println("usage: RenderTypeStringsOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
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

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1) {
            switch (parts[0]) {
                case "count" -> out.addProperty("value",
                        String.valueOf(ObjectiveCriteria.RenderType.values().length));
                case "names" -> {
                    StringBuilder names = new StringBuilder();
                    for (ObjectiveCriteria.RenderType type : ObjectiveCriteria.RenderType.values()) {
                        if (names.length() > 0) {
                            names.append(',');
                        }
                        names.append(type.getSerializedName());
                    }
                    out.addProperty("value", names.toString());
                }
                default -> throw new IllegalArgumentException(
                        "render_type_strings: 无法识别的单元: " + unit);
            }
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("render_type_strings: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "toString" -> out.addProperty("value",
                    ObjectiveCriteria.RenderType.valueOf(parts[1]).getSerializedName());
            case "parse" -> {
                ObjectiveCriteria.RenderType type = ObjectiveCriteria.RenderType.byId(parts[1]);
                out.addProperty("value", type == null ? "null" : type.name());
            }
            default -> throw new IllegalArgumentException(
                    "render_type_strings: 无法识别的单元: " + unit);
        }

        return out;
    }
}