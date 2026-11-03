#ifndef TEXT_ANALYZER_H
#define TEXT_ANALYZER_H

#include <string>
#include "../common/Protocol.h"

class TextAnalyzer {
public:
    static AnalysisResponsePayload Analyze(const std::string& text);

private:
    static bool IsWordChar(char c);
    static bool IsDigit(char c);
    static bool IsLatinLetter(char c);
};

#endif // TEXT_ANALYZER_H