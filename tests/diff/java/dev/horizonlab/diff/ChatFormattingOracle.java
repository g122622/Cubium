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
import net.minecraft.ChatFormatting;

/**
 * {@code ChatFormatting} 格式码解析的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ChatFormattingOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ChatFormatting#stripFormatting(String)} /
 * {@link ChatFormatting#getByCode(char)} / {@link ChatFormatting#getById(int)} /
 * {@link ChatFormatting#getByName(String)}，以及实例方法 `isColor` / `isFormat` / `getId` /
 * `toString`。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/chat_formatting.cpp 逐项一致。
 */
public final class ChatFormattingOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** `§` 前缀码。 */
    private static final String PREFIX = "\u00a7";

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "strip|" + PREFIX + "ahello",
                    "strip|" + PREFIX + "Ahello",
                    "strip|" + PREFIX + "fhello",
                    "strip|" + PREFIX + "0hello",
                    "strip|" + PREFIX + "9hello",
                    "strip|" + PREFIX + "lhello",
                    "strip|" + PREFIX + "Lhello",
                    "strip|" + PREFIX + "khello",
                    "strip|" + PREFIX + "ohello",
                    "strip|" + PREFIX + "rhello",
                    "strip|" + PREFIX + "ghello",
                    "strip|" + PREFIX + "zhello",
                    "strip|" + PREFIX + " hello",
                    "strip|" + PREFIX,
                    "strip|" + PREFIX + "a" + PREFIX + "lBold" + PREFIX + "rPlain",
                    "strip|no codes here",
                    "byCode|a",
                    "byCode|A",
                    "byCode|0",
                    "byCode|f",
                    "byCode|r",
                    "byCode|g",
                    "byCode|1",
                    "byId|0",
                    "byId|15",
                    "byId|16",
                    "byId|-1",
                    "byId|-2",
                    "byId|100",
                    "byName|black",
                    "byName|BLACK",
                    "byName|dark_blue",
                    "byName|Dark-Blue",
                    "byName|reset",
                    "byName|not_a_color",
                    "info|BLACK",
                    "info|WHITE",
                    "info|BOLD",
                    "info|RESET");

    private ChatFormattingOracle() {}

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
                "usage: ChatFormattingOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static String nameOf(ChatFormatting formatting) {
        return formatting == null ? "null" : formatting.name();
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("chat_formatting: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "strip" -> out.addProperty("value", ChatFormatting.stripFormatting(parts[1]));
            case "byCode" -> {
                if (parts[1].isEmpty()) {
                    throw new IllegalArgumentException("chat_formatting: byCode 需要非空字符");
                }
                out.addProperty("value", nameOf(ChatFormatting.getByCode(parts[1].charAt(0))));
            }
            case "byId" -> out.addProperty(
                    "value", nameOf(ChatFormatting.getById(Integer.parseInt(parts[1]))));
            case "byName" -> out.addProperty(
                    "value", nameOf(ChatFormatting.getByName(parts[1])));
            case "info" -> {
                ChatFormatting formatting = matchByName(parts[1]);
                if (formatting == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value",
                            formatting.getChar() + "," + formatting.getId() + ","
                                    + (formatting.isColor() ? "true" : "false") + ","
                                    + (formatting.isFormat() ? "true" : "false") + ","
                                    + formatting.toString());
                }
            }
            default -> throw new IllegalArgumentException("chat_formatting: 无法识别的单元: " + unit);
        }

        return out;
    }

    /** 按枚举名精确匹配（大小写敏感，与探针一致）。 */
    private static ChatFormatting matchByName(String name) {
        for (ChatFormatting formatting : ChatFormatting.values()) {
            if (formatting.name().equals(name)) {
                return formatting;
            }
        }
        return null;
    }
}