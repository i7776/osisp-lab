#pragma once
#include <windows.h>
#include <vector>
#include <numeric>
#include <iostream>

// индивидуальные параметры
const int STUDENT_ID = 72;
const int ARRAY_SIZE = 100;
const int INITIAL_VALUE = 10;
const int EXPECTED_SUM = ARRAY_SIZE * INITIAL_VALUE; // 100 * 10 = 1000

// индивидуальные формулы
const int SEED_BASE       = 1000 + STUDENT_ID * 97; // 7984
const int OPERATIONS_COUNT = 200 + STUDENT_ID * 5;  // 560 операций на поток

// режимы блокировки писателей
enum LockStrategy {
    LOCK_GROUP = 0, // блокировка на всю группу переносов
    LOCK_EACH  = 1  // блокировка отдельно на каждый перенос
};

// конфигурация эксперимента
struct ExperimentConfig {
    LockStrategy strategy; // LOCK_GROUP или LOCK_EACH
    int groupSize;         // 5 или 20
    int readersCount = 3;
    int writersCount = 2;
};

// общий ресурс писателей и читателей
struct SharedArray {
    int data[ARRAY_SIZE];
    SRWLOCK lock; // системный замок

    void Initialize() {
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            data[i] = INITIAL_VALUE;
        }
        // инициализация замка
        InitializeSRWLock(&lock);
    }
};

// статистика конкретного читателя
struct ReaderStats {
    long long totalReadOps = 0;        // сколько раз прочитал
    double totalWaitTimeMs = 0.0;      // время ожидания замка в мс
    int invariantViolations = 0;       // сколько раз сумма не была равна 1000 (ошибки)
};

// класс высокоточного таймера
class PrecisionTimer {
    long long freq = 0;
    long long start = 0;
    long long end = 0;

public:
    PrecisionTimer() {
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    }

    void Start() {
        QueryPerformanceCounter((LARGE_INTEGER*)&start);
    }

    void Stop() {
        QueryPerformanceCounter((LARGE_INTEGER*)&end);
    }

    // возвращает время в микросекундах
    double GetMicroseconds() const {
        return static_cast<double>(end - start) * 1000000.0 / static_cast<double>(freq);
    }

    // возвращает время в миллисекундах
    double GetMilliseconds() const {
        return static_cast<double>(end - start) * 1000.0 / static_cast<double>(freq);
    }
};