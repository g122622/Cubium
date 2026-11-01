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
 * {@code ChatFormatting} 名字双向映射的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.TextFormattingNameOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ChatFormatting#getName()} / {@link ChatFormatting#getNames(boolean,
 * boolean)}。
 *
 * <p>**本能力的探针对接 Cubium 的真实实现**（`text::toName` / `text::fromName`），
 * 因此这里的比对是「原版 vs Cubium」。
 *
 * <p>{@code roundTrip} 是**往返自检**（`fromName(toName(x)) == x`），原版没有 `fromName`
 * 入口，因此该单元在 oracle 侧恒定输出 {@code true}（作为基准），实际校验的是 Cubium 侧
 * 的正向/反向映射能否互相还原。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/text_formatting_name.cpp 逐项一致。
 */
public final class TextFormattingNameOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "name|BLACK",
                    "name|DARK_BLUE",
                    "name|DARK_GREEN",
                    "name|DARK_AQUA",
                    "name|DARK_RED",
                    "name|DARK_PURPLE",
                    "name|GOLD",
                    "name|GRAY",
                    "name|DARK_GRAY",
                    "name|BLUE",
                    "name|GREEN",
                    "name|AQUA",
                    "name|RED",
                    "name|LIGHT_PURPLE",
                    "name|YELLOW",
                    "name|WHITE",
                    "name|OBFUSCATED",
                    "name|BOLD",
                    "name|STRIKETHROUGH",
                    "name|UNDERLINE",
                    "name|ITALIC",
                    "name|RESET",
                    "roundTrip|BLACK",
                    "roundTrip|DARK_BLUE",
                    "roundTrip|LIGHT_PURPLE",
                    "roundTrip|BOLD",
                    "roundTrip|RESET",
                    "fromName|dark_blue",
                    "fromName|DARK_BLUE",
                    "fromName|Dark_Blue",
                    "fromName|bold",
                    "fromName|reset",
                    "fromName|darkBlue",
                    "fromName|not_a_color",
                    "fromName|",
                    "names",
                    "count");

    private TextFormattingNameOracle() {}

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
        System.err.println("usage: TextFormattingNameOracle list <units.txt> "
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

    /**
     * 原版没有 {@code fromName} 入口，这里按「小写名精确匹配」模拟——
     * 这正是 {@code getName()} 的逆运算。未命中返回 {@code null}。
     */
    private static ChatFormatting fromNameLenient(String name) {
        String lowered = name.toLowerCase(Locale.ROOT);
        for (ChatFormatting formatting : ChatFormatting.values()) {
            if (formatting.getName().equals(lowered)) {
                return formatting;
            }
        }
        return null;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1) {
            switch (parts[0]) {
                case "names" -> {
                    StringBuilder names = new StringBuilder();
                    for (String name : ChatFormatting.getNames(true, true)) {
                        if (names.length() > 0) {
                            names.append(',');
                        }
                        names.append(name);
                    }
                    out.addProperty("value", names.toString());
                }
                case "count" -> out.addProperty("value",
                        String.valueOf(ChatFormatting.values().length));
                default -> throw new IllegalArgumentException(
                        "text_formatting_name: 无法识别的单元: " + unit);
            }
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("text_formatting_name: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "name" -> out.addProperty("value",
                    formattingByName(parts[1]).getName());
            case "roundTrip" -> {
                // 原版基准：getName() 是小写名，按小写名反查必然命中自身。
                ChatFormatting forward = formattingByName(parts[1]);
                ChatFormatting back = fromNameLenient(forward.getName());
                out.addProperty("value", String.valueOf(back == forward));
            }
            case "fromName" -> {
                ChatFormatting formatting = fromNameLenient(parts[1]);
                out.addProperty("value", formatting == null ? "NONE" : formatting.name());
            }
            default -> throw new IllegalArgumentException(
                    "text_formatting_name: 无法识别的单元: " + unit);
        }

        return out;
    }
}