#include "report_generator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

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
    html << "  <meta charset='UTF-8'>\n";
    html << "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
    html << "  <title>SafeCpp Analysis Report</title>\n";
    html << "  <style>\n";
    html << "    * { margin: 0; padding: 0; box-sizing: border-box; }\n";
    html << "    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #f8f9fa; color: #212529; line-height: 1.6; }\n";
    html << "    .container { max-width: 1400px; margin: 0 auto; padding: 20px; }\n";
    html << "    header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 12px; margin-bottom: 30px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }\n";
    html << "    h1 { font-size: 32px; font-weight: 600; margin-bottom: 8px; }\n";
    html << "    .subtitle { opacity: 0.9; font-size: 14px; }\n";
    html << "    .summary { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-bottom: 30px; }\n";
    html << "    .stat-card { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.08); border-left: 4px solid #667eea; }\n";
    html << "    .stat-card.error { border-left-color: #dc3545; }\n";
    html << "    .stat-card.warning { border-left-color: #ffc107; }\n";
    html << "    .stat-label { font-size: 13px; color: #6c757d; text-transform: uppercase; letter-spacing: 0.5px; margin-bottom: 8px; }\n";
    html << "    .stat-value { font-size: 36px; font-weight: 700; color: #212529; }\n";
    html << "    .section { background: white; border-radius: 8px; padding: 25px; margin-bottom: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.08); }\n";
    html << "    .section-title { font-size: 20px; font-weight: 600; color: #212529; margin-bottom: 20px; padding-bottom: 12px; border-bottom: 2px solid #e9ecef; }\n";
    html << "    .violation-card { background: #f8f9fa; border: 1px solid #dee2e6; border-left: 5px solid #dc3545; border-radius: 6px; padding: 20px; margin-bottom: 20px; }\n";
    html << "    .violation-card.warning { border-left-color: #ffc107; }\n";
    html << "    .violation-header { display: flex; align-items: center; justify-content: space-between; margin-bottom: 15px; }\n";
    html << "    .violation-title { font-size: 16px; font-weight: 600; color: #212529; }\n";
    html << "    .badge { display: inline-block; padding: 4px 12px; border-radius: 12px; font-size: 12px; font-weight: 600; text-transform: uppercase; }\n";
    html << "    .badge.error { background: #dc3545; color: white; }\n";
    html << "    .badge.warning { background: #ffc107; color: #212529; }\n";
    html << "    .detail-row { margin: 8px 0; font-size: 14px; }\n";
    html << "    .detail-label { font-weight: 600; color: #495057; margin-right: 8px; }\n";
    html << "    .detail-value { color: #212529; }\n";
    html << "    code { background: #e9ecef; padding: 2px 6px; border-radius: 4px; font-family: 'Courier New', monospace; font-size: 13px; color: #d63384; }\n";
    html << "    .code-snippet { background: #282c34; color: #abb2bf; padding: 16px; border-radius: 6px; overflow-x: auto; margin: 15px 0; font-family: 'Courier New', monospace; font-size: 13px; line-height: 1.5; }\n";
    html << "    .code-line { display: block; }\n";
    html << "    .code-line.highlight { background: #3e4451; border-left: 3px solid #e06c75; padding-left: 13px; margin-left: -16px; padding-right: 16px; }\n";
    html << "    .line-number { display: inline-block; width: 40px; color: #5c6370; text-align: right; margin-right: 16px; user-select: none; }\n";
    html << "    .explanation { background: #e7f3ff; border-left: 4px solid #0d6efd; padding: 12px 16px; border-radius: 4px; margin: 15px 0; font-size: 14px; }\n";
    html << "    .explanation-title { font-weight: 600; color: #0d6efd; margin-bottom: 6px; }\n";
    html << "    .suppression { background: #fff3cd; border-left: 4px solid #ffc107; padding: 12px 16px; border-radius: 4px; margin: 15px 0; font-size: 13px; }\n";
    html << "    .suppression-title { font-weight: 600; color: #856404; margin-bottom: 6px; }\n";
    html << "    .suppression code { background: #fff; color: #212529; }\n";
    html << "    .keyword { color: #c678dd; }\n";
    html << "    .string { color: #98c379; }\n";
    html << "    .comment { color: #5c6370; font-style: italic; }\n";
    html << "    .function { color: #61afef; }\n";
    html << "    .no-issues { text-align: center; padding: 60px 20px; color: #28a745; }\n";
    html << "    .no-issues-icon { font-size: 64px; margin-bottom: 16px; }\n";
    html << "    .no-issues-text { font-size: 24px; font-weight: 600; }\n";
    html << "  </style>\n";
    html << "</head>\n<body>\n";
    html << "  <div class='container'>\n";
    html << "    <header>\n";
    html << "      <h1>SafeCpp Memory Safety Analysis</h1>\n";
    html << "      <div class='subtitle'>Deep interprocedural ownership tracking for modern C++</div>\n";
    html << "    </header>\n";
    
    // Summary cards
    html << "    <div class='summary'>\n";
    html << "      <div class='stat-card'>\n";
    html << "        <div class='stat-label'>Total Issues</div>\n";
    html << "        <div class='stat-value'>" << results.total() << "</div>\n";
    html << "      </div>\n";
    html << "      <div class='stat-card error'>\n";
    html << "        <div class='stat-label'>Use-After-Free</div>\n";
    html << "        <div class='stat-value'>" << results.uaf_violations.size() << "</div>\n";
    html << "      </div>\n";
    html << "      <div class='stat-card warning'>\n";
    html << "        <div class='stat-label'>Memory Leaks</div>\n";
    html << "        <div class='stat-value'>" << results.leak_violations.size() << "</div>\n";
    html << "      </div>\n";
    html << "      <div class='stat-card error'>\n";
    html << "        <div class='stat-label'>Null Dereferences</div>\n";
    html << "        <div class='stat-value'>" << results.null_violations.size() << "</div>\n";
    html << "      </div>\n";
    html << "    </div>\n";
    
    // Use-after-free violations
    if (!results.uaf_violations.empty()) {
        html << "    <div class='section'>\n";
        html << "      <div class='section-title'>Use-After-Free Violations</div>\n";
        for (const auto& v : results.uaf_violations) {
            html << "      <div class='violation-card'>\n";
            html << "        <div class='violation-header'>\n";
            html << "          <div class='violation-title'>Memory accessed after being freed</div>\n";
            html << "          <span class='badge error'>ERROR</span>\n";
            html << "        </div>\n";
            html << "        <div class='detail-row'><span class='detail-label'>Variable:</span><code>" << htmlEscape(v.variable) << "</code></div>\n";
            html << "        <div class='detail-row'><span class='detail-label'>Freed at line:</span><span class='detail-value'>" << v.free_line << "</span></div>\n";
            html << "        <div class='detail-row'><span class='detail-label'>Used at line:</span><span class='detail-value'>" << v.use_line << "</span></div>\n";
            html << "        <div class='explanation'>\n";
            html << "          <div class='explanation-title'>⚠️ Why This Matters</div>\n";
            html << "          " << getViolationExplanation("use-after-free") << "\n";
            html << "        </div>\n";
            html << "        <div class='suppression'>\n";
            html << "          <div class='suppression-title'>💡 Suppression Suggestion</div>\n";
            html << "          " << getSuppressionSuggestion("use-after-free") << "\n";
            html << "        </div>\n";
            html << "      </div>\n";
        }
        html << "    </div>\n";
    }
    
    // Memory leak violations
    if (!results.leak_violations.empty()) {
        html << "    <div class='section'>\n";
        html << "      <div class='section-title'>Memory Leak Violations</div>\n";
        for (const auto& v : results.leak_violations) {
            html << "      <div class='violation-card warning'>\n";
            html << "        <div class='violation-header'>\n";
            html << "          <div class='violation-title'>Allocated memory never freed</div>\n";
            html << "          <span class='badge warning'>WARNING</span>\n";
            html << "        </div>\n";
            html << "        <div class='detail-row'><span class='detail-label'>Variable:</span><code>" << htmlEscape(v.variable) << "</code></div>\n";
            html << "        <div class='detail-row'><span class='detail-label'>Allocated at line:</span><span class='detail-value'>" << v.alloc_line << "</span></div>\n";
            html << "        <div class='explanation'>\n";
            html << "          <div class='explanation-title'>⚠️ Why This Matters</div>\n";
            html << "          " << getViolationExplanation("memory-leak") << "\n";
            html << "        </div>\n";
            html << "        <div class='suppression'>\n";
            html << "          <div class='suppression-title'>💡 Suppression Suggestion</div>\n";
            html << "          " << getSuppressionSuggestion("memory-leak") << "\n";
            html << "        </div>\n";
            html << "      </div>\n";
        }
        html << "    </div>\n";
    }
    
    // Null pointer violations
    if (!results.null_violations.empty()) {
        html << "    <div class='section'>\n";
        html << "      <div class='section-title'>Null Pointer Dereference Violations</div>\n";
        for (const auto& v : results.null_violations) {
            html << "      <div class='violation-card'>\n";
            html << "        <div class='violation-header'>\n";
            html << "          <div class='violation-title'>Null pointer dereferenced</div>\n";
            html << "          <span class='badge error'>ERROR</span>\n";
            html << "        </div>\n";
            html << "        <div class='detail-row'><span class='detail-label'>Variable:</span><code>" << htmlEscape(v.variable) << "</code></div>\n";
            html << "        <div class='detail-row'><span class='detail-label'>Assigned null at line:</span><span class='detail-value'>" << v.null_assign_line << "</span></div>\n";
            html << "        <div class='detail-row'><span class='detail-label'>Dereferenced at line:</span><span class='detail-value'>" << v.deref_line << "</span></div>\n";
            html << "        <div class='explanation'>\n";
            html << "          <div class='explanation-title'>⚠️ Why This Matters</div>\n";
            html << "          " << getViolationExplanation("null-dereference") << "\n";
            html << "        </div>\n";
            html << "        <div class='suppression'>\n";
            html << "          <div class='suppression-title'>💡 Suppression Suggestion</div>\n";
            html << "          " << getSuppressionSuggestion("null-dereference") << "\n";
            html << "        </div>\n";
            html << "      </div>\n";
        }
        html << "    </div>\n";
    }
    
    // No issues found
    if (results.total() == 0) {
        html << "    <div class='section no-issues'>\n";
        html << "      <div class='no-issues-icon'>✓</div>\n";
        html << "      <div class='no-issues-text'>No memory safety issues found!</div>\n";
        html << "    </div>\n";
    }
    
    html << "  </div>\n";
    html << "</body>\n</html>\n";
    
    return html.str();
}

