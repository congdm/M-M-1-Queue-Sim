// Sim.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <time.h>
#include <thread>
#include <string>
#include <intrin.h>

import SimCore;
import RNG;

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
    double lambda[8];
    double mu[8];
    double meanServTime, meanTimeInSys;
};

class Worker {
    int nDataPoint, nNode, nPacket;
    std::thread* threadObj;
    void run();
public:
    DataPoint* data;
    int id;

    Worker(int id, int nDataPoint, int nNode, int nPacket);
    void writeToFile(time_t startTime);
    void waitTillDone();
};

void Worker::writeToFile(time_t startTime) {
    std::string filename;
    std::ofstream out;
    int i, j;

    filename = "data_";
    filename.append(std::to_string(startTime));
    if (RNG::mode == RNG::MersenneTwister) filename.append("_mersennetwister");
    else if (RNG::mode == RNG::cpuRDRAND) filename.append("_rdrand");
    filename.append("_p"); filename.append(std::to_string(nPacket));
    filename.append("_n"); filename.append(std::to_string(nNode));
    filename.append("_w"); filename.append(std::to_string(id));
    filename.append(".json");

    out.open(filename);
    out << "{\n\"nData\": " << nDataPoint << ",\n";
    out << "\"nNode\": " << nNode << ",\n";

    for (i = 0; i < nNode; i++) {
        out << "\"lambda" << i << "\": [";
        for (j = 0; j < nDataPoint; j++) {
            out << data[j].lambda[i];
            if (j < nDataPoint-1) out << ", ";
        }
        out << "],\n";

        out << "\"mu" << i << "\": [";
        for (j = 0; j < nDataPoint; j++) {
            out << data[j].mu[i];
            if (j < nDataPoint-1) out << ", ";
        }
        out << "],\n";
    }

    out << "\"meanServTime\": [";
    for (i = 0; i < nDataPoint; i++) {
        out << data[i].meanServTime;
        if (i < nDataPoint-1) out << ", ";
    }
    out << "],\n";
    out << "\"meanTimeInSys\": [";
    for (int i = 0; i < nDataPoint; i++) {
        out << data[i].meanTimeInSys;
        if (i < nDataPoint-1) out << ", ";
    }
    out << "]\n";
    out << "}\n";
    out.close();
}

unsigned __int64 generateUniform(unsigned __int64 limit) {
    unsigned __int64 result;
    _rdrand64_step(&result);
    return (result % limit);
}

unsigned __int64 generateIrwinHall(unsigned __int64 limitUni, int n) {
    if (n <= 1) return generateUniform(limitUni);
    else return generateUniform(limitUni) + generateIrwinHall(limitUni, n - 1);
}

void Worker::run() {
    int i, j;
    double* lambda, * mu, sumLambda;
    int* queueLen;
    std::string msg;
    
    msg = "Worker "; msg = msg.append(std::to_string(id)); msg.append(" starts\n");
    std::cout << msg;

    lambda = new double[nNode]; mu = new double[nNode];
    data = new DataPoint[nDataPoint];
    for (i = 0; i < nDataPoint; i++) {
        sumLambda = 0.0;
        for (j = 0; j < nNode; j++) {
            do lambda[j] = generateUniform(16384);
            while (lambda[j] == 0.0);
            do mu[j] = generateIrwinHall(16384*2, j+1);
            while (mu[j] <= sumLambda + lambda[j]);
            sumLambda = sumLambda + lambda[j];
            data[i].lambda[j] = lambda[j] / 16384.0 * 16.0;
            data[i].mu[j] = mu[j] / 16384.0 * 16.0;
        }
    }

    SimCore::Sim sim;
    queueLen = new int[nNode];
    std::fill_n(queueLen, nNode, nPacket);
    double servTime, timeInSys;

    for (i = 0; i < nDataPoint; i++) {
        if (i % 1000 == 0) {
            msg = "Worker "; msg = msg.append(std::to_string(id));
            msg.append(" begins iteration "); msg = msg.append(std::to_string(i)); msg.append("\n");
            std::cout << msg;
        }

        sim.init(nNode, data[i].lambda, data[i].mu, queueLen);
        sim.runSim(nPacket);

        servTime = 0.0;
        timeInSys = 0.0;
        for (j = 0; j < sim.dataCnt; j++) {
            servTime = servTime + sim.servTimeData[j];
            timeInSys = timeInSys + sim.servTimeData[j] + sim.queueTimeData[j];
        }
        data[i].meanServTime = servTime / sim.dataCnt;
        data[i].meanTimeInSys = timeInSys / sim.dataCnt;

        sim.cleanUp();
    }

    delete lambda; delete mu; delete queueLen;
}

Worker::Worker(int id, int nDataPoint, int nNode, int nPacket) {
    this->id = id;
    this->nDataPoint = nDataPoint;
    this->nNode = nNode;
    this->nPacket = nPacket;
    threadObj = new std::thread(&Worker::run, this);
}

void Worker::waitTillDone() {
    threadObj->join();
}

int main()
{
    int nWorker = 1;
    int nData = 10000;
    int nNode = 1;
    int nPacket = 100000;
    time_t startTime, endTime;
    std::string str;

    int i;
    Worker** worker;

    std::cout << "List of PRNG:\n";
    std::cout << "1. C++ Standard Library Mersenne Twister\n";
    std::cout << "2. RDRAND instruction\n";
    std::cout << "Choose: ";
    std::cin >> str;
    if (str.compare("2") == 0) {
        RNG::mode = RNG::cpuRDRAND;
        std::cout << "PRNG is set to RDRAND\n";
    }
    else {
        std::cout << "PRNG is set to Mersenne Twister\n";
    }

    std::cout << "Number of workers: ";
    std::cin >> nWorker;
    std::cout << "Number of simulations each worker: ";
    std::cin >> nData;
    std::cout << "Number of nodes each simulation: ";
    std::cin >> nNode;
    std::cout << "Number of packets each simulation: ";
    std::cin >> nPacket;

    std::time(&startTime);
    worker = new Worker * [nWorker];
    for (i = 0; i < nWorker; i++) {
        worker[i] = new Worker(i, nData, nNode, nPacket);
    }
    for (i = 0; i < nWorker; i++) {
        worker[i]->waitTillDone();
    }
    std::time(&endTime);
    std::cout << "Running time: " << endTime - startTime << " seconds";

    for (i = 0; i < nWorker; i++) {
        worker[i]->writeToFile(startTime);
    }

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
