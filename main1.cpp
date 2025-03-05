#include <iostream>
#include <vector>
#include <queue>
#include <sstream>

using namespace std;

// Process Control Block (PCB) Structure
struct PCB {
    int processID;
    string state;
    int programCounter;
    int instructionBase;
    int dataBase;
    int memoryLimit;
    int cpuCyclesUsed;
    int registerValue;
    int maxMemoryNeeded;
    int mainMemoryBase;
};

// Global variables
vector<int> mainMemory; // Simulated Main Memory
queue<PCB> jobQueue;
queue<int> readyQueue;

void loadJobsToMemory(int maxMemory) {
    int memoryPointer = 0; // Tracks memory allocation
    while (!jobQueue.empty()) {
        PCB process = jobQueue.front();
        jobQueue.pop();
        
        if (memoryPointer + 10 + process.maxMemoryNeeded > maxMemory) {
            cout << "Not enough memory for Process " << process.processID << "\n";
            continue;
        }

        process.mainMemoryBase = memoryPointer;
        process.instructionBase = memoryPointer + 10;
        process.dataBase = process.instructionBase + process.memoryLimit;
        
        mainMemory[memoryPointer] = process.processID;
        mainMemory[memoryPointer + 1] = 0; // State NEW (encoded as int)
        mainMemory[memoryPointer + 2] = 0; // Program Counter
        mainMemory[memoryPointer + 3] = process.instructionBase;
        mainMemory[memoryPointer + 4] = process.dataBase;
        mainMemory[memoryPointer + 5] = process.memoryLimit;
        mainMemory[memoryPointer + 6] = 0; // CPU Cycles Used
        mainMemory[memoryPointer + 7] = 0; // Register Value
        mainMemory[memoryPointer + 8] = process.maxMemoryNeeded;
        mainMemory[memoryPointer + 9] = process.mainMemoryBase;
        
        memoryPointer += 10 + process.maxMemoryNeeded;
        readyQueue.push(process.processID);
        cout << "Loaded Process " << process.processID << " into memory." << endl;
    }
}

void executeCPU() {
    while (!readyQueue.empty()) {
        int processID = readyQueue.front();
        readyQueue.pop();
        
        int baseAddress = -1;
        for (size_t i = 0; i < mainMemory.size(); i++) {
            if (mainMemory[i] == processID) {
                baseAddress = i;
                break;
            }
        }
        if (baseAddress == -1) continue;
        
        int instructionPointer = mainMemory[baseAddress + 3];
        int programCounter = mainMemory[baseAddress + 2];
        int memoryLimit = mainMemory[baseAddress + 5];
        
        cout << "Executing Process " << processID << "\n";
        
        while (programCounter < memoryLimit) {
            int opcode = mainMemory[instructionPointer + programCounter];
            if (opcode == 1) { // Compute
                int iterations = mainMemory[instructionPointer + programCounter + 1];
                int cycles = mainMemory[instructionPointer + programCounter + 2];
                cout << "Process " << processID << " executing COMPUTE for " << iterations << " iterations, " << cycles << " cycles." << endl;
                programCounter += 3;
            } else if (opcode == 2) { // Print
                int cycles = mainMemory[instructionPointer + programCounter + 1];
                cout << "Process " << processID << " executing PRINT operation, taking " << cycles << " cycles." << endl;
                programCounter += 2;
            } else if (opcode == 3) { // Store
                int value = mainMemory[instructionPointer + programCounter + 1];
                int address = mainMemory[instructionPointer + programCounter + 2];
                mainMemory[address] = value;
                cout << "Process " << processID << " STORED value " << value << " at memory address " << address << endl;
                programCounter += 3;
            } else if (opcode == 4) { // Load
                int address = mainMemory[instructionPointer + programCounter + 1];
                int value = mainMemory[address];
                cout << "Process " << processID << " LOADED value " << value << " from memory address " << address << endl;
                programCounter += 2;
            } else {
                cout << "Unknown instruction encountered." << endl;
                break;
            }
        }
        cout << "Process " << processID << " TERMINATED." << endl;
    }
}

void parseInput() {
    string line;
    getline(cin, line);
    int maxMemory = stoi(line);
    mainMemory.resize(maxMemory, -1);

    getline(cin, line);
    int numProcesses = stoi(line);
    
    while (getline(cin, line)) {
        stringstream ss(line);
        PCB process;
        ss >> process.processID >> process.maxMemoryNeeded >> process.memoryLimit;
        process.state = "NEW";
        process.programCounter = 0;
        process.cpuCyclesUsed = 0;
        process.registerValue = 0;
        jobQueue.push(process);
    }
    
    loadJobsToMemory(maxMemory);
}

int main() {
    parseInput();
    executeCPU();
    return 0;
}
