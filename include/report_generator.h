#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include "config.h"
#include "uaf_detector.h"
#include "memory_leak_detector.h"
#include "null_deref_detector.h"
#include <string>
#include <vector>

namespace safecpp {

struct AnalysisResults {
    std::vector<UAFViolation> uaf_violations;
    std::vector<MemoryLeakViolation> leak_violations;
    std::vector<NullDerefViolation> null_violations;
    
    int total() const {
        return uaf_violations.size() + leak_violations.size() + null_violations.size();
    }
};

class ReportGenerator {
public:
    explicit ReportGenerator(const Config& config);
    
    void generate(const AnalysisResults& results);
    
private:
    const Config& config_;
    
    void generateText(const AnalysisResults& results);
    void generateJSON(const AnalysisResults& results);
    void generateHTML(const AnalysisResults& results);
    void generateSARIF(const AnalysisResults& results);
    
    std::string toJSON(const AnalysisResults& results);
    std::string toHTML(const AnalysisResults& results);
    std::string toSARIF(const AnalysisResults& results);
    
    std::string severityToString(Severity severity);
    std::string getCodeSnippet(const std::string& file_path, unsigned int line, int context_lines = 3);
    std::string htmlEscape(const std::string& str);
    std::string getViolationExplanation(const std::string& check_type);
    std::string getSuppressionSuggestion(const std::string& check_type);
};

} // namespace safecpp

#endif // REPORT_GENERATOR_H
