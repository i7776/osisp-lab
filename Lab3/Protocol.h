#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <iomanip>

const int VARIANT_ID = 15;
const int STUDENT_ID = 72;

// Имена наших каналов (в Windows они ВСЕГДА начинаются с \\.\pipe\)
const char* const PIPE_GEN_TO_PROC  = "\\\\.\\pipe\\Pipe_Gen_To_Proc_72";
const char* const PIPE_PROC_TO_VAL  = "\\\\.\\pipe\\Pipe_Proc_To_Val_72";
const char* const PIPE_VAL_TO_COL   = "\\\\.\\pipe\\Pipe_Val_To_Col_72";
const char* const PIPE_VAL_TO_PROC  = "\\\\.\\pipe\\Pipe_Val_To_Proc_Retry_72"; // Канал для повторов!

// типы сообщений
enum MessageType {
    MSG_TASK       = 1, // обычное задание
    MSG_RESULT_OK  = 2, // успешный результат
    MSG_RESULT_ERR = 3, // результат с ошибкой
    MSG_STOP       = 4  // сигнал остановки работы
};

// номер типа в слово для логов
inline std::string TypeToString(unsigned int type) {
    switch (type) {
        case MSG_TASK:       return "TASK";
        case MSG_RESULT_OK:  return "RESULT_OK";
        case MSG_RESULT_ERR: return "RESULT_ERR";
        case MSG_STOP:       return "STOP";
        default:             return "UNKNOWN";
    }
}


// структура сообщения для передачи по Named Pipe
#pragma pack(push, 1) // чтобы компилятор не вставлял пустые байты между полями
struct PipeMessage {
    unsigned int version;      // версия протокола (всегда 1)
    unsigned int messageType;  // тип сообщения
    unsigned int sourcePid;    // PID процесса
    unsigned int requestId;    // номер задания
    unsigned int retryCount;   // раз повтор
    unsigned int payloadSize;  // сколько полезных букв
    char payload[256];         // данные
};
#pragma pack(pop)

// функция вывода времени: [ЧЧ:ММ:СС.мс]
inline void LogEvent(const std::string& action, const PipeMessage& msg, int runId) {
    SYSTEMTIME st;
    GetLocalTime(&st); // системное время Windows

    std::cout << "["
              << std::setfill('0') << std::setw(2) << st.wHour << ":"
              << std::setfill('0') << std::setw(2) << st.wMinute << ":"
              << std::setfill('0') << std::setw(2) << st.wSecond << "."
              << std::setfill('0') << std::setw(3) << st.wMilliseconds << "] "
              << "PID=" << GetCurrentProcessId() << " | "
              << "RUN_ID=" << runId << " | "
              << action << " | "
              << "reqId=" << msg.requestId << " | "
              << "type=" << TypeToString(msg.messageType) << " | "
              << "retry=" << msg.retryCount << " | "
              << "size=" << msg.payloadSize << " | "
              << "data=\"" << msg.payload << "\""
              << std::endl;
}