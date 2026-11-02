#include "Processor.h"
#include "Protocol.h"
#include "PipeHelper.h"
#include <iostream>
#include <string>

// функция обработки одного сообщения
void HandleTask(PipeMessage& msg, HANDLE hPipeToVal, int runId, int& successCount, int& errorCount) {
    // каждое 3-е задание ломается на первой попытке (обычный сбой):
    bool isNormalError = (msg.requestId % 3 == 0 && msg.retryCount == 0);

    // задание с номером 72 - пусть будет ФАТАЛЬНЫМ
    bool isFatalError = (msg.requestId == STUDENT_ID);

    bool shouldFail = isNormalError || isFatalError;

    if (shouldFail) {
        msg.messageType = MSG_RESULT_ERR;
        errorCount++;
        std::string errText = "ERR: Сбой на шаге retry=" + std::to_string(msg.retryCount);
        strcpy_s(msg.payload, sizeof(msg.payload), errText.c_str());
        msg.payloadSize = static_cast<unsigned int>(errText.size());
    } else {
        msg.messageType = MSG_RESULT_OK;
        successCount++;
        std::string okText = "OK: Обработано reqId=" + std::to_string(msg.requestId);
        strcpy_s(msg.payload, sizeof(msg.payload), okText.c_str());
        msg.payloadSize = static_cast<unsigned int>(okText.size());
    }

    msg.sourcePid = GetCurrentProcessId();
    SendMessageToPipe(hPipeToVal, msg, runId);
}

void RunProcessor(int runId) {
    // идентификация процесса
    std::cout << "====================================================\n";
    std::cout << "ROLE: PROCESSOR | PID=" << GetCurrentProcessId()
              << " | VARIANT=" << VARIANT_ID
              << " | RUN_ID=" << runId << "\n";
    std::cout << "====================================================\n\n";

    // создаем две входные трубы (Серверы):
    // 1ая - для новых задач от Генератора
    HANDLE hPipeFromGen = CreateServerPipe(PIPE_GEN_TO_PROC);
    if (hPipeFromGen == INVALID_HANDLE_VALUE) return;

    // 2ая - для повторных задач от Валидатора
    HANDLE hPipeFromValRetry = CreateServerPipe(PIPE_VAL_TO_PROC);
    if (hPipeFromValRetry == INVALID_HANDLE_VALUE) {
        CloseHandle(hPipeFromGen);
        return;
    }

    // подключаемся к Валидатору (куда отдавать результаты)
    std::cout << "[Processor] Подключение к Валидатору...\n";
    HANDLE hPipeToVal = ConnectClientPipe(PIPE_PROC_TO_VAL);
    if (hPipeToVal == INVALID_HANDLE_VALUE) {
        CloseHandle(hPipeFromGen);
        CloseHandle(hPipeFromValRetry);
        return;
    }
    std::cout << "[Processor] Канал к Валидатору готов!\n";

    // ждем подключения обратного канала от Валидатора
    std::cout << "[Processor] Ожидание подключения Валидатора к каналу повторов...\n";
    if (!ConnectNamedPipe(hPipeFromValRetry, NULL)) {
        if (GetLastError() != ERROR_PIPE_CONNECTED) {
            PrintError("ConnectNamedPipe (Retry)");
        }
    }
    std::cout << "[Processor] Канал повторов подключен!\n";

    // ждем подключения Генератора
    std::cout << "[Processor] Ожидание подключения Генератора...\n";
    if (!ConnectNamedPipe(hPipeFromGen, NULL)) {
        if (GetLastError() != ERROR_PIPE_CONNECTED) {
            PrintError("ConnectNamedPipe (Gen)");
        }
    }
    std::cout << "[Processor] Генератор подключен! Начинаем обработку конвейера.\n\n";

    int processedOk = 0;
    int simulatedErrors = 0;
    PipeMessage msg;

    // обрабатываем весь поток задач от Генератора
    while (ReceiveMessageFromPipe(hPipeFromGen, msg, runId)) {
        if (msg.messageType == MSG_STOP) {
            std::cout << "[Processor] Получен сигнал STOP от Генератора.\n";
            break;
        }

        // обрабатываем свежую задачу
        HandleTask(msg, hPipeToVal, runId, processedOk, simulatedErrors);

        // проверяем: нет ли в канале повторов задач от Валидатора прямо сейчас
        DWORD bytesAvail = 0;
        if (PeekNamedPipe(hPipeFromValRetry, NULL, 0, NULL, &bytesAvail, NULL) && bytesAvail > 0) {
            PipeMessage retryMsg;
            if (ReceiveMessageFromPipe(hPipeFromValRetry, retryMsg, runId)) {
                std::cout << ">>> [Processor] Обработка ПОВТОРНОЙ задачи reqId=" << retryMsg.requestId << " <<<\n";
                HandleTask(retryMsg, hPipeToVal, runId, processedOk, simulatedErrors);
            }
        }
    }

    // добираем оставшиеся повторные задачи от Валидатора, если они еще идут
    DWORD bytesAvail = 0;
    while (PeekNamedPipe(hPipeFromValRetry, NULL, 0, NULL, &bytesAvail, NULL) && bytesAvail > 0) {
        PipeMessage retryMsg;
        if (ReceiveMessageFromPipe(hPipeFromValRetry, retryMsg, runId)) {
            std::cout << ">>> [Processor] Доработка финальной повторной задачи reqId=" << retryMsg.requestId << " <<<\n";
            HandleTask(retryMsg, hPipeToVal, runId, processedOk, simulatedErrors);
        }
    }

    // передаем сигнал STOP дальше по цепочке в Валидатор
    PipeMessage stopMsg{};
    stopMsg.version = 1;
    stopMsg.messageType = MSG_STOP;
    stopMsg.sourcePid = GetCurrentProcessId();
    SendMessageToPipe(hPipeToVal, stopMsg, runId);

    // статистика Процессора
    std::cout << "\n================ СТАТИСТИКА ПРОЦЕССОРА ================\n";
    std::cout << "Успешно обработано: " << processedOk << "\n";
    std::cout << "Сгенерировано ошибок (симуляция): " << simulatedErrors << "\n";
    std::cout << "=======================================================\n";

    // освобождение ресурсов
    DisconnectNamedPipe(hPipeFromGen);
    DisconnectNamedPipe(hPipeFromValRetry);
    CloseHandle(hPipeFromGen);
    CloseHandle(hPipeFromValRetry);
    CloseHandle(hPipeToVal);
}