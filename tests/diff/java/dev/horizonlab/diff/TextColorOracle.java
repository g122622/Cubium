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
import com.mojang.serialization.DataResult;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Locale;
import net.minecraft.network.chat.TextColor;

/**
 * {@code TextColor} 解析与序列化的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.TextColorOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link TextColor#parseColor(String)} /
 * {@link TextColor#fromRgb(int)} / {@link TextColor#serialize()}.
 *
 * <p>本能力不需要注册表。
 *
 * <p>输出格式：`&lt;value 6 位大写十六进制&gt;|&lt;serialize&gt;`，解析失败输出 `&lt;error&gt;`。
 *
 * <p>单元表必须与 probes/text_color.cpp 逐项一致。
 */
public final class TextColorOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "parse|#ff0000",
                    "parse|#FF0000",
                    "parse|#000000",
                    "parse|#ffffff",
                    "parse|#f00",
                    "parse|#0",
                    "parse|#1000000",
                    "parse|#",
                    "parse|#gggggg",
                    "parse|#-1",
                    "parse|#+ff",
                    "parse|red",
                    "parse|black",
                    "parse|white",
                    "parse|light_purple",
                    "parse|dark_gray",
                    "parse|RED",
                    "parse|bold",
                    "parse|reset",
                    "parse|",
                    "fromRgb|16711680",
                    "fromRgb|-1",
                    "fromRgb|16777216",
                    "fromRgb|4294967295",
                    "match|red|#ff5555",
                    "match|red|red",
                    "match|red|#ff0000");

    private TextColorOracle() {}

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
                "usage: TextColorOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 渲染为 `&lt;hex&gt;|&lt;serialize&gt;`；失败输出 `&lt;error&gt;`。 */
    private static String render(TextColor color) {
        if (color == null) {
            return "<error>";
        }
        return String.format(Locale.ROOT, "%06X", color.getValue()) + "|" + color.serialize();
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("text_color: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "parse" -> {
                DataResult<TextColor> result = TextColor.parseColor(parts[1]);
                out.addProperty("value", result.result().map(TextColorOracle::render)
                        .orElse("<error>"));
            }
            case "fromRgb" -> out.addProperty("value",
                    render(TextColor.fromRgb((int) Long.parseLong(parts[1]))));
            case "match" -> {
                TextColor a = TextColor.parseColor(parts[1]).result().orElse(null);
                TextColor b = TextColor.parseColor(parts[2]).result().orElse(null);
                out.addProperty("value", a != null && a.equals(b));
            }
            default -> throw new IllegalArgumentException(
                    "text_color: 无法识别的单元: " + unit);
        }

        return out;
    }
}