#include "Matrix.h"
#include <random>
#include <cmath>

Matrix GenerateRandomMatrix(int n) {
    Matrix m(n * n);

    // генератор случайных чисел
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(-10.0, 10.0);

    for (int i = 0; i < n * n; ++i) {
        m[i] = dist(gen);
    }
    return m;
}

void MultiplyMatrix(const Matrix& A, const Matrix& B, Matrix& C, int n) {
    std::fill(C.begin(), C.end(), 0.0);

    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < n; ++k) {
            double r = A[i * n + k];
            for (int j = 0; j < n; ++j) {
                C[i * n + j] += r * B[k * n + j];
            }
        }
    }
}

bool AreMatricesEqual(const Matrix& M1, const Matrix& M2, int n, double epsilon) {
    for (int i = 0; i < n * n; ++i) {
        if (std::fabs(M1[i] - M2[i]) > epsilon) {
            return false;
        }
    }
    return true;
}