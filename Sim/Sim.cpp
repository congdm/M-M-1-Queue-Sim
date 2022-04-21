// Sim.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>

import SimCore;

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
    out << "]\n";

    out << "}\n";

    out.close();
}

int main()
{
    SimCore::Sim sim;
    double lambda[] = { 1.0 };
    double mu[] = { 2.0 };
    int queueLen[] = { 100 };

    sim.init(1, lambda, mu, queueLen);
    sim.runSim(1000);
    outputData(sim);
    sim.cleanUp();
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
