#include "report_generator.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace safecpp {

ReportGenerator::ReportGenerator(const Config& config) 
    : config_(config) {}

void ReportGenerator::generate(const AnalysisResults& results) {
    switch (config_.getOutputFormat()) {
        case OutputFormat::TEXT:
            generateText(results);
            break;
        case OutputFormat::JSON:
            generateJSON(results);
            break;
        case OutputFormat::HTML:
            generateHTML(results);
            break;
        case OutputFormat::SARIF:
            generateSARIF(results);
            break;
    }
}

void ReportGenerator::generateText(const AnalysisResults& results) {
    if (results.total() == 0) {
        std::cout << "\n✓ No memory safety issues found!\n";
        return;
    }
    
    // Use-after-free violations
    if (!results.uaf_violations.empty() && config_.getUseAfterFreeConfig().enabled) {
        std::cout << "\n=== Use-After-Free Detection Results ===\n\n";
        for (const auto& v : results.uaf_violations) {
            std::cout << "error: " << v.message << "\n";
            std::cout << "  Variable: " << v.variable << "\n";
            std::cout << "  Freed at line: " << v.free_line << "\n";
            std::cout << "  Used at line: " << v.use_line << "\n\n";
        }
        std::cout << "Found " << results.uaf_violations.size() << " memory safety violation(s)\n";
    }
    
    // Memory leak violations
    if (!results.leak_violations.empty() && config_.getMemoryLeakConfig().enabled) {
        std::cout << "\n=== Memory Leak Detection Results ===\n\n";
        for (const auto& v : results.leak_violations) {
            std::cout << "warning: memory leak detected\n";
            std::cout << "  Variable: " << v.variable << "\n";
            std::cout << "  Allocated at line: " << v.alloc_line << "\n";
            std::cout << "  " << v.message << "\n\n";
        }
        std::cout << "Found " << results.leak_violations.size() << " potential memory leak(s)\n";
    }
    
    // Null pointer violations
    if (!results.null_violations.empty() && config_.getNullDerefConfig().enabled) {
        std::cout << "\n=== Null Pointer Dereference Detection Results ===\n\n";
        for (const auto& v : results.null_violations) {
            std::cout << "error: null pointer dereference\n";
            std::cout << "  Variable: " << v.variable << "\n";
            std::cout << "  Assigned null at line: " << v.null_assign_line << "\n";
            std::cout << "  Dereferenced at line: " << v.deref_line << "\n";
            std::cout << "  " << v.message << "\n\n";
        }
        std::cout << "Found " << results.null_violations.size() << " null pointer dereference(s)\n";
    }
    
    // Summary
    std::cout << "\n=== Summary ===\n";
    std::cout << "Total issues found: " << results.total() << "\n";
    std::cout << "  - Use-after-free: " << results.uaf_violations.size() << "\n";
    std::cout << "  - Memory leaks: " << results.leak_violations.size() << "\n";
    std::cout << "  - Null dereferences: " << results.null_violations.size() << "\n";
}

void ReportGenerator::generateJSON(const AnalysisResults& results) {
    std::string json = toJSON(results);
    
    if (config_.getOutputFile().empty()) {
        std::cout << json << std::endl;
    } else {
        std::ofstream file(config_.getOutputFile());
        file << json;
        std::cout << "JSON report written to: " << config_.getOutputFile() << "\n";
    }
}

void ReportGenerator::generateHTML(const AnalysisResults& results) {
    std::string html = toHTML(results);
    
    std::string filename = config_.getOutputFile().empty() ? "safecpp_report.html" : config_.getOutputFile();
    std::ofstream file(filename);
    file << html;
    std::cout << "HTML report written to: " << filename << "\n";
}

void ReportGenerator::generateSARIF(const AnalysisResults& results) {
    std::string sarif = toSARIF(results);
    
    std::string filename = config_.getOutputFile().empty() ? "safecpp_report.sarif" : config_.getOutputFile();
    std::ofstream file(filename);
    file << sarif;
    std::cout << "SARIF report written to: " << filename << "\n";
}

