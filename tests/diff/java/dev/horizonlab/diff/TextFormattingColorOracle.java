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
import java.util.Locale;
import net.minecraft.ChatFormatting;

/**
 * {@code ChatFormatting} 颜色/码位/分类查询的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.TextFormattingColorOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ChatFormatting#getColor()} / {@link ChatFormatting#isColor()} /
 * {@link ChatFormatting#isFormat()} / {@link ChatFormatting#getChar()} /
 * {@link ChatFormatting#getByCode(char)}。
 *
 * <p>**本能力的探针对接 Cubium 的真实实现**（`text::getFormattingColor` 等），
 * 因此这里的比对是「原版 vs Cubium」，而非「原版 vs 手抄转写」。
 *
 * <p>输出约定：
 * <ul>
 *   <li>{@code color}：原版返回 {@code null}（格式码）时输出字面量 {@code null}；
 *       否则输出 8 位大写 ARGB（Alpha 恒为 {@code FF}）。</li>
 *   <li>{@code fromCode}：原版返回 {@code null}（未知字符）时输出字面量 {@code NONE}。</li>
 * </ul>
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/text_formatting_color.cpp 逐项一致。
 */
public final class TextFormattingColorOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "color|BLACK",
                    "color|DARK_BLUE",
                    "color|DARK_GREEN",
                    "color|DARK_AQUA",
                    "color|DARK_RED",
                    "color|DARK_PURPLE",
                    "color|GOLD",
                    "color|GRAY",
                    "color|DARK_GRAY",
                    "color|BLUE",
                    "color|GREEN",
                    "color|AQUA",
                    "color|RED",
                    "color|LIGHT_PURPLE",
                    "color|YELLOW",
                    "color|WHITE",
                    "color|OBFUSCATED",
                    "color|BOLD",
                    "color|STRIKETHROUGH",
                    "color|UNDERLINE",
                    "color|ITALIC",
                    "color|RESET",
                    "isColor|BLACK",
                    "isColor|WHITE",
                    "isColor|BOLD",
                    "isColor|RESET",
                    "isStyle|OBFUSCATED",
                    "isStyle|ITALIC",
                    "isStyle|RESET",
                    "isStyle|BLACK",
                    "code|BLACK",
                    "code|GREEN",
                    "code|WHITE",
                    "code|BOLD",
                    "code|RESET",
                    "fromCode|0",
                    "fromCode|a",
                    "fromCode|f",
                    "fromCode|k",
                    "fromCode|r",
                    "fromCode|A",
                    "fromCode|z",
                    "fromCode|q",
                    "count");

    private TextFormattingColorOracle() {}

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
        System.err.println("usage: TextFormattingColorOracle list <units.txt> "
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

    private static ChatFormatting formattingByName(String name) {
        return ChatFormatting.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1 && parts[0].equals("count")) {
            out.addProperty("value", String.valueOf(ChatFormatting.values().length));
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("text_formatting_color: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "color" -> {
                Integer rgb = formattingByName(parts[1]).getColor();
                // 原版 getColor() 对格式码返回 null；输出字面量 null 以便与 Cubium
                // 的白色哨兵值产生可见差异。
                out.addProperty("value", rgb == null
                        ? "null"
                        : String.format(Locale.ROOT, "%08X", 0xFF000000 | (rgb & 0xFFFFFF)));
            }
            case "isColor" -> out.addProperty("value",
                    String.valueOf(formattingByName(parts[1]).isColor()));
            case "isStyle" -> out.addProperty("value",
                    String.valueOf(formattingByName(parts[1]).isFormat()));
            case "code" -> out.addProperty("value",
                    String.valueOf(formattingByName(parts[1]).getChar()));
            case "fromCode" -> {
                ChatFormatting formatting = ChatFormatting.getByCode(
                        parts[1].isEmpty() ? '\0' : parts[1].charAt(0));
                out.addProperty("value", formatting == null ? "NONE" : formatting.name());
            }
            default -> throw new IllegalArgumentException(
                    "text_formatting_color: 无法识别的单元: " + unit);
        }

        return out;
    }
}