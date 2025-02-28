#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>

using namespace std;

mutex mtx; // Мьютекс для синхронизации потоков
condition_variable cv; // Условная переменная для управления доступом к ресурсам

int processes = 5, resources = 3;
// Матрица выделенных ресурсов (сколько уже выделено каждому процессу)
vector<vector<int>> allocation = {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}};
// Матрица максимальных потребностей (сколько всего может потребоваться процессу)
vector<vector<int>> maximumNeed = {{7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}};
// Доступные ресурсы в системе
vector<int> available = {3, 3, 2};
bool systemSafe = true; // Флаг безопасного состояния системы

// Функция проверки безопасного состояния системы
bool isSafeState() {
    vector<int> work = available; // Копируем доступные ресурсы
    vector<bool> finish(processes, false); // Вектор отслеживания завершенных процессов
    vector<int> safeSequence;

    int count = 0;
    while (count < processes) {
        bool found = false;
        for (int i = 0; i < processes; i++) {
            if (!finish[i]) {
                bool canAllocate = true;
                for (int j = 0; j < resources; j++) {
                    if (maximumNeed[i][j] - allocation[i][j] > work[j]) {
                        canAllocate = false;
                        break;
                    }
                }
                if (canAllocate) {
                    for (int j = 0; j < resources; j++) {
                        work[j] += allocation[i][j];
                    }
                    safeSequence.push_back(i);
                    finish[i] = true;
                    found = true;
                    count++;
                }
            }
        }
        if (!found) {
            cout << "Система находится в небезопасном состоянии!" << endl;
            return false;
        }
    }
    
    cout << "Система в безопасном состоянии. Последовательность выполнения: ";
    for (int i : safeSequence) {
        cout << "P" << i << " ";
    }
    cout << endl;
    return true;
}

// Функция обработки запроса ресурса процессом
void processRequest(int processID, vector<int> request) {
    unique_lock<mutex> lock(mtx);
    
    cout << "Процесс P" << processID << " запрашивает ресурсы: ";
    for (int r : request) cout << r << " ";
    cout << endl;

    // Проверяем возможность удовлетворения запроса
    for (int i = 0; i < resources; i++) {
        if (request[i] > maximumNeed[processID][i] - allocation[processID][i] || request[i] > available[i]) {
            cout << "Запрос процесса P" << processID << " отклонен (недостаточно ресурсов)." << endl;
            return;
        }
    }
    
    // Выделяем ресурсы процессу
    for (int i = 0; i < resources; i++) {
        available[i] -= request[i];
        allocation[processID][i] += request[i];
    }
    
    // Проверяем безопасность состояния после выделения ресурсов
    bool wasSafe = isSafeState();
    
    // Если система не в безопасном состоянии, откатываем выделенные ресурсы
    if (!wasSafe) {
        for (int i = 0; i < resources; i++) {
            available[i] += request[i];
            allocation[processID][i] -= request[i];
        }
        cout << "Запрос процесса P" << processID << " отклонен для сохранения безопасности системы." << endl;
    } else {
        cout << "Запрос процесса P" << processID << " выполнен успешно." << endl;
        systemSafe = true;
    }
    
    // Выводим текущее состояние доступных ресурсов
    cout << "Доступные ресурсы после обработки запроса: ";
    for (int r : available) cout << r << " ";
    cout << endl;
    
    cv.notify_all(); // Уведомляем другие потоки
}

int main() {
    vector<thread> threads;
    vector<vector<int>> requests = {{1, 0, 2}, {0, 2, 0}, {3, 0, 3}, {2, 1, 1}, {0, 0, 2}};
    
    cout << "Начальное состояние системы:" << endl;
    cout << "Доступные ресурсы: ";
    for (int r : available) cout << r << " ";
    cout << endl;
    isSafeState();
    
    // Запускаем потоки, моделирующие процессы, запрашивающие ресурсы
    for (int i = 0; i < processes; ++i) {
        threads.emplace_back(processRequest, i, requests[i]);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    cout << "Итоговое состояние доступных ресурсов: ";
    for (int r : available) cout << r << " ";
    cout << endl;
    
    return 0;
}
