#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

#include "Protocol.h"
#include "PipeHelper.h"
#include "Collector.h"
#include "Validator.h"
#include "Processor.h"
#include "Generator.h"
#include <chrono>

void RunBenchmark() {
    std::cout << "\n================ ТЕСТ ПРОИЗВОДИТЕЛЬНОСТИ IPC (1000 СООБЩЕНИЙ) ================\n";
    const char* benchPipe = "\\\\.\\pipe\\BenchPipe_72";
    const int MSG_COUNT = 1000;

    // Тест для 64 байт
    {
        HANDLE hServer = CreateNamedPipeA(benchPipe, PIPE_ACCESS_DUPLEX,
                                          PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 65536, 65536, 0, NULL);
        HANDLE hClient = CreateFileA(benchPipe, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        PipeMessage msg{};
        msg.payloadSize = 64;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < MSG_COUNT; ++i) {
            DWORD written = 0, read = 0;
            WriteFile(hClient, &msg, sizeof(PipeMessage), &written, NULL);
            ReadFile(hServer, &msg, sizeof(PipeMessage), &read, NULL);
        }

        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        double msgPerSec = (MSG_COUNT / (ms / 1000.0));

        std::cout << "[Тест 1 | 64 байта]: Время = " << ms << " мс | Скорость = " << (int)msgPerSec << " сообщ/сек\n";

        CloseHandle(hClient);
        CloseHandle(hServer);
    }

    // Тест для 4 КБ
    {
        HANDLE hServer = CreateNamedPipeA(benchPipe, PIPE_ACCESS_DUPLEX,
                                          PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 65536, 65536, 0, NULL);
        HANDLE hClient = CreateFileA(benchPipe, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        std::vector<char> bigPayload(4096, 'A');

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < MSG_COUNT; ++i) {
            DWORD written = 0, read = 0;
            WriteFile(hClient, bigPayload.data(), 4096, &written, NULL);
            ReadFile(hServer, bigPayload.data(), 4096, &read, NULL);
        }

        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        double msgPerSec = (MSG_COUNT / (ms / 1000.0));

        std::cout << "[Тест 2 | 4 КБ]:     Время = " << ms << " мс | Скорость = " << (int)msgPerSec << " сообщ/сек\n";

        CloseHandle(hClient);
        CloseHandle(hServer);
    }
    std::cout << "===============================================================================\n\n";
}

// функция для запуска дочернего процесса в отдельном окне консоли
HANDLE SpawnProcess(const std::string& appPath, const std::string& role, int runId, int taskCount) {
    std::string cmd = "\"" + appPath + "\" --role " + role +
                      " --run-id " + std::to_string(runId) +
                      " --tasks " + std::to_string(taskCount);

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL success = CreateProcessA(
            NULL,
            cmd.data(),
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_CONSOLE, // каждому процессу свое окно
            NULL,
            NULL,
            &si,
            &pi
    );

    if (!success) {
        PrintError("CreateProcess (" + role + ")");
        return INVALID_HANDLE_VALUE;
    }

    // дескриптор первичного потока сразу закрываем
    CloseHandle(pi.hThread);
    return pi.hProcess; // возвращаем дескриптор процесса
}

int main(int argc, char* argv[]) {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    std::string role = "master";
    int runId = STUDENT_ID;                     // По умолчанию 72
    int taskCount = 100 + STUDENT_ID;           // По формуле: 100 + 72 = 172 задачи

    // разбор аргументов командной строки (если процесс запущен как дочерний)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--role" && i + 1 < argc) {
            role = argv[++i];
        } else if (arg == "--run-id" && i + 1 < argc) {
            runId = std::stoi(argv[++i]);
        } else if (arg == "--tasks" && i + 1 < argc) {
            taskCount = std::stoi(argv[++i]);
        }
    }

    // выполнение роли конкретного процесса
    if (role == "collector") {
        RunCollector(runId);
        std::cout << "\nНажмите Enter для закрытия окна...";
        std::cin.get();
        return 0;
    }
    else if (role == "validator") {
        RunValidator(runId);
        std::cout << "\nНажмите Enter для закрытия окна...";
        std::cin.get();
        return 0;
    }
    else if (role == "processor") {
        RunProcessor(runId);
        std::cout << "\nНажмите Enter для закрытия окна...";
        std::cin.get();
        return 0;
    }
    else if (role == "generator") {
        RunGenerator(runId, taskCount);
        std::cout << "\nНажмите Enter для закрытия окна...";
        std::cin.get();
        return 0;
    }


    // ГЛАВНЫЙ ДИСПЕТЧЕР: Запускает все 4 процесса

    std::cout << "============================================================\n";
    std::cout << "ЛАБОРАТОРНАЯ РАБОТА №3: IPC ЧЕРЕЗ NAMED PIPE\n";
    std::cout << "Вариант " << VARIANT_ID << " | STUDENT_ID=" << STUDENT_ID << " | RUN_ID=" << runId << "\n";
    std::cout << "Конвейер: Generator -> Processor -> Validator -> Collector\n";
    std::cout << "Всего задач для генерации: " << taskCount << "\n";
    std::cout << "============================================================\n\n";

    RunBenchmark();

    // Узнаем путь к нашему собственному .exe файлу
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    std::vector<HANDLE> processes;

    std::cout << "[Master] 1. Запуск Collector...\n";
    processes.push_back(SpawnProcess(exePath, "collector", runId, taskCount));
    Sleep(500); // 0.5 сек, чтобы Collector успел создать серверный канал

    std::cout << "[Master] 2. Запуск Validator...\n";
    processes.push_back(SpawnProcess(exePath, "validator", runId, taskCount));
    Sleep(500); // время на создание каналов

    std::cout << "[Master] 3. Запуск Processor...\n";
    processes.push_back(SpawnProcess(exePath, "processor", runId, taskCount));
    Sleep(500); // время на подключение и создание входных каналов

    std::cout << "[Master] 4. Запуск Generator (старт потока данных!)...\n\n";
    processes.push_back(SpawnProcess(exePath, "generator", runId, taskCount));

    std::cout << "[Master] Все 4 процесса запущены в отдельных окнах!\n";
    std::cout << "[Master] Ожидание завершения работы конвейера...\n";

    // ждем, пока 4 процесса завершат свою работу
    WaitForMultipleObjects(
            static_cast<DWORD>(processes.size()),
            processes.data(),
            TRUE,
            INFINITE
    );

    std::cout << "\n[Master] Все процессы успешно завершили работу!\n";

    // закрываем дескрипторы процессов
    for (HANDLE hProc : processes) {
        CloseHandle(hProc);
    }

    std::cout << "[Master] Системные ресурсы освобождены. Программа завершена.\n";
    return 0;
}