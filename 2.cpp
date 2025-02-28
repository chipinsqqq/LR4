#include <thread>
#include <iostream>
#include <vector>
#include <random>
#include <mutex>
#include <chrono>

using namespace std;

// Структура для представления даты
struct Date {
    int day;   // День месяца
    int month; // Месяц
    int year;  // Год
};

// Функция генерации случайных дат
vector<Date> generateDates(int size) {
    vector<Date> dates(size);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dayDist(1, 28);  // Генерируем день от 1 до 28 (чтобы избежать проблем с месяцами)
    uniform_int_distribution<> monthDist(1, 12); // Генерируем месяц от 1 до 12
    uniform_int_distribution<> yearDist(2000, 2025); // Генерируем год от 2000 до 2025

    for (int i = 0; i < size; ++i) {
        dates[i].day = dayDist(gen);
        dates[i].month = monthDist(gen);
        dates[i].year = yearDist(gen);
    }

    return dates;
}

// Проверка, находится ли дата в заданном диапазоне
bool isWithinRange(const Date& date, const Date& start, const Date& end) {
    return (date.year > start.year || (date.year == start.year && date.month > start.month) ||
            (date.year == start.year && date.month == start.month && date.day >= start.day)) &&
           (date.year < end.year || (date.year == end.year && date.month < end.month) ||
            (date.year == end.year && date.month == end.month && date.day <= end.day));
}

// Последовательная обработка дат (без многопоточности)
int processDates(const vector<Date>& dates, const Date& start, const Date& end) {
    int count = 0;
    for (const auto& date : dates) {
        if (isWithinRange(date, start, end)) {
            count++; // Подсчет количества дат, попадающих в диапазон
        }
    }
    return count;
}

mutex mtx; // Мьютекс для защиты общего счетчика в многопоточной обработке

// Обработка части массива дат в отдельном потоке
void processDatesThread(const vector<Date>& dates, const Date& start, const Date& end, int& totalCount, int startIdx, int endIdx) {
    int localCount = 0;
    for (int i = startIdx; i < endIdx; ++i) {
        if (isWithinRange(dates[i], start, end)) {
            localCount++; // Подсчет дат в заданном диапазоне
        }
    }
    lock_guard<mutex> lock(mtx); // Защищаем доступ к общему счетчику
    totalCount += localCount;
}

// Многопоточная обработка дат
int processDatesParallel(const vector<Date>& dates, const Date& start, const Date& end, int numThreads) {
    int size = dates.size();
    int chunkSize = size / numThreads; // Разделение массива на части для потоков
    vector<thread> threads;
    int totalCount = 0;

    // Запускаем потоки, каждый обрабатывает свою часть массива
    for (int i = 0; i < numThreads; ++i) {
        int startIdx = i * chunkSize;
        int endIdx = (i == numThreads - 1) ? size : startIdx + chunkSize;
        threads.emplace_back(processDatesThread, ref(dates), ref(start), ref(end), ref(totalCount), startIdx, endIdx);
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    return totalCount;
}

int main() {
    int size = 10000; // Размер массива дат
    int numThreads = 4; // Количество потоков
    Date start = {1, 1, 2010}; // Начальная дата диапазона
    Date end = {31, 12, 2020}; // Конечная дата диапазона

    vector<Date> dates = generateDates(size); // Генерация случайных дат

    // Последовательная обработка
    auto startTime = chrono::high_resolution_clock::now();
    int singleCount = processDates(dates, start, end);
    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double> singleTime = endTime - startTime;

    // Многопоточная обработка
    startTime = chrono::high_resolution_clock::now();
    int multiCount = processDatesParallel(dates, start, end, numThreads);
    endTime = chrono::high_resolution_clock::now();
    chrono::duration<double> multiTime = endTime - startTime;

    // Вывод результатов
    cout << "Обработка в однопоточном режиме: " << singleTime.count() << " секунд\n";
    cout << "Обработка в многопоточном режиме: " << multiTime.count() << " секунд\n";
    cout << "Количество дат в диапазоне (однопоточность): " << singleCount << "\n";
    cout << "Количество дат в диапазоне (многопоточность): " << multiCount << "\n";

    return 0;
}
