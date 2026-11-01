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
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.contents.PlainTextContents;

/**
 * {@code PlainTextContents} 构造与 {@code toString} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.PlainTextContentsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link PlainTextContents#create(String)} /
 * {@link PlainTextContents#text()} / {@link Component#literal(String)}。
 *
 * <p>本能力不需要注册表，也不需要 bootstrap——{@code PlainTextContents} 是纯数据类。
 *
 * <p>单元表必须与 probes/plain_text_contents.cpp 逐项一致。
 */
public final class PlainTextContentsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "create|",
                    "create|abc",
                    "create| ",
                    "create|a b",
                    "create|{}",
                    "create|literal{x}",
                    "text|",
                    "text|abc",
                    "identity|",
                    "identity|abc",
                    "eq||",
                    "eq|abc|abc",
                    "eq|abc|abd",
                    "eq||abc",
                    "component|",
                    "component|abc",
                    "string|",
                    "string|abc");

    private PlainTextContentsOracle() {}

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
        System.err.println("usage: PlainTextContentsOracle list <units.txt> "
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
        if (parts.length < 2) {
            throw new IllegalArgumentException("plain_text_contents: 单元段数不符: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "create" -> out.addProperty("value",
                    PlainTextContents.create(parts[1]).toString());
            case "text" -> out.addProperty("value", PlainTextContents.create(parts[1]).text());
            case "identity" -> out.addProperty("value",
                    String.valueOf(PlainTextContents.create(parts[1])
                            == PlainTextContents.create(parts[1])));
            case "eq" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException(
                            "plain_text_contents: eq 需要 3 段: " + unit);
                }
                out.addProperty("value",
                        String.valueOf(PlainTextContents.create(parts[1]).equals(
                                PlainTextContents.create(parts[2]))));
            }
            case "component" -> out.addProperty("value",
                    Component.literal(parts[1]).getContents().toString());
            case "string" -> out.addProperty("value", Component.literal(parts[1]).getString());
            default -> throw new IllegalArgumentException(
                    "plain_text_contents: 无法识别的单元: " + unit);
        }

        return out;
    }
}