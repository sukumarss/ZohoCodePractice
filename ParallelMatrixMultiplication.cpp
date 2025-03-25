#include <iostream>
#include <vector>
#include <thread>

void multiplyRow(const std::vector<std::vector<int>>& A,
                 const std::vector<std::vector<int>>& B,
                 std::vector<std::vector<int>>& C,
                 int row) {
    int colsB = B[0].size();
    int colsA = A[0].size();
    for (int col = 0; col < colsB; ++col) {
        C[row][col] = 0;
        for (int k = 0; k < colsA; ++k) {
            C[row][col] += A[row][k] * B[k][col];
        }
    }
}

std::vector<std::vector<int>> parallelMatrixMultiply(const std::vector<std::vector<int>>& A,
                                                     const std::vector<std::vector<int>>& B) {
    int rowsA = A.size();
    int colsA = A[0].size();
    int colsB = B[0].size();

     if (colsA != B.size()) {
        throw std::invalid_argument("Matrix dimensions do not match for mul");
    }

    std::vector<std::vector<int>> C(rowsA, std::vector<int>(colsB, 0));

     std::vector<std::thread> threads;
    for (int i = 0; i < rowsA; ++i) {
        threads.emplace_back(multiplyRow, std::cref(A), std::cref(B), std::ref(C), i);
    }

    for (auto& th : threads) {
        th.join();
    }

    return C;
}

void printMatrix(const std::vector<std::vector<int>>& matrix) {
    for (const auto& row : matrix) {
        for (int val : row) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::vector<std::vector<int>> A = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };

    std::vector<std::vector<int>> B = {
        {9, 8, 7},
        {6, 5, 4},
        {3, 2, 1}
    };

    try {
        std::vector<std::vector<int>> C = parallelMatrixMultiply(A, B);
        std::cout << "Resultant Matrix:" << std::endl;
        printMatrix(C);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
