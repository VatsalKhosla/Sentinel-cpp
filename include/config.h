#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <map>

namespace safecpp {

enum class OutputFormat {
    TEXT,
    JSON,
    HTML,
    SARIF
};

enum class Severity {
    INFO,
    WARNING,
    ERROR
};

struct CheckerConfig {
    bool enabled = true;
    Severity severity = Severity::ERROR;
};

class Config {
public:
    Config();
    
    // Load configuration from JSON file
    bool loadFromFile(const std::string& filepath);
    
    // Getters
    OutputFormat getOutputFormat() const { return output_format_; }
    std::string getOutputFile() const { return output_file_; }
    bool isVerbose() const { return verbose_; }
    
    // Checker configurations
    CheckerConfig getUseAfterFreeConfig() const { return uaf_config_; }
    CheckerConfig getMemoryLeakConfig() const { return leak_config_; }
    CheckerConfig getNullDerefConfig() const { return null_config_; }
    
    // Setters
    void setOutputFormat(OutputFormat format) { output_format_ = format; }
    void setOutputFile(const std::string& file) { output_file_ = file; }
    void setVerbose(bool verbose) { verbose_ = verbose; }
    
    // Default configuration
    static Config getDefault();
    
private:
    OutputFormat output_format_;
    std::string output_file_;
    bool verbose_;
    
    CheckerConfig uaf_config_;
    CheckerConfig leak_config_;
    CheckerConfig null_config_;
    
    void parseJSON(const std::string& content);
};

} // namespace safecpp

#endif // CONFIG_H
