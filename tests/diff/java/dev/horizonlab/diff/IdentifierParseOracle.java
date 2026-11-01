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
import net.minecraft.resources.Identifier;

/**
 * {@code Identifier} 解析与校验的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.IdentifierParseOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Identifier#isValidPath(String)} /
 * {@link Identifier#isValidNamespace(String)} / {@link Identifier#validPathChar(char)} /
 * {@link Identifier#parse(String)} / {@link Identifier#tryParse(String)} /
 * {@link Identifier#tryBuild(String, String)} /
 * {@link Identifier#bySeparator(String, char)} / {@link Identifier#withDefaultNamespace(String)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>非法输入统一输出字符串 `"&lt;invalid&gt;"`——**不抛异常**，避免整个 oracle 进程失败。
 *
 * <p>单元表必须与 probes/identifier_parse.cpp 逐项一致。
 */
public final class IdentifierParseOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 非法输入的输出标记。 */
    private static final String INVALID = "<invalid>";

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "path|foo/bar",
                    "path|foo/bar.txt",
                    "path|Foo",
                    "path|foo bar",
                    "path|",
                    "ns|foo",
                    "ns|foo/bar",
                    "ns|Foo",
                    "ns|",
                    "pathChar|97",
                    "pathChar|65",
                    "pathChar|47",
                    "pathChar|58",
                    "pathChar|95",
                    "parse|minecraft:stone",
                    "parse|foo:bar/baz",
                    "parse|foo",
                    "parse|:foo",
                    "parse|foo:",
                    "parse|Foo:bar",
                    "parse|foo/bar:baz",
                    "parse|foo:bar:baz",
                    "tryParse|minecraft:stone",
                    "tryParse|Foo:bar",
                    "tryParse|foo:bar:baz",
                    "tryBuild|foo|bar",
                    "tryBuild|Foo|bar",
                    "tryBuild|foo|Bar",
                    "bySep|foo.bar|.",
                    "bySep|foo|.",
                    "bySep|.bar|.",
                    "withDefault|foo/bar",
                    "withDefault|Foo");

    private IdentifierParseOracle() {}

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
        System.err.println("usage: IdentifierParseOracle list <units.txt> "
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

    /** 格式化标识符；`null` 输出 `<invalid>`。 */
    private static String render(Identifier identifier) {
        return identifier == null ? INVALID : identifier.toString();
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("identifier_parse: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "path" -> out.addProperty("value", Identifier.isValidPath(parts[1]));
            case "ns" -> out.addProperty("value", Identifier.isValidNamespace(parts[1]));
            case "pathChar" -> {
                int codepoint = Integer.parseInt(parts[1]);
                boolean valid = codepoint >= 0 && codepoint < 128
                        && Identifier.validPathChar((char) codepoint);
                out.addProperty("value", valid);
            }
            case "parse" -> {
                try {
                    out.addProperty("value", render(Identifier.parse(parts[1])));
                } catch (RuntimeException exception) {
                    out.addProperty("value", INVALID);
                }
            }
            case "tryParse" -> out.addProperty("value", render(Identifier.tryParse(parts[1])));
            case "tryBuild" -> out.addProperty("value",
                    render(Identifier.tryBuild(parts[1], parts[2])));
            case "bySep" -> {
                try {
                    out.addProperty("value",
                            render(Identifier.bySeparator(parts[1], parts[2].charAt(0))));
                } catch (RuntimeException exception) {
                    out.addProperty("value", INVALID);
                }
            }
            case "withDefault" -> {
                try {
                    out.addProperty("value", render(Identifier.withDefaultNamespace(parts[1])));
                } catch (RuntimeException exception) {
                    out.addProperty("value", INVALID);
                }
            }
            default -> throw new IllegalArgumentException(
                    "identifier_parse: 无法识别的单元: " + unit);
        }

        return out;
    }
}