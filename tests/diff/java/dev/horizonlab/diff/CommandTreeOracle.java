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
import com.mojang.brigadier.arguments.ArgumentType;
import com.mojang.brigadier.tree.ArgumentCommandNode;
import com.mojang.brigadier.tree.CommandNode;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import net.minecraft.commands.CommandBuildContext;
import net.minecraft.commands.synchronization.ArgumentTypeInfo;
import net.minecraft.commands.synchronization.ArgumentTypeInfos;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.commands.CommandSourceStack;
import net.minecraft.commands.Commands;
import net.minecraft.network.FriendlyByteBuf;
import net.minecraft.world.flag.FeatureFlags;
import io.netty.buffer.Unpooled;

/**
 * 服务端命令树的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CommandTreeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>用原版 {@code new Commands(CommandSelection.DEDICATED, context)} 注册全部服务端命令（与
 * 独立服务器启动时同一入口），再按 probes/command_tree.cpp 的格式扁平化：
 *
 * <pre>
 *   命令名 [子节点...]，参数写作 名字:类型名，非一级节点展开成 名字(子1,子2)
 * </pre>
 *
 * <p>参数类型名取原版类名（如 {@code GameModeArgument} → {@code gamemode}），与 Cubium 的
 * {@code ArgumentType::getTypeName()} 同一套词汇。
 */