std::string ReportGenerator::htmlEscape(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

std::string ReportGenerator::getViolationExplanation(const std::string& check_type) {
    if (check_type == "use-after-free") {
        return "Accessing memory after it has been freed causes undefined behavior. "
               "This can lead to crashes, data corruption, or security vulnerabilities (CVE-2021-xxxxx pattern). "
               "The freed memory may have been reallocated for a different purpose, causing unpredictable program behavior.";
    } else if (check_type == "memory-leak") {
        return "Memory leaks occur when dynamically allocated memory is never freed, causing the program's memory usage to grow over time. "
               "While not immediately dangerous like UAF, leaks can degrade performance, exhaust system resources, and eventually cause out-of-memory errors. "
               "Consider using RAII patterns (std::unique_ptr, std::shared_ptr) to automatically manage memory lifetime.";
    } else if (check_type == "null-dereference") {
        return "Dereferencing a null pointer is undefined behavior that typically causes a segmentation fault (crash). "
               "Always check pointers for null before dereferencing, especially after operations that might fail (malloc, dynamic_cast, etc.). "
               "Consider using optional types or references to enforce non-null contracts at the API level.";
    }
    return "Memory safety violation detected.";
}

std::string ReportGenerator::getSuppressionSuggestion(const std::string& check_type) {
    if (check_type == "use-after-free") {
        return "To suppress this check, add to your config file:<br><code>\"checks\": { \"use-after-free\": { \"enabled\": false } }</code><br>"
               "Or use modern alternatives: <code>std::unique_ptr</code> ensures single ownership and automatic cleanup.";
    } else if (check_type == "memory-leak") {
        return "To suppress this check, add to your config file:<br><code>\"checks\": { \"memory-leak\": { \"enabled\": false } }</code><br>"
               "Better solution: Replace raw <code>new/delete</code> with RAII: <code>auto ptr = std::make_unique&lt;T&gt;();</code>";
    } else if (check_type == "null-dereference") {
        return "To suppress this check, add to your config file:<br><code>\"checks\": { \"null-dereference\": { \"enabled\": false } }</code><br>"
               "Better solution: Add null checks: <code>if (ptr != nullptr) { *ptr = value; }</code> or use <code>std::optional</code>.";
    }
    return "See documentation for suppression options.";
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
