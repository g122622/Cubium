#pragma once

#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本数据
 *
 * 包含从行为包加载的脚本源码和元信息
 */
struct ScriptData {
    std::string name;          // 脚本模块名或路径
    std::string source;        // 脚本源码
    std::string filePath;      // 脚本文件路径（用于错误报告）
    bool isModule = false;     // 是否为ES6模块
};

} // namespace mc::mod::bedrock::addon
