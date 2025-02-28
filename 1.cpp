#include <thread>
#include <iostream>
#include <mutex>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <condition_variable>
#include <atomic>

using namespace std;

mutex mtx; // Глобальный мьютекс для защиты ресурсов
int num_th = 100; // Количество потоков

// Реализация семафора (Semaphore) с использованием мьютекса и условной переменной
class Semaphore {
public:
    Semaphore(int count = 0) : count(count) {}

    void acquire() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return count > 0; });
        --count;
    }

    void release() {
        unique_lock<mutex> lock(mtx);
        ++count;
        cv.notify_one();
    }

private:
    mutex mtx;
    condition_variable cv;
    int count;
};

// Реализация семафора (SlimSemaphore) с использованием мьютекса и условной переменной
class SlimSemaphore {
public:
    SlimSemaphore(int count = 0) : count(count) {}

    void acquire() {
        unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return count > 0; }); // Ожидание доступного ресурса
        --count;
    }

    void release() {
        unique_lock<mutex> lock(mtx);
        ++count;
        cv.notify_one(); // Уведомление ожидающего потока
    }

private:
    mutex mtx;
    condition_variable cv;
    int count;
};

// Реализация барьера (SimpleBarrier) для синхронизации потоков
class SimpleBarrier {
private:
    mutex mtx;
    condition_variable cv;
    int count;
    int original;

public:
    SimpleBarrier(int num_threads) : count(num_threads), original(num_threads) {}

    void arrive_and_wait() {
        unique_lock<mutex> lock(mtx);
        if (--count == 0) {
            count = original;
            cv.notify_all(); // Разблокировка всех потоков
        } else {
            cv.wait(lock, [this]() { return count == original; }); // Ожидание завершения всех потоков
        }
    }
};

// Использование мьютекса для синхронизации вывода
void t_mutex() {
    static mutex mtx;
    lock_guard<mutex> lock(mtx);
    cout << char(33 + rand() % 90) << ' ';
}

// Использование Semaphore для управления доступом к ресурсу
void t_semaphore() {
    static Semaphore sem(1);
    sem.acquire();
    cout << char(33 + rand() % 90) << ' ';
    sem.release();
}

// Использование SlimSemaphore для управления доступом к ресурсу
void t_slim_semaphore() {
    static SlimSemaphore sem(1);
    sem.acquire();
    cout << char(33 + rand() % 90) << ' ';
    sem.release();
}

// Реализация SpinLock на основе атомарной переменной
void t_spinlock() {
    static atomic<bool> spinLock(false);
    while (spinLock.exchange(true)) {}
    cout << char(33 + rand() % 90) << ' ';
    spinLock.store(false);
}

// Использование SpinWait с передачей управления процессору
void t_spinwait() {
    static atomic<bool> spinWait(false);
    while (spinWait.exchange(true)) {
        this_thread::yield(); // Освобождение процессорного времени
    }
    cout << char(33 + rand() % 90) << ' ';
    spinWait.store(false);
}

// Использование SimpleBarrier для синхронизации потоков
void t_barrier() {
    static SimpleBarrier Barrier(num_th);
    Barrier.arrive_and_wait(); // Ждем, пока все потоки дойдут до барьера
    {
        lock_guard<mutex> lock(mtx);
        cout << char(33 + rand() % 90) << ' ';
    }
}

// Использование condition_variable для синхронизации потоков
void t_monitor() {
    static condition_variable cv;
    static mutex monitor_mtx;
    static bool ready = false;
    
    {
        lock_guard<mutex> lock(monitor_mtx);
        ready = true; // Устанавливаем флаг перед запуском
        cv.notify_all(); // Уведомляем все потоки
    }

    {
        unique_lock<mutex> lock(monitor_mtx);
        cv.wait(lock, []{ return ready; }); // Ожидание сигнала
    }

    {
        lock_guard<mutex> lock(mtx);
        cout << char(33 + rand() % 90) << ' ';
    }
}

// Запуск потоков и замер времени выполнения
void runThreads(void (*threadFunction)(), const string& PrimoName) {
    vector<thread> threads;
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_th; ++i) {
        threads.emplace_back(threadFunction);
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> restime = end - start;
    cout << "\nВремя для " << PrimoName << ": " << restime.count() << " секунд\n";
}

// Основная программа
int main() {
    srand(time(0));
    runThreads(t_mutex, "Mutex");
    runThreads(t_semaphore, "Semaphore");
    runThreads(t_slim_semaphore, "Slim semaphore");
    runThreads(t_spinlock, "SpinLock");
    runThreads(t_spinwait, "SpinWait");
    runThreads(t_barrier, "Barrier");
    runThreads(t_monitor, "Monitor");
    return 0;
}
