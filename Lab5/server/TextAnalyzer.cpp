#include "TextAnalyzer.h"

bool TextAnalyzer::IsWordChar(char c) {
    return IsDigit(c) || IsLatinLetter(c);
}

bool TextAnalyzer::IsDigit(char c) {
    return (c >= '0' && c <= '9');
}

bool TextAnalyzer::IsLatinLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

AnalysisResponsePayload TextAnalyzer::Analyze(const std::string& text) {
    AnalysisResponsePayload result{};
    result.totalBytes = static_cast<unsigned long long>(text.size());

    // если текст полностью пустой
    if (text.empty()) {
        return result;
    }

    bool inWord = false;
    unsigned long long newlineCount = 0;

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];

        // подсчет цифр
        if (IsDigit(c)) {
            result.totalDigits++;
        }

        // подсчет латинских букв
        if (IsLatinLetter(c)) {
            result.totalLetters++;
        }

        // подсчет символов переноса строки
        if (c == '\n') {
            newlineCount++;
        }

        // подсчет слов через автомат состояний
        if (IsWordChar(c)) {
            if (!inWord) {
                inWord = true;
                result.totalWords++;
            }
        } else {
            inWord = false;
        }
    }

    // расчет итогового числа строк
    if (newlineCount == 0) {
        result.totalLines = 1;
    } else {
        result.totalLines = (text.back() == '\n') ? newlineCount : (newlineCount + 1);
    }

    return result;
}