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
import net.minecraft.util.StringUtil;

/**
 * {@code StringUtil} 字符串工具的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.StringUtilOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link StringUtil#formatTickDuration(int, float)} /
 * {@link StringUtil#stripColor(String)} /
 * {@link StringUtil#truncateStringIfNecessary(String, int, boolean)} /
 * {@link StringUtil#lineCount(String)} / {@link StringUtil#trimChatMessage(String)} /
 * {@link StringUtil#isAllowedChatCharacter(int)} / {@link StringUtil#isValidPlayerName(String)} /
 * {@link StringUtil#filterText(String, boolean)} / {@link StringUtil#isWhitespace(int)} /
 * {@link StringUtil#isBlank(String)}。
 *
 * <p>本能力不需要注册表——`StringUtil` 全部是静态纯函数。
 *
 * <p>单元表必须与 probes/string_util.cpp 逐项一致。
 */
public final class StringUtilOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /**
     * 单元里表示「反斜杠 + 小写 u」两字符的转义前缀。
     *
     * <p>不能在源码里直接写反斜杠 u 的转义序列——Java 的词法分析会在处理字符串和注释之前就把它
     * 当作 Unicode 转义转换掉。因此用常量拼接。
     */
    private static final String UNICODE_ESC = "\\" + "u";

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "duration|0|20",
                    "duration|119|20",
                    "duration|1200|20",
                    "duration|3600|20",
                    "duration|72000|20",
                    "strip|" + UNICODE_ESC + "00A7ahello",
                    "strip|" + UNICODE_ESC + "00A7a" + UNICODE_ESC + "00A7lhello",
                    "strip|" + UNICODE_ESC + "00A7zhello",
                    "strip|hello",
                    "strip|" + UNICODE_ESC + "00A7A" + UNICODE_ESC + "00A7B" + UNICODE_ESC + "00A7C",
                    "trunc|abcdefgh|5|true",
                    "trunc|abcdefgh|4|true",
                    "trunc|abcdefgh|3|true",
                    "trunc|abcdefgh|5|false",
                    "trunc|abc|5|true",
                    "lines|",
                    "lines|abc",
                    "lines|a\\nb",
                    "lines|a\\r\\nb",
                    "lines|a\\vb",
                    "trimChat|hello",
                    "allowedChat|65",
                    "allowedChat|31",
                    "allowedChat|127",
                    "allowedChat|167",
                    "playerName|Steve",
                    "playerName|Steve123",
                    "playerName|abcdefghijklmnopq",
                    "playerName|abc def",
                    "playerName|abc" + UNICODE_ESC + "00A7def",
                    "filter|ab" + UNICODE_ESC + "0001cd|false",
                    "filter|ab\\ncd|false",
                    "filter|ab\\ncd|true",
                    "whitespace|32",
                    "whitespace|9",
                    "whitespace|65",
                    "blank|   ",
                    "blank|",
                    "blank|abc");

    private StringUtilOracle() {}

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
                "usage: StringUtilOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 把单元里的转义序列（换行、回车、垂直制表、制表、Unicode 十六进制）解码为真实字符。 */
    private static String decodeEscapes(String text) {
        StringBuilder out = new StringBuilder(text.length());
        for (int i = 0; i < text.length(); i++) {
            char ch = text.charAt(i);
            if (ch == '\\' && i + 1 < text.length()) {
                char next = text.charAt(i + 1);
                switch (next) {
                    case 'n' -> {
                        out.append('\n');
                        i++;
                        continue;
                    }
                    case 'r' -> {
                        out.append('\r');
                        i++;
                        continue;
                    }
                    case 'v' -> {
                        out.append('\u000B');
                        i++;
                        continue;
                    }
                    case 't' -> {
                        out.append('\t');
                        i++;
                        continue;
                    }
                    case 'u', 'U' -> {
                        if (i + 6 <= text.length()) {
                            out.append((char) Integer.parseInt(text.substring(i + 2, i + 6), 16));
                            i += 5;
                            continue;
                        }
                        break;
                    }
                    default -> {
                        break;
                    }
                }
            }
            out.append(ch);
        }
        return out.toString();
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("string_util: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "duration" -> out.addProperty("value", StringUtil.formatTickDuration(
                    Integer.parseInt(parts[1]), (float) Double.parseDouble(parts[2])));
            case "strip" -> out.addProperty("value", StringUtil.stripColor(decodeEscapes(parts[1])));
            case "trunc" -> out.addProperty("value", StringUtil.truncateStringIfNecessary(
                    decodeEscapes(parts[1]), Integer.parseInt(parts[2]),
                    Boolean.parseBoolean(parts[3])));
            case "lines" -> out.addProperty("value", StringUtil.lineCount(decodeEscapes(parts[1])));
            case "trimChat" -> out.addProperty("value",
                    StringUtil.trimChatMessage(decodeEscapes(parts[1])));
            case "allowedChat" -> out.addProperty("value",
                    StringUtil.isAllowedChatCharacter(Integer.parseInt(parts[1])));
            case "playerName" -> out.addProperty("value",
                    StringUtil.isValidPlayerName(decodeEscapes(parts[1])));
            case "filter" -> out.addProperty("value", StringUtil.filterText(
                    decodeEscapes(parts[1]), Boolean.parseBoolean(parts[2])));
            case "whitespace" -> out.addProperty("value",
                    StringUtil.isWhitespace(Integer.parseInt(parts[1])));
            case "blank" -> out.addProperty("value", StringUtil.isBlank(decodeEscapes(parts[1])));
            default -> throw new IllegalArgumentException(
                    "string_util: 无法识别的单元: " + unit);
        }

        return out;
    }
}