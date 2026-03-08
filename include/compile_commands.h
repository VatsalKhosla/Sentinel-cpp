#ifndef COMPILE_COMMANDS_H
#define COMPILE_COMMANDS_H

#include "llvm/Support/JSON.h"
#include <string>
#include <vector>
#include <map>

namespace safecpp {

// Represents a single compilation command from compile_commands.json
struct CompileCommand {
    std::string file;        // Source file path
    std::string directory;   // Working directory
    std::string command;     // Full command line
    std::vector<std::string> arguments;  // Parsed arguments
};

class CompileCommandsParser {
public:
    // Parse compile_commands.json file
    bool parse(const std::string& json_file);
    
    // Get all commands
    const std::vector<CompileCommand>& getCommands() const { return commands_; }
    
    // Get command for a specific file
    const CompileCommand* getCommandForFile(const std::string& file) const;
    
    // Get all source files
    std::vector<std::string> getSourceFiles() const;
    
    // Extract include paths from compile commands
    std::vector<std::string> getIncludePaths() const;
    
private:
    std::vector<CompileCommand> commands_;
    std::map<std::string, const CompileCommand*> file_to_command_;
    
    bool parseJSONArray(const llvm::json::Array& array);
    CompileCommand parseCommand(const llvm::json::Object& obj);
};

} // namespace safecpp

#endif // COMPILE_COMMANDS_H
