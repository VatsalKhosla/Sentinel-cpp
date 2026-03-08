#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace safecpp {

Config::Config() 
    : output_format_(OutputFormat::TEXT),
      output_file_(""),
      verbose_(false) {
    uaf_config_.enabled = true;
    uaf_config_.severity = Severity::ERROR;
    
    leak_config_.enabled = true;
    leak_config_.severity = Severity::WARNING;
    
    null_config_.enabled = true;
    null_config_.severity = Severity::ERROR;
}

Config Config::getDefault() {
    return Config();
}

bool Config::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << filepath << "\n";
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Simple JSON parsing (for now, just a basic implementation)
    // In production, use a real JSON library like nlohmann/json
    parseJSON(content);
    
    return true;
}

void Config::parseJSON(const std::string& content) {
    // Simple parser for basic config values
    // This is a minimal implementation - use a proper JSON library for production
    
    if (content.find("\"text\"") != std::string::npos) {
        output_format_ = OutputFormat::TEXT;
    } else if (content.find("\"json\"") != std::string::npos) {
        output_format_ = OutputFormat::JSON;
    } else if (content.find("\"html\"") != std::string::npos) {
        output_format_ = OutputFormat::HTML;
    } else if (content.find("\"sarif\"") != std::string::npos) {
        output_format_ = OutputFormat::SARIF;
    }
    
    if (content.find("\"verbose\": true") != std::string::npos) {
        verbose_ = true;
    }
    
    // Parse checker configurations
    if (content.find("\"use_after_free\"") != std::string::npos) {
        if (content.find("\"enabled\": false") != std::string::npos) {
            uaf_config_.enabled = false;
        }
    }
}

} // namespace safecpp
