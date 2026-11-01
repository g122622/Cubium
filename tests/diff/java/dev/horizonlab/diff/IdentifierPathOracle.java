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
 * {@code Identifier} 字符串派生方法的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.IdentifierPathOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Identifier#toLanguageKey()} /
 * {@link Identifier#toShortLanguageKey()} / {@link Identifier#toShortString()} /
 * {@link Identifier#toDebugFileName()} / {@link Identifier#toLanguageKey(String)} /
 * {@link Identifier#withPrefix(String)} / {@link Identifier#withSuffix(String)} /
 * {@link Identifier#withPath(String)}。
 *
 * <p>构造失败统一输出字符串 `"&lt;invalid&gt;"`——**不抛异常**。
 *
 * <p>单元表必须与 probes/identifier_path.cpp 逐项一致。
 */
public final class IdentifierPathOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 非法输入的输出标记。 */
    private static final String INVALID = "<invalid>";

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "lang|minecraft|block/stone",
                    "lang|foo|bar/baz",
                    "lang|minecraft|stone",
                    "shortLang|minecraft|block/stone",
                    "shortLang|foo|bar/baz",
                    "shortStr|minecraft|block/stone",
                    "shortStr|foo|bar/baz",
                    "debugFile|minecraft|block/stone",
                    "debugFile|foo|bar/baz.txt",
                    "langPrefix|minecraft|stone|block",
                    "langPrefix|foo|bar|item",
                    "withPrefix|minecraft|stone|block/",
                    "withPrefix|minecraft|stone|Block/",
                    "withSuffix|minecraft|stone|_top",
                    "withSuffix|minecraft|stone|_TOP",
                    "withPath|minecraft|stone|block/stone",
                    "withPath|minecraft|stone|Stone");

    private IdentifierPathOracle() {}

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
                "usage: IdentifierPathOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 把可能失败的操作包装成字符串输出。 */
    private static String render(java.util.function.Supplier<Identifier> supplier) {
        try {
            Identifier identifier = supplier.get();
            return identifier == null ? INVALID : identifier.toString();
        } catch (RuntimeException exception) {
            return INVALID;
        }
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 3) {
            throw new IllegalArgumentException("identifier_path: 单元至少三段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String nameSpace = parts[1];
        String path = parts[2];

        switch (parts[0]) {
            case "lang" -> out.addProperty("value", Identifier.fromNamespaceAndPath(
                    nameSpace, path).toLanguageKey());
            case "shortLang" -> out.addProperty("value", Identifier.fromNamespaceAndPath(
                    nameSpace, path).toShortLanguageKey());
            case "shortStr" -> out.addProperty("value", Identifier.fromNamespaceAndPath(
                    nameSpace, path).toShortString());
            case "debugFile" -> out.addProperty("value", Identifier.fromNamespaceAndPath(
                    nameSpace, path).toDebugFileName());
            case "langPrefix" -> out.addProperty("value", Identifier.fromNamespaceAndPath(
                    nameSpace, path).toLanguageKey(parts[3]));
            case "withPrefix" -> out.addProperty("value", render(() ->
                    Identifier.fromNamespaceAndPath(nameSpace, path).withPrefix(parts[3])));
            case "withSuffix" -> out.addProperty("value", render(() ->
                    Identifier.fromNamespaceAndPath(nameSpace, path).withSuffix(parts[3])));
            case "withPath" -> out.addProperty("value", render(() ->
                    Identifier.fromNamespaceAndPath(nameSpace, path).withPath(parts[3])));
            default -> throw new IllegalArgumentException(
                    "identifier_path: 无法识别的单元: " + unit);
        }

        return out;
    }
}