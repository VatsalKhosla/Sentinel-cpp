#include "compile_commands.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include <iostream>
#include <filesystem>

using namespace safecpp;

bool CompileCommandsParser::parse(const std::string& json_file) {
    // Read the JSON file
    auto buffer_or_error = llvm::MemoryBuffer::getFile(json_file);
    if (!buffer_or_error) {
        std::cerr << "Failed to read compile_commands.json: " << json_file << "\n";
        return false;
    }
    
    auto& buffer = buffer_or_error.get();
    llvm::StringRef json_str = buffer->getBuffer();
    
    // Parse JSON
    auto json_value = llvm::json::parse(json_str);
    if (!json_value) {
        std::cerr << "Failed to parse compile_commands.json\n";
        return false;
    }
    
    auto* array = json_value->getAsArray();
    if (!array) {
        std::cerr << "compile_commands.json root must be an array\n";
        return false;
    }
    
    return parseJSONArray(*array);
}

bool CompileCommandsParser::parseJSONArray(const llvm::json::Array& array) {
    for (const auto& value : array) {
        if (const auto* obj = value.getAsObject()) {
            CompileCommand cmd = parseCommand(*obj);
            if (!cmd.file.empty()) {
                commands_.push_back(cmd);
                file_to_command_[cmd.file] = &commands_.back();
            }
        }
    }
    return !commands_.empty();
}

CompileCommand CompileCommandsParser::parseCommand(const llvm::json::Object& obj) {
    CompileCommand cmd;
    
    // Extract required fields
    if (auto file = obj.getString("file")) {
        cmd.file = file->str();
    }
    if (auto dir = obj.getString("directory")) {
        cmd.directory = dir->str();
    }
    
    // Extract either "command" (string) or "arguments" (array)
    if (auto command = obj.getString("command")) {
        cmd.command = command->str();
        // Simple split on spaces (would need better parsing for quoted args)
        std::istringstream iss(cmd.command);
        std::string arg;
        while (iss >> arg) {
            cmd.arguments.push_back(arg);
        }
    } else if (auto args = obj.getArray("arguments")) {
        for (const auto& arg_val : *args) {
            if (auto arg = arg_val.getAsString()) {
                cmd.arguments.push_back(arg->str());
            }
        }
    }
    
    return cmd;
}

const CompileCommand* CompileCommandsParser::getCommandForFile(const std::string& file) const {
    auto it = file_to_command_.find(file);
    if (it != file_to_command_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> CompileCommandsParser::getSourceFiles() const {
    std::vector<std::string> files;
    for (const auto& cmd : commands_) {
        files.push_back(cmd.file);
    }
    return files;
}

std::vector<std::string> CompileCommandsParser::getIncludePaths() const {
    std::vector<std::string> includes;
    
    for (const auto& cmd : commands_) {
        for (size_t i = 0; i < cmd.arguments.size(); ++i) {
            const auto& arg = cmd.arguments[i];
            if (arg == "-I" && i + 1 < cmd.arguments.size()) {
                includes.push_back(cmd.arguments[i + 1]);
            } else if (arg.substr(0, 2) == "-I") {
                includes.push_back(arg.substr(2));
            }
        }
    }
    
    return includes;
}
