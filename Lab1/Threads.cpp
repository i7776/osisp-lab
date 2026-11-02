#include "Threads.h"
#include <iostream>
#include <vector>
#include "Utils.h"

// работа потока
DWORD WINAPI WorkerThread(LPVOID lpParam) {
    auto* data = (ThreadData*)lpParam;

    // узнаем системный ID потока
    DWORD tid = GetCurrentThreadId();

    std::cout << "[Поток #" << data->threadNum << " | TID: " << tid << "] "
              << "взял строки от " << data->startRow
              << " до " << data->endRow - 1 << std::endl;

    // ссылки на матрицы для быстрого доступа
    int n = data->n;
    const Matrix& A = *(data->A);
    const Matrix& B = *(data->B);
    Matrix& C = *(data->C);

    // считаем ТОЛЬКО строки от startRow до endRow
    for (int i = data->startRow; i < data->endRow; ++i) {
        for (int k = 0; k < n; ++k) {
            double r = A[i * n + k];
            for (int j = 0; j < n; ++j) {
                C[i * n + j] += r * B[k * n + j];
            }
        }
    }

    return 0;
}


// запуск потоков главным потоком
void RunParallelMultiplication(const Matrix& A, const Matrix& B, Matrix& C, int n, int numThreads) {

    std::fill(C.begin(), C.end(), 0.0);

    std::vector<HANDLE> threads(numThreads);
    std::vector<ThreadData> tasks(numThreads);

    // считаем сколько строк отдадим каждому
    int rowsPerThread = n / numThreads;
    int remainder = n % numThreads;
    int currentRow = 0;

    for (int i = 0; i < numThreads; ++i) {
        // строк отдать потоку
        int count = rowsPerThread + (i < remainder ? 1 : 0);

        tasks[i].threadNum = i + 1;
        tasks[i].startRow = currentRow;
        tasks[i].endRow = currentRow + count;
        tasks[i].n = n;
        tasks[i].A = &A;
        tasks[i].B = &B;
        tasks[i].C = &C;

        currentRow += count; // сдвигаем указатель строк для следующего потока

        // запускаем поток через WinAPI
        threads[i] = CreateThread(
                NULL,               // атрибуты безопасности
                0,
                WorkerThread,       // имя функции, которую надо выполнять
                &tasks[i],          // адрес коробки с заданием
                0,                  // запустить сразу
                NULL                // TID
        );

        if (threads[i] == NULL) {
            PrintError("CreateThread #" + std::to_string(i + 1));
            // закрываем уже созданные дескрипторы, если упали на середине
            for (int j = 0; j < i; ++j) CloseHandle(threads[j]);
            return;
        }

        // безопасный приоритет
        SetThreadPriority(threads[i], THREAD_PRIORITY_NORMAL);
    }


    WaitForMultipleObjects(
            numThreads,      // сколько потоков ждать
            threads.data(),  // массив дескрипторов
            TRUE,
            INFINITE         // без таймаута
    );

    for (int i = 0; i < numThreads; ++i) {
        DWORD exitCode = 0;
        // код завершения потока
        GetExitCodeThread(threads[i], &exitCode);
        std::cout << "[Поток #" << (i + 1) << "] завершился с кодом: " << exitCode << '\n';

        CloseHandle(threads[i]);   // закрываем
    }
}

