#ifndef PROTOCOL_H
#define PROTOCOL_H

// параметры варианта 15
const unsigned short DEFAULT_PORT = 20072;
const unsigned int DEFAULT_SEED = 7984;
const char* const DEFAULT_HOST = "127.0.0.1";
const unsigned int MAX_PAYLOAD_SIZE = 65536;
const unsigned short PROTOCOL_VERSION = 1;

enum MessageType {
    MSG_TEXT_REQUEST = 1,    // клиент -> сервер: отправка текста
    MSG_ANALYSIS_RESULT = 2, // сервер -> клиент: результаты анализа
    MSG_ERROR = 3,           // сервер -> клиент: ошибка формата
    MSG_DISCONNECT = 4       // завершение
};

#pragma pack(push, 1)

// заголовок сетевого сообщения
struct MessageHeader {
    unsigned int payloadLength; // длина полезных данных в байтах
    unsigned short version;
    unsigned short msgType;
    unsigned int requestId;
    unsigned int clientId;
};

struct AnalysisResponsePayload {
    unsigned long long totalBytes;   // кол-во байтов
    unsigned long long totalLines;   // количество строк
    unsigned long long totalWords;   // количество слов ([A-Za-z0-9]+)
    unsigned long long totalDigits;  // количество цифр
    unsigned long long totalLetters; // количество латинских букв (A-Z, a-z)
};

#pragma pack(pop)

#endif