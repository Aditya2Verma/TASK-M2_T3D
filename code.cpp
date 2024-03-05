#include <iostream>
#include <fstream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace std;
using namespace std::chrono;

struct TrafficData {
    time_point<system_clock> timestamp;
    int id;
    int passingCars;
};

queue<TrafficData> trafficQueue;
mutex queueMutex;
condition_variable conditionVariable;

void ProduceTrafficData(int signalCount) {
    while (true) {
        for (int i = 1; i <= signalCount; i++) {
            TrafficData data;
            data.timestamp = system_clock::now();
            data.id = i;
            data.passingCars = rand() % 500;

            unique_lock<mutex> lock(queueMutex);
            trafficQueue.push(data);

            conditionVariable.notify_one();
        }

        this_thread::sleep_for(chrono::minutes(5));
    }
}

void ConsumeTrafficData(int signalCount, int topCount) {
    while (true) {
        unique_lock<mutex> lock(queueMutex);

        conditionVariable.wait(lock, [] { return !trafficQueue.empty(); });

        vector<TrafficData> trafficData;

        while (!trafficQueue.empty()) {
            trafficData.push_back(trafficQueue.front());
            trafficQueue.pop();
        }

        lock.unlock();

        ofstream outputFile("TrafficData.txt", ios::app); 

        if (outputFile.is_open()) {
            outputFile << "Traffic Light Data:" << endl;

            for (const auto& data : trafficData) {
                time_t time = system_clock::to_time_t(data.timestamp);
                outputFile << "Timestamp: " << ctime(&time) << "| ID: " << data.id << "| Vehicles Passed: " << data.passingCars << endl;
            }
            outputFile << endl;

            sort(trafficData.begin(), trafficData.end(), [](const TrafficData& first, const TrafficData& second) {
                return first.passingCars > second.passingCars;
            });

            outputFile << "Top " << topCount << " congested traffic lights:" << endl;

            for (int i = 0; i < min(topCount, static_cast<int>(trafficData.size())); i++) {
                time_t time = system_clock::to_time_t(trafficData[i].timestamp);
                outputFile << "Timestamp: " << ctime(&time) << "| ID: " << trafficData[i].id << "| Vehicles Passed: " << trafficData[i].passingCars << endl;
            }

            outputFile.close();
        } else {
            cerr << "Unable to open the output file." << endl;
        }

        this_thread::sleep_for(chrono::hours(1));
    }
}

int main() {
    int signalCount = 10;  
    int topCount = 3;     

    thread producer(ProduceTrafficData, signalCount);
    thread consumer(ConsumeTrafficData, signalCount, topCount);

    producer.join();
    consumer.join();

    return 0;
}
