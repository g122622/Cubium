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
 * {@code Identifier} 三个构造入口与 {@code toString} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.IdentifierResolveOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Identifier#parse(String)} / {@link Identifier#tryParse(String)} /
 * {@link Identifier#withDefaultNamespace(String)} / {@link Identifier#toString()}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/identifier_resolve.cpp 逐项一致。
 */
public final class IdentifierResolveOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "parse|stone",
                    "tryParse|stone",
                    "default|stone",
                    "parse|:stone",
                    "tryParse|:stone",
                    "default|:stone",
                    "parse|foo:bar",
                    "tryParse|foo:bar",
                    "default|foo:bar",
                    "parse|a:b:c",
                    "tryParse|a:b:c",
                    "default|a:b:c",
                    "parse|Stone",
                    "tryParse|Stone",
                    "default|Stone",
                    "parse|foo:Bar",
                    "parse|",
                    "tryParse|",
                    "default|",
                    "parse|minecraft:block/stone",
                    "tryParse|block/stone",
                    "default|block/stone",
                    "parse|a/b:stone",
                    "tryParse|a/b:stone",
                    "tryParse|minecraft:a:b",
                    "toString|minecraft|stone",
                    "toString|foo|block/bar");

    private IdentifierResolveOracle() {}

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
        System.err.println("usage: IdentifierResolveOracle list <units.txt> "
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
            throw new IllegalArgumentException("identifier_resolve: 单元段数不符: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String value;
        switch (parts[0]) {
            case "toString" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException(
                            "identifier_resolve: toString 需要 3 段: " + unit);
                }
                // Identifier 的构造器是私有的，公开入口只有 parse / tryParse /
                // withDefaultNamespace。这里用 parse("ns:path") 构造——只用于验证
                // toString 的拼接形式（分隔符是 ':'，无其他修饰）。
                value = Identifier.parse(parts[1] + ":" + parts[2]).toString();
            }
            case "parse" -> {
                try {
                    value = Identifier.parse(parts[1]).toString();
                } catch (RuntimeException exception) {
                    value = "!IdentifierException";
                }
            }
            case "tryParse" -> {
                Identifier parsed = Identifier.tryParse(parts[1]);
                value = parsed == null ? "null" : parsed.toString();
            }
            case "default" -> {
                try {
                    value = Identifier.withDefaultNamespace(parts[1]).toString();
                } catch (RuntimeException exception) {
                    value = "!IdentifierException";
                }
            }
            default -> throw new IllegalArgumentException(
                    "identifier_resolve: 无法识别的单元: " + unit);
        }

        out.addProperty("value", value);
        return out;
    }
}