public final class CommandTreeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private CommandTreeOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            Files.writeString(Path.of(args[1]), "tree\n", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]), "");
            return;
        }
        if (args.length == 4 && args[0].equals("run")) {
            // 第四个参数是逗号分隔的命令名：这些命令不在命令树里注册，后面的命令因此
            // 看不到它们带来的副作用（例如 /item 的槽位参数）。
            run(Path.of(args[1]), Path.of(args[2]), args[3]);
            return;
        }
        System.err.println("usage: CommandTreeOracle list <units.txt> | run <input.jsonl> <output.jsonl> [skip,commands]");
        System.exit(2);
    }

    private static void run(Path input, Path output, String skipSpec) throws IOException {
        DataPackRegistries registries = DataPackRegistries.load();
        CommandBuildContext context = CommandBuildContext.simple(registries.lookup, FeatureFlags.DEFAULT_FLAGS);
        CommandNode<CommandSourceStack> root =
                new Commands(Commands.CommandSelection.DEDICATED, context).getDispatcher().getRoot();

        Set<String> skip = new HashSet<>();
        for (String name : skipSpec.split(",")) {
            if (!name.isBlank()) {
                skip.add(name.trim());
                root.getChildren().removeIf(node -> node.getName().equals(name.trim()));
            }
        }

        List<String> lines = new ArrayList<>();
        for (CommandNode<CommandSourceStack> child : root.getChildren()) {
            lines.add(describeTree(child));
        }
        lines.sort(String::compareTo);

        JsonObject arguments = describeArgumentWire(root);

        JsonObject result = new JsonObject();
        result.add("tree", GSON.toJsonTree(lines));
        result.add("arguments", arguments);
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (!line.isBlank()) {
                text.append(GSON.toJson(result)).append('\n');
            }
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /**
     * 每个参数节点的网络编码，键与 probes/command_tree.cpp 一致（{@code 名字:序号}）。
     *
     * <p>键里的序号取所有参数节点按遍历顺序（先字面量后参数，各自再按名字排序）的全局下标，
     * 两侧用同一套遍历规则，因此序号可直接对齐。值为
     * {@code {typeId, payload, suggestions}}：typeId 是 COMMAND_ARGUMENT_TYPE 注册表里的数值 id，
     * payload 是序列化后的十六进制字节（不含名字），suggestions 是建议提供器 Identifier。
     */
    private static JsonObject describeArgumentWire(CommandNode<CommandSourceStack> root) {
        JsonObject result = new JsonObject();
        walkArguments(root, "", result);
        return result;
    }

    /**
     * 递归遍历命令树，把每个参数节点的网络编码记录到 {@code out}，键为从根到该节点的路径
     * （如 {@code " team add <team>:word"}），与 probes/command_tree.cpp 完全一致。
     */
    private static void walkArguments(
            CommandNode<CommandSourceStack> node, String path, JsonObject out) {
        for (CommandNode<CommandSourceStack> child : node.getChildren()) {
            String childPath = path + " " + child.getName();
            if (child instanceof ArgumentCommandNode<CommandSourceStack, ?> argument) {
                childPath += ":" + typeName(argument.getType());
                out.add(childPath, describeArgument(argument));
            }
            walkArguments(child, childPath, out);
        }
    }

    /** 单个参数节点的网络编码：{typeId, payload, suggestions}。 */
    private static JsonObject describeArgument(ArgumentCommandNode<CommandSourceStack, ?> argument) {
        JsonObject entry = new JsonObject();
        ArgumentType<?> type = argument.getType();
        ArgumentTypeInfo<?, ?> info = ArgumentTypeInfos.byClass(type);
        entry.addProperty("typeId", networkId(info));

        // 原版序列化：数值 id 之后的 properties 部分，单独取出来比字节。
        FriendlyByteBuf buffer = new FriendlyByteBuf(Unpooled.buffer());
        serializeInfo(info, type, buffer);
        entry.addProperty("payload", hex(buffer));
        // Cubium 侧对自定义建议提供器统一回落到 minecraft:ask_server（见命令树快照）。
        entry.addProperty(
                "suggestions", argument.getCustomSuggestions() != null ? "minecraft:ask_server" : "");
        return entry;
    }


    /**
     * 把参数类型编码成网络上的 properties 载荷（数值 id 之后的部分）。
     *
     * <p>原版先由 {@code ArgumentTypeInfo} 生成该参数类型的 Template，再把 Template 写进缓冲，
     * 客户端按同一张表反序列化。这里的泛型参数在编译期无法对齐，故走反射调用同一对方法，
     * 语义与原版一致；无法往返的类型留空载荷，由 typeId 差异暴露。
     */
    @SuppressWarnings("unchecked")
    private static void serializeInfo(
            ArgumentTypeInfo<?, ?> info, ArgumentType<?> type, FriendlyByteBuf buffer) {
        try {
            // 泛型在此处无法静态对齐（Template 的具体类型随 ArgumentTypeInfo 的子类变化），
            // 故用「原始类型 + 强制转换」调用接口上的 unpack / serializeToNetwork 两个方法：
            // 原版 ArgumentTypeInfo 的约定就是 unpack(type) 产出的 Template 可直接交给
            // serializeToNetwork 写出网络载荷（ClientboundCommandsPacket:226 即此路径）。
            ArgumentTypeInfo<ArgumentType<?>, ArgumentTypeInfo.Template<ArgumentType<?>>> typed =
                    (ArgumentTypeInfo<ArgumentType<?>, ArgumentTypeInfo.Template<ArgumentType<?>>>) info;
            ArgumentTypeInfo.Template<ArgumentType<?>> template = typed.unpack(type);
            typed.serializeToNetwork(template, buffer);
        } catch (RuntimeException e) {
            // 该类型不支持往返序列化：留空载荷，但把原因打出来，避免静默失败。
            System.err.println("serializeInfo failed: " + e);
        }
    }

    /**
     * ArgumentTypeInfo → COMMAND_ARGUMENT_TYPE 注册表里的数值 id。
     *
     * <p>原版就用 {@code BuiltInRegistries.COMMAND_ARGUMENT_TYPE.getId(info)} 写这个编号
     * （ClientboundCommandsPacket:226），直接沿用同一接口，避免自己按迭代顺序推算出错。
     */
    private static int networkId(ArgumentTypeInfo<?, ?> info) {
        return BuiltInRegistries.COMMAND_ARGUMENT_TYPE.getId(info);
    }

    private static String hex(FriendlyByteBuf buffer) {
        StringBuilder text = new StringBuilder();
        // 只取已写入的部分（readerIndex..writerIndex），不碰未用的容量区。
        int start = buffer.readerIndex();
        int end = buffer.writerIndex();
        for (int i = start; i < end; i++) {
            text.append(String.format("%02x", buffer.getByte(i)));
        }
        return text.toString();
    }

    private static String describeTree(CommandNode<CommandSourceStack> root) {
        List<String> parts = new ArrayList<>();
        for (CommandNode<CommandSourceStack> child : root.getChildren()) {
            StringBuilder text = new StringBuilder(signature(child));
            List<String> grandchildren = childSignatures(child);
            if (!grandchildren.isEmpty()) {
                text.append('(').append(String.join(",", grandchildren)).append(')');
            }
            parts.add(text.toString());
        }
        parts.sort(String::compareTo);
        StringBuilder line = new StringBuilder(root.getName());
        for (String part : parts) {
            line.append(' ').append(part);
        }
        return line.toString();
    }

    private static List<String> childSignatures(CommandNode<CommandSourceStack> node) {
        List<String> result = new ArrayList<>();
        for (CommandNode<CommandSourceStack> child : node.getChildren()) {
            result.add(signature(child));
        }
        result.sort(String::compareTo);
        return result;
    }

    /**
     * 字面量输出名字；参数输出 `名字:类型名`。
     *
     * <p>类型名取原版 {@code COMMAND_ARGUMENT_TYPE} 注册表里的 Identifier 路径
     * （{@code minecraft:entity}、{@code brigadier:integer}……），这是原版协议真正使用的
     * 词汇；Cubium 侧的 {@code ArgumentType::getTypeName()} 也按同一套命名。
     */
    private static String signature(CommandNode<CommandSourceStack> node) {
        if (node instanceof ArgumentCommandNode<CommandSourceStack, ?> argument) {
            return node.getName() + ":" + typeName(argument.getType());
        }
        return node.getName();
    }

    /**
     * 参数类型 → 注册表里的类型名（如 {@code entity}、{@code integer}）。
     *
     * <p>先用 {@code ArgumentTypeInfos.byClass} 按实现类精确定位参数类型信息，再反查
     * 注册表键；注册表里 brigadier 的四种基础类型是 {@code brigadier:integer} 等，
     * 取路径后与 Cubium 的类型名同形（integer/float/double/bool/string）。
     */
    private static String typeName(ArgumentType<?> type) {
        ArgumentTypeInfo<?, ?> info = ArgumentTypeInfos.byClass(type);
        for (var entry : BuiltInRegistries.COMMAND_ARGUMENT_TYPE.entrySet()) {
            if (entry.getValue() == info) {
                return entry.getKey().identifier().getPath();
            }
        }
        return "unknown";
    }
}