std::string ReportGenerator::toJSON(const AnalysisResults& results) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"tool\": \"SafeCpp\",\n";
    json << "  \"version\": \"1.0.0\",\n";
    json << "  \"summary\": {\n";
    json << "    \"total_issues\": " << results.total() << ",\n";
    json << "    \"use_after_free\": " << results.uaf_violations.size() << ",\n";
    json << "    \"memory_leaks\": " << results.leak_violations.size() << ",\n";
    json << "    \"null_dereferences\": " << results.null_violations.size() << "\n";
    json << "  },\n";
    json << "  \"violations\": [\n";
    
    bool first = true;
    
    // Use-after-free violations
    for (const auto& v : results.uaf_violations) {
        if (!first) json << ",\n";
        json << "    {\n";
        json << "      \"type\": \"use-after-free\",\n";
        json << "      \"severity\": \"error\",\n";
        json << "      \"variable\": \"" << v.variable << "\",\n";
        json << "      \"free_line\": " << v.free_line << ",\n";
        json << "      \"use_line\": " << v.use_line << ",\n";
        json << "      \"message\": \"" << v.message << "\"\n";
        json << "    }";
        first = false;
    }
    
    // Memory leak violations
    for (const auto& v : results.leak_violations) {
        if (!first) json << ",\n";
        json << "    {\n";
        json << "      \"type\": \"memory-leak\",\n";
        json << "      \"severity\": \"warning\",\n";
        json << "      \"variable\": \"" << v.variable << "\",\n";
        json << "      \"alloc_line\": " << v.alloc_line << ",\n";
        json << "      \"message\": \"" << v.message << "\"\n";
        json << "    }";
        first = false;
    }
    
    // Null pointer violations
    for (const auto& v : results.null_violations) {
        if (!first) json << ",\n";
        json << "    {\n";
        json << "      \"type\": \"null-dereference\",\n";
        json << "      \"severity\": \"error\",\n";
        json << "      \"variable\": \"" << v.variable << "\",\n";
        json << "      \"null_assign_line\": " << v.null_assign_line << ",\n";
        json << "      \"deref_line\": " << v.deref_line << ",\n";
        json << "      \"message\": \"" << v.message << "\"\n";
        json << "    }";
        first = false;
    }
    
    json << "\n  ]\n";
    json << "}\n";
    
    return json.str();
}

std::string ReportGenerator::toHTML(const AnalysisResults& results) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n<head>\n";
    html << "  <title>SafeCpp Analysis Report</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    html << "    .container { max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    html << "    h1 { color: #333; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }\n";
    html << "    h2 { color: #555; margin-top: 30px; }\n";
    html << "    .summary { background: #e8f5e9; padding: 15px; border-radius: 5px; margin: 20px 0; }\n";
    html << "    .violation { background: #fff3e0; border-left: 4px solid #ff9800; padding: 10px; margin: 10px 0; }\n";
    html << "    .error { background: #ffebee; border-left: 4px solid #f44336; }\n";
    html << "    .warning { background: #fff3e0; border-left: 4px solid #ff9800; }\n";
    html << "    .stat { display: inline-block; margin: 10px 20px 10px 0; }\n";
    html << "    .stat-value { font-size: 24px; font-weight: bold; color: #4CAF50; }\n";
    html << "    code { background: #f5f5f5; padding: 2px 6px; border-radius: 3px; }\n";
    html << "  </style>\n";
    html << "</head>\n<body>\n";
    html << "  <div class='container'>\n";
    html << "    <h1>SafeCpp Memory Safety Analysis Report</h1>\n";
    
    // Summary section
    html << "    <div class='summary'>\n";
    html << "      <h2>Summary</h2>\n";
    html << "      <div class='stat'>Total Issues: <span class='stat-value'>" << results.total() << "</span></div>\n";
    html << "      <div class='stat'>Use-After-Free: <span class='stat-value'>" << results.uaf_violations.size() << "</span></div>\n";
    html << "      <div class='stat'>Memory Leaks: <span class='stat-value'>" << results.leak_violations.size() << "</span></div>\n";
    html << "      <div class='stat'>Null Dereferences: <span class='stat-value'>" << results.null_violations.size() << "</span></div>\n";
    html << "    </div>\n";
    
    // Use-after-free violations
    if (!results.uaf_violations.empty()) {
        html << "    <h2>Use-After-Free Violations</h2>\n";
        for (const auto& v : results.uaf_violations) {
            html << "    <div class='violation error'>\n";
            html << "      <strong>Variable:</strong> <code>" << v.variable << "</code><br>\n";
            html << "      <strong>Freed at line:</strong> " << v.free_line << "<br>\n";
            html << "      <strong>Used at line:</strong> " << v.use_line << "<br>\n";
            html << "      <strong>Message:</strong> " << v.message << "\n";
            html << "    </div>\n";
        }
    }
    
    // Memory leak violations
    if (!results.leak_violations.empty()) {
        html << "    <h2>Memory Leak Violations</h2>\n";
        for (const auto& v : results.leak_violations) {
            html << "    <div class='violation warning'>\n";
            html << "      <strong>Variable:</strong> <code>" << v.variable << "</code><br>\n";
            html << "      <strong>Allocated at line:</strong> " << v.alloc_line << "<br>\n";
            html << "      <strong>Message:</strong> " << v.message << "\n";
            html << "    </div>\n";
        }
    }
    
    // Null pointer violations
    if (!results.null_violations.empty()) {
        html << "    <h2>Null Pointer Dereference Violations</h2>\n";
        for (const auto& v : results.null_violations) {
            html << "    <div class='violation error'>\n";
            html << "      <strong>Variable:</strong> <code>" << v.variable << "</code><br>\n";
            html << "      <strong>Assigned null at line:</strong> " << v.null_assign_line << "<br>\n";
            html << "      <strong>Dereferenced at line:</strong> " << v.deref_line << "<br>\n";
            html << "      <strong>Message:</strong> " << v.message << "\n";
            html << "    </div>\n";
        }
    }
    
    html << "  </div>\n";
    html << "</body>\n</html>\n";
    
    return html.str();
}

