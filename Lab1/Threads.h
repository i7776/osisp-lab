#pragma once
#include <windows.h>
#include "Matrix.h"

struct ThreadData {
    int threadNum;
    int startRow;          // c какой строки матрицы считать
    int endRow;            // по какую строку считать
    int n;                 // n * n
    const Matrix* A;       // указатель на матрицу A
    const Matrix* B;       // указатель на матрицу B
    Matrix* C;             // указатель на матрицу C
};

DWORD WINAPI WorkerThread(LPVOID lpParam);

// функция запускает N потоков, распределяет строки
void RunParallelMultiplication(const Matrix& A, const Matrix& B, Matrix& C, int n, int numThreads);