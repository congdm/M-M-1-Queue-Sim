// Sim.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <random>
#include <time.h>
#include <thread>

import SimCore;

/*
void outputData(SimCore::Sim& sim) {
    std::ofstream out;
    out.open("output.json");
    out << "{\n";
    out << "\"dataCnt\": " << sim.dataCnt << ",\n";

    out << "\"servTimeData\": [";
    for (int i = 0; i < sim.dataCnt; i++) {
        out << sim.servTimeData[i];
        if (i < sim.dataCnt - 1) out << ", ";
    }
    out << "],\n";

    out << "\"queueTimeData\": [";
    for (int i = 0; i < sim.dataCnt; i++) {
        out << sim.queueTimeData[i];
        if (i < sim.dataCnt - 1) out << ", ";
    }
    out << "],\n";

    out << "\"droppedPktData\": [";
    for (int i = 0; i < sim.dataCnt; i++) {
        if (sim.droppedPktData[i]) out << "true";
        else out << "false";
        if (i < sim.dataCnt - 1) out << ", ";
    }
    out << "],\n";

    out << "\"pktOriginData\": [";
    for (int i = 0; i < sim.dataCnt; i++) {
        out << sim.pktOriginData[i];
        if (i < sim.dataCnt - 1) out << ", ";
    }
    out << "]\n";

    out << "}\n";

    out.close();
}

void testRun() {
    SimCore::Sim sim;
    double lambda[] = { 1.0, 1.5 };
    double mu[] = { 2.0, 4.0 };
    int queueLen[] = { 10000, 10000 };

    sim.init(2, lambda, mu, queueLen);
    sim.runSim(10000);
    outputData(sim);
    sim.cleanUp();
}
*/

class DataPoint {
public:
    double lambda[2];
    double mu[2];
    double meanServTime, meanTimeInSys;
};

class Worker {
    int nDataPoint, nRound;
    std::thread* threadObj;
    void run();
public:
    DataPoint* data;

    Worker(int nDataPoint, int nRound);
    void waitTillDone();
};

void Worker::run() {
    std::mt19937* generator;
    std::hash<std::thread::id> hasher;
    time_t now = time(nullptr);
    unsigned int seed = now + hasher(std::this_thread::get_id());
    generator = new std::mt19937(seed);
    std::uniform_real_distribution<> distLambda(0.0, 8.0);
    std::uniform_real_distribution<> distMu(0.0, 10.0);

    int i;
    data = new DataPoint[nDataPoint];
    for (i = 0; i < nDataPoint; i++) {
        data[i].lambda[0] = distLambda(*generator);
        double mu = 0.0;
        while (mu < data[i].lambda[0])
            mu = distMu(*generator);
        data[i].mu[0] = mu;

        data[i].lambda[1] = distLambda(*generator);
        mu = 0.0;
        while (mu < data[i].lambda[0] + data[i].lambda[1])
            mu = distMu(*generator) + distMu(*generator);
        data[i].mu[1] = mu;
    }

    SimCore::Sim sim;
    double lambda[2], mu[2];
    int queueLen[] = { nRound, nRound };
    double servTime, timeInSys;
    for (i = 0; i < nDataPoint; i++) {
        lambda[0] = data[i].lambda[0];
        lambda[1] = data[i].lambda[1];
        mu[0] = data[i].mu[0];
        mu[1] = data[i].mu[1];

        sim.init(2, lambda, mu, queueLen);
        sim.runSim(nRound);

        servTime = 0.0;
        timeInSys = 0.0;
        for (int j = 0; j < sim.dataCnt; j++) {
            servTime = servTime + sim.servTimeData[j];
            timeInSys = timeInSys + sim.servTimeData[j] + sim.queueTimeData[j];
        }
        data[i].meanServTime = servTime / sim.dataCnt;
        data[i].meanTimeInSys = timeInSys / sim.dataCnt;

        sim.cleanUp();
    }
}

Worker::Worker(int nDataPoint, int nRound) {
    this->nDataPoint = nDataPoint;
    this->nRound = nRound;
    threadObj = new std::thread(&Worker::run, this);
}

void Worker::waitTillDone() {
    threadObj->join();
}

int main()
{
    int cnt = 100;
    Worker wk1(cnt, 10000);
    wk1.waitTillDone();

    std::ofstream out;
    out.open("data.json");
    out << "{\n\"cnt\": " << cnt << ",\n";

    out << "\"lambda0\": [";
    for (int i = 0; i < cnt; i++) {
        out << wk1.data[i].lambda[0];
        if (i < cnt - 1) out << ", ";
    }
    out << "],\n";
    out << "\"mu0\": [";
    for (int i = 0; i < cnt; i++) {
        out << wk1.data[i].mu[0];
        if (i < cnt - 1) out << ", ";
    }
    out << "],\n";

    out << "\"lambda1\": [";
    for (int i = 0; i < cnt; i++) {
        out << wk1.data[i].lambda[1];
        if (i < cnt - 1) out << ", ";
    }
    out << "],\n";
    out << "\"mu1\": [";
    for (int i = 0; i < cnt; i++) {
        out << wk1.data[i].mu[1];
        if (i < cnt - 1) out << ", ";
    }
    out << "],\n";

    out << "\"meanServTime\": [";
    for (int i = 0; i < cnt; i++) {
        out << wk1.data[i].meanServTime;
        if (i < cnt - 1) out << ", ";
    }
    out << "],\n";
    out << "\"meanTimeInSys\": [";
    for (int i = 0; i < cnt; i++) {
        out << wk1.data[i].meanTimeInSys;
        if (i < cnt - 1) out << ", ";
    }
    out << "]\n";

    out << "}\n";
    out.close();

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
