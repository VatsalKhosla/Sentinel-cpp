#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "llvm/Support/JSON.h"

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
    return parseJSON(content);
}

namespace {

OutputFormat parseOutputFormat(const std::string& format) {
    if (format == "json") {
        return OutputFormat::JSON;
    }
    if (format == "html") {
        return OutputFormat::HTML;
    }
    if (format == "sarif") {
        return OutputFormat::SARIF;
    }
    return OutputFormat::TEXT;
}

Severity parseSeverity(const std::string& severity) {
    if (severity == "info") {
        return Severity::INFO;
    }
    if (severity == "warning") {
        return Severity::WARNING;
    }
    return Severity::ERROR;
}

void applyCheckerConfig(const llvm::json::Object* obj, CheckerConfig& config) {
    if (!obj) {
        return;
    }

    auto enabled = obj->getBoolean("enabled");
    if (enabled.has_value()) {
        config.enabled = *enabled;
    }

    auto severity = obj->getString("severity");
    if (severity.has_value()) {
        config.severity = parseSeverity(severity->str());
    }
}

} // namespace

bool Config::parseJSON(const std::string& content) {
    auto parsed = llvm::json::parse(content);
    if (!parsed) {
        std::cerr << "Warning: Failed to parse config JSON\n";
        return false;
    }

    const auto* root = parsed->getAsObject();
    if (!root) {
        std::cerr << "Warning: Config root must be a JSON object\n";
        return false;
    }

    auto output_format = root->getString("output_format");
    if (output_format.has_value()) {
        output_format_ = parseOutputFormat(output_format->str());
    }

    if (const auto* output = root->getObject("output")) {
        auto format = output->getString("format");
        if (format.has_value()) {
            output_format_ = parseOutputFormat(format->str());
        }
        auto file = output->getString("file");
        if (file.has_value()) {
            output_file_ = file->str();
        }
        auto output_verbose = output->getBoolean("verbose");
        if (output_verbose.has_value()) {
            verbose_ = *output_verbose;
        }
    }

    auto output_file = root->getString("output_file");
    if (output_file.has_value()) {
        output_file_ = output_file->str();
    }

    auto root_verbose = root->getBoolean("verbose");
    if (root_verbose.has_value()) {
        verbose_ = *root_verbose;
    }

    if (const auto* checkers = root->getObject("checkers")) {
        applyCheckerConfig(checkers->getObject("use_after_free"), uaf_config_);
        applyCheckerConfig(checkers->getObject("memory_leak"), leak_config_);
        applyCheckerConfig(checkers->getObject("null_dereference"), null_config_);
    }
    
    return true;
}

} // namespace safecpp
