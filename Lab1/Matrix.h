#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <cmath>

using Matrix = std::vector<double>;

Matrix GenerateRandomMatrix(int n);

void MultiplyMatrix(const Matrix& A, const Matrix& B, Matrix& C, int n);

bool AreMatricesEqual(const Matrix& M1, const Matrix& M2, int n, double epsilon = 1e-4);
