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
import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.arguments.BoolArgumentType;
import com.mojang.brigadier.arguments.DoubleArgumentType;
import com.mojang.brigadier.arguments.IntegerArgumentType;
import com.mojang.brigadier.arguments.StringArgumentType;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.commands.arguments.GameModeArgument;
import net.minecraft.commands.arguments.NbtPathArgument;
import net.minecraft.commands.arguments.SlotArgument;
import net.minecraft.commands.arguments.coordinates.WorldCoordinate;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.GameType;

/**
 * 命令参数解析的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CommandArgumentOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元形如 {@code gamemode\tnope}（参数类型 + 制表符 + 输入），清单与
 * probes/command_argument.cpp 逐条一致——两侧必须解析同一批输入，否则比的是清单而不是解析器。
 *
 * <p>输出只有「接受/拒绝 + 值 + 出错游标」，不含错误文案：原版报错是翻译键拼装出来的
 * 面向玩家的文字，文案不同不影响行为；游标不同则说明两侧读到同一输入的不同位置。
 */
public final class CommandArgumentOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private CommandArgumentOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            Files.writeString(Path.of(args[1]), String.join("\n", units()) + "\n", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: CommandArgumentOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    /**
     * 清单。<b>必须与 probes/command_argument.cpp 的 cases() 逐条相同</b>——探针用自己的清单，
     * 两边各写一份是为了让「清单本身」也受版本控制，改动时两边同时改。
     */
    private static final String[][] CASES = {
        // 整数
        {"int", "0"}, {"int", "1"}, {"int", "-1"}, {"int", "+1"}, {"int", "01"}, {"int", "2147483647"},
        {"int", "2147483648"}, {"int", "-2147483648"}, {"int", "-2147483649"}, {"int", "1.0"}, {"int", ""},
        {"int", " 1"}, {"int", "1a"}, {"int", "١"},
        // 双精度（坐标语法）
        {"double", "0"}, {"double", "1.5"}, {"double", "-1.5"}, {"double", ".5"}, {"double", "1."}, {"double", "1e3"},
        {"double", "1E3"}, {"double", "1e"}, {"double", "NaN"}, {"double", "Infinity"}, {"double", "-0.0"},
        {"double", ""}, {"double", "1.5.5"}, {"double", "0x10"}, {"double", "1d"}, {"double", "-"}, {"double", "+1"},
        {"double", " 1"},
        // 布尔
        {"bool", "true"}, {"bool", "false"}, {"bool", "1"}, {"bool", "0"}, {"bool", "True"}, {"bool", ""},
        {"bool", "truex"},
        // 字符串
        {"string", "hello"}, {"string", "\"quoted text\""}, {"string", "hello world"}, {"string", ""},
        {"string", "\"unterminated"}, {"string", "a\"b c\"d"},
        {"word", "hello"}, {"word", "hello world"}, {"word", "\"quoted\""}, {"word", ""}, {"word", "a_b-c.d"},
        {"greedy", "hello"}, {"greedy", "  padded "}, {"greedy", ""}, {"greedy", "\"unterminated"},
        // 资源位置
        {"resource_location", "stone"}, {"resource_location", "minecraft:stone"}, {"resource_location", "mine craft"},
        {"resource_location", "Minecraft:Stone"}, {"resource_location", "a:b:c"}, {"resource_location", ""},
        {"resource_location", ":x"}, {"resource_location", "x:"}, {"resource_location", "a_b-c.d"},
        {"resource_location", "\"quoted\""}, {"resource_location", "minecraft:/leading_slash"},
        {"resource_location", "minecraft:a b"},
        // 游戏模式
        {"gamemode", "survival"}, {"gamemode", "creative"}, {"gamemode", "adventure"}, {"gamemode", "spectator"},
        {"gamemode", "s"}, {"gamemode", "c"}, {"gamemode", "a"}, {"gamemode", "sp"}, {"gamemode", "Survival"},
        {"gamemode", "nope"}, {"gamemode", ""}, {"gamemode", "survivalx"}, {"gamemode", "0"}, {"gamemode", "1"},
        // 时间
        {"time", "0"}, {"time", "100"}, {"time", "100t"}, {"time", "5s"}, {"time", "1d"}, {"time", "1.5d"},
        {"time", "0.1s"}, {"time", "-1"}, {"time", "1.5t"}, {"time", "1.5s"}, {"time", "01"}, {"time", "+1"},
        {"time", "1D"}, {"time", "1S"}, {"time", "1m"}, {"time", ""}, {"time", " 5s"}, {"time", "5 s"},
        {"time", "999999999"}, {"time", "2147483647"}, {"time", "1.9s"},
        // 槽位
        {"slot", "weapon"}, {"slot", "weapon.mainhand"}, {"slot", "weapon.offhand"},
        {"slot", "armor.head"}, {"slot", "armor.chest"}, {"slot", "armor.legs"}, {"slot", "armor.feet"},
        {"slot", "armor.body"}, {"slot", "container.0"}, {"slot", "container.53"}, {"slot", "container.54"},
        {"slot", "hotbar.0"}, {"slot", "hotbar.8"}, {"slot", "hotbar.9"}, {"slot", "hotbar.17"},
        {"slot", "inventory.0"}, {"slot", "inventory.26"}, {"slot", "inventory.27"}, {"slot", "inventory.007"},
        {"slot", "enderchest.0"}, {"slot", "enderchest.26"}, {"slot", "enderchest.27"}, {"slot", "villager.0"},
        {"slot", "villager.7"}, {"slot", "villager.8"}, {"slot", "horse.0"}, {"slot", "horse.14"},
        {"slot", "horse.chest"}, {"slot", "player.crafting.0"}, {"slot", "player.cursor"}, {"slot", "saddle"},
        {"slot", "weapon.mainhandx"}, {"slot", ""}, {"slot", "-1"}, {"slot", "contents"}, {"slot", "mob.0"},
        {"slot", "0"},
        // NBT 路径
        {"nbt_path", "a"}, {"nbt_path", "a.b"}, {"nbt_path", "a.b[0]"}, {"nbt_path", "a[0].b"}, {"nbt_path", ""},
        {"nbt_path", "a..b"}, {"nbt_path", "{x:1}.a"}, {"nbt_path", "a[-1]"}, {"nbt_path", "*"},
        {"nbt_path", "a:b"}, {"nbt_path", "Steve:Inventory"}, {"nbt_path", "a:b.c"},
        {"nbt_path", "{k:1}"}, {"nbt_path", "{k:1}.a[0]"},
        {"nbt_path", "a[]"}, {"nbt_path", "  a"},
        // 坐标读取的补充档位
        {"double", "~"}, {"double", "~5"}, {"double", "~-5"}, {"double", "^"}, {"double", "^1"}, {"double", "-0"},
        {"double", "007"}, {"double", "1.5e2"},
    };

    private static List<String> units() {
        List<String> units = new java.util.ArrayList<>(CASES.length);
        for (String[] c : CASES) {
            units.add(c[0] + '\t' + c[1]);
        }
        return units;
    }

    private static void run(Path input, Path output) throws IOException {
        try (BufferedReader reader = Files.newBufferedReader(input, StandardCharsets.UTF_8);
                BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.isBlank()) {
                    continue;
                }
                String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
                writer.write(GSON.toJson(summarise(unit)));
                writer.write('\n');
            }
        }
    }

    private static JsonObject summarise(String unit) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        int tab = unit.indexOf('\t');
        if (tab < 0) {
            result.addProperty("missing", true);
            return result;
        }
        String type = unit.substring(0, tab);
        String text = unit.substring(tab + 1);

        StringReader reader = new StringReader(text);
        try {
            switch (type) {
                case "int" -> result.addProperty("value", IntegerArgumentType.integer().parse(reader));
                case "double" -> result.addProperty("value", doubleBits(DoubleArgumentType.doubleArg().parse(reader)));
                case "bool" -> result.addProperty("value", BoolArgumentType.bool().parse(reader));
                case "string" -> result.addProperty("value", StringArgumentType.string().parse(reader));
                case "word" -> result.addProperty("value", StringArgumentType.word().parse(reader));
                case "greedy" -> result.addProperty("value", StringArgumentType.greedyString().parse(reader));
                case "resource_location" -> result.addProperty("value", Identifier.read(reader).toString());
                case "gamemode" -> result.addProperty("value", gamemode(reader));
                case "time" -> result.addProperty("value", time(reader));
                case "slot" -> result.addProperty("value", SlotArgument.slot().parse(reader));
                case "nbt_path" -> nbtPath(result, reader);
                default -> result.addProperty("unsupported", type);
            }
            result.addProperty("ok", true);
        } catch (CommandSyntaxException e) {
            result.addProperty("ok", false);
            result.addProperty("cursor", e.getCursor());
        }
        return result;
    }

    /** 浮点按位输出，与 probe 的 f64Bits 同格式，避免两侧 toString 掩盖末位差异。 */
    private static String doubleBits(double value) {
        if (Double.isNaN(value)) {
            return "nan";
        }
        return String.format("0x%016x", Double.doubleToRawLongBits(value));
    }

    /**
     * 原版 NbtPathArgument：只有解析出了节点时才 toString（原始文本）；空输入在原版里
     * 返回一个没有节点的路径。两侧都比对「往返后的文本」。
     */
    private static void nbtPath(JsonObject result, StringReader reader) throws CommandSyntaxException {
        NbtPathArgument.NbtPath path = NbtPathArgument.nbtPath().parse(reader);
        result.addProperty("value", path.toString());
    }

    /**
     * 原版 GameModeArgument：只认 "survival"/"creative"/"adventure"/"spectator" 全名，
     * 1.13 起的 s/c/a/sp 与数字简写全部已被移除。输出用 GameType 的序号（与原版
     * {@code GameType.getId()} 一致：survival=0, creative=1, adventure=2, spectator=3）。
     */
    private static int gamemode(StringReader reader) throws CommandSyntaxException {
        return modeId(GameModeArgument.gameMode().parse(reader));
    }

    private static int modeId(GameType type) {
        return switch (type) {
            case SURVIVAL -> 0;
            case CREATIVE -> 1;
            case ADVENTURE -> 2;
            case SPECTATOR -> 3;
        };
    }

    /**
     * 原版 TimeArgument.parse：坐标读取（数字 + 可选小数点 + 可选尾随字母）→ 读单位 →
     * 查 {@code UNITS}（""/"t"=1, "s"=20, "d"=24000）→ {@code (int) Math.round(value * multiplier)}，
     * 再检查最小值 0。原版 TimeArgument 的 int 铸造不查上下界，溢出按 Java 语义截断。
     */
    private static int time(StringReader reader) throws CommandSyntaxException {
        // 注意：前导空白时原版 WorldCoordinate.parseDouble 会解得 0 而不报错，
        // 与 TimeArgument 真实行为一致（` 5s` → 0 tick）。
        double value = WorldCoordinate.parseDouble(reader, false).value();
        String unit = reader.readUnquotedString();
        int multiplier;
        switch (unit) {
            case "", "t" -> multiplier = 1;
            case "s" -> multiplier = 20;
            case "d" -> multiplier = 24000;
            default -> throw CommandSyntaxException.BUILT_IN_EXCEPTIONS.readerInvalidInt().createWithContext(reader, unit);
        }
        return (int) Math.round(value * multiplier);
    }
}