std::string ReportGenerator::toSARIF(const AnalysisResults& results) {
    std::ostringstream sarif;
    sarif << "{\n";
    sarif << "  \"version\": \"2.1.0\",\n";
    sarif << "  \"$schema\": \"https://raw.githubusercontent.com/oasis-tcs/sarif-spec/master/Schemata/sarif-schema-2.1.0.json\",\n";
    sarif << "  \"runs\": [{\n";
    sarif << "    \"tool\": {\n";
    sarif << "      \"driver\": {\n";
    sarif << "        \"name\": \"SafeCpp\",\n";
    sarif << "        \"version\": \"1.0.0\",\n";
    sarif << "        \"informationUri\": \"https://github.com/safecpp/safecpp\"\n";
    sarif << "      }\n";
    sarif << "    },\n";
    sarif << "    \"results\": [\n";
    
    bool first = true;
    
    // Use-after-free violations
    for (const auto& v : results.uaf_violations) {
        if (!first) sarif << ",\n";
        sarif << "      {\n";
        sarif << "        \"ruleId\": \"use-after-free\",\n";
        sarif << "        \"level\": \"error\",\n";
        sarif << "        \"message\": { \"text\": \"" << v.message << "\" },\n";
        sarif << "        \"locations\": [{\n";
        sarif << "          \"physicalLocation\": {\n";
        sarif << "            \"region\": { \"startLine\": " << v.use_line << " }\n";
        sarif << "          }\n";
        sarif << "        }]\n";
        sarif << "      }";
        first = false;
    }
    
    // Memory leak violations
    for (const auto& v : results.leak_violations) {
        if (!first) sarif << ",\n";
        sarif << "      {\n";
        sarif << "        \"ruleId\": \"memory-leak\",\n";
        sarif << "        \"level\": \"warning\",\n";
        sarif << "        \"message\": { \"text\": \"" << v.message << "\" },\n";
        sarif << "        \"locations\": [{\n";
        sarif << "          \"physicalLocation\": {\n";
        sarif << "            \"region\": { \"startLine\": " << v.alloc_line << " }\n";
        sarif << "          }\n";
        sarif << "        }]\n";
        sarif << "      }";
        first = false;
    }
    
    // Null pointer violations
    for (const auto& v : results.null_violations) {
        if (!first) sarif << ",\n";
        sarif << "      {\n";
        sarif << "        \"ruleId\": \"null-dereference\",\n";
        sarif << "        \"level\": \"error\",\n";
        sarif << "        \"message\": { \"text\": \"" << v.message << "\" },\n";
        sarif << "        \"locations\": [{\n";
        sarif << "          \"physicalLocation\": {\n";
        sarif << "            \"region\": { \"startLine\": " << v.deref_line << " }\n";
        sarif << "          }\n";
        sarif << "        }]\n";
        sarif << "      }";
        first = false;
    }
    
    sarif << "\n    ]\n";
    sarif << "  }]\n";
    sarif << "}\n";
    
    return sarif.str();
}

std::string ReportGenerator::severityToString(Severity severity) {
    switch (severity) {
        case Severity::INFO: return "info";
        case Severity::WARNING: return "warning";
        case Severity::ERROR: return "error";
        default: return "unknown";
    }
}

} // namespace safecpp
