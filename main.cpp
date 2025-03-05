#include <iostream>
#include <queue>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

// Constants for process states
enum ProcessState {
    NEW = 0,
    READY = 1,
    RUNNING = 2,
    TERMINATED = 3,
    IOWAITING = 4
};

// Constants for instruction types
enum InstructionType {
    COMPUTE = 1,
    PRINT = 2,
    STORE = 3,
    LOAD = 4
};

// PCB structure to hold process information
struct PCB {
    int processID;
    ProcessState state;
    int programCounter;
    int instructionBase;
    int dataBase;
    int memoryLimit;
    int cpuCyclesUsed;
    int registerValue;
    int maxMemoryNeeded;
    int mainMemoryBase;
    int numInstructions;
    int startTime;      // Time when process first entered running state
    int endTime;        // Time when process terminated
    int ioReturnTime;   // Time when I/O operation will complete
};

// Global variables
int CPUAllocated;      // Max CPU time allocation before timeout
int contextSwitchTime; // Time to switch context
int globalCPUClock = 0; // Global CPU clock

// Function prototypes
void loadJobsToMemory(vector<PCB>& processes, vector<int>& mainMemory, queue<int>& readyQueue);
void executeCPU(int startAddress, vector<int>& mainMemory, queue<int>& readyQueue, queue<PCB>& ioWaitingQueue);
void checkIOWaitingQueue(queue<PCB>& ioWaitingQueue, queue<int>& readyQueue, vector<int>& mainMemory);
void printMainMemory(vector<int>& mainMemory);
string getStateString(ProcessState state);

int main() {
    int maxMemory;
    int numProcesses;
    vector<PCB> processes;
    queue<int> readyQueue;
    queue<PCB> ioWaitingQueue;
    
    // Step 1: Read and parse input file
    cin >> maxMemory >> CPUAllocated >> contextSwitchTime;
    cin >> numProcesses;
    
    vector<int> mainMemory(maxMemory, -1); // Initialize main memory with -1
    
    // Parse process information and instructions
    for (int i = 0; i < numProcesses; i++) {
        PCB process;
        process.processID = i + 1; // Process IDs start from 1
        process.state = NEW;
        process.programCounter = 0;
        process.cpuCyclesUsed = 0;
        process.registerValue = 0;
        process.startTime = -1;  // Not started yet
        process.endTime = -1;    // Not terminated yet
        
        cin >> process.maxMemoryNeeded >> process.numInstructions;
        
        // Store the process in our processes vector
        processes.push_back(process);
        
        // Skip the actual instructions for now, we'll parse them when loading into memory
        for (int j = 0; j < process.numInstructions; j++) {
            int instructionType;
            cin >> instructionType;
            
            if (instructionType == COMPUTE) {
                int param1, param2;
                cin >> param1 >> param2;
            } else if (instructionType == PRINT) {
                int param1;
                cin >> param1;
            } else if (instructionType == STORE) {
                int param1, param2;
                cin >> param1 >> param2;
            } else if (instructionType == LOAD) {
                int param1;
                cin >> param1;
            }
        }
    }
    
    // Load jobs into main memory
    loadJobsToMemory(processes, mainMemory, readyQueue);
    
    // Print the content of the main memory
    printMainMemory(mainMemory);
    
    // Process execution loop
    while (!readyQueue.empty() || !ioWaitingQueue.empty()) {
        // Check if any I/O operations have completed
        checkIOWaitingQueue(ioWaitingQueue, readyQueue, mainMemory);
        
        if (!readyQueue.empty()) {
            int startAddress = readyQueue.front();
            readyQueue.pop();
            
            // Execute job
            executeCPU(startAddress, mainMemory, readyQueue, ioWaitingQueue);
        } else if (!ioWaitingQueue.empty()) {
            // If only I/O jobs are running, increment CPU clock
            globalCPUClock += contextSwitchTime;
            
            // Check again if any I/O operations have completed
            checkIOWaitingQueue(ioWaitingQueue, readyQueue, mainMemory);
        }
    }
    
    // Print total CPU time used
    cout << "Total CPU time used: " << globalCPUClock << "." << endl;
    
    return 0;
}

// Load jobs into main memory and move them to ready queue
void loadJobsToMemory(vector<PCB>& processes, vector<int>& mainMemory, queue<int>& readyQueue) {
    int currentMemoryPosition = 0;
    
    for (PCB& process : processes) {
        // Check if there's enough memory for this process
        if (currentMemoryPosition + process.maxMemoryNeeded <= mainMemory.size()) {
            // Set memory locations for this process
            process.mainMemoryBase = currentMemoryPosition;
            process.instructionBase = currentMemoryPosition + 10; // PCB takes 10 spaces
            process.dataBase = process.instructionBase + (process.numInstructions * 3); // Each instruction takes 3 integers
            process.memoryLimit = process.maxMemoryNeeded;
            
            // Store PCB in main memory
            mainMemory[currentMemoryPosition] = process.processID;
            mainMemory[currentMemoryPosition + 1] = process.state;
            mainMemory[currentMemoryPosition + 2] = process.programCounter;
            mainMemory[currentMemoryPosition + 3] = process.instructionBase;
            mainMemory[currentMemoryPosition + 4] = process.dataBase;
            mainMemory[currentMemoryPosition + 5] = process.memoryLimit;
            mainMemory[currentMemoryPosition + 6] = process.cpuCyclesUsed;
            mainMemory[currentMemoryPosition + 7] = process.registerValue;
            mainMemory[currentMemoryPosition + 8] = process.maxMemoryNeeded;
            mainMemory[currentMemoryPosition + 9] = process.mainMemoryBase;
            
            // Seek back to beginning of this process's instructions
            cin.clear();
            cin.seekg(0, ios::beg);
            
            // Skip the first few lines of input
            int dummy;
            cin >> dummy >> dummy >> dummy; // Skip maxMemory, CPUAllocated, contextSwitchTime
            cin >> dummy; // Skip numProcesses
            
            // Skip to this process's instructions
            for (int i = 0; i < process.processID - 1; i++) {
                int maxMem, numInstr;
                cin >> maxMem >> numInstr;
                
                for (int j = 0; j < numInstr; j++) {
                    int instrType;
                    cin >> instrType;
                    
                    if (instrType == COMPUTE) {
                        int param1, param2;
                        cin >> param1 >> param2;
                    } else if (instrType == PRINT) {
                        int param1;
                        cin >> param1;
                    } else if (instrType == STORE) {
                        int param1, param2;
                        cin >> param1 >> param2;
                    } else if (instrType == LOAD) {
                        int param1;
                        cin >> param1;
                    }
                }
            }
            
            // Now read this process's instructions
            int maxMem, numInstr;
            cin >> maxMem >> numInstr;
            
            // Read and store instructions in main memory
            for (int j = 0; j < process.numInstructions; j++) {
                int instrType;
                cin >> instrType;
                
                mainMemory[process.instructionBase + (j * 3)] = instrType;
                
                if (instrType == COMPUTE) {
                    int param1, param2;
                    cin >> param1 >> param2;
                    mainMemory[process.instructionBase + (j * 3) + 1] = param1;
                    mainMemory[process.instructionBase + (j * 3) + 2] = param2;
                } else if (instrType == PRINT) {
                    int param1;
                    cin >> param1;
                    mainMemory[process.instructionBase + (j * 3) + 1] = param1;
                    mainMemory[process.instructionBase + (j * 3) + 2] = 0; // Dummy value
                } else if (instrType == STORE) {
                    int param1, param2;
                    cin >> param1 >> param2;
                    mainMemory[process.instructionBase + (j * 3) + 1] = param1;
                    mainMemory[process.instructionBase + (j * 3) + 2] = param2;
                } else if (instrType == LOAD) {
                    int param1;
                    cin >> param1;
                    mainMemory[process.instructionBase + (j * 3) + 1] = param1;
                    mainMemory[process.instructionBase + (j * 3) + 2] = 0; // Dummy value
                }
            }
            
            // Initialize data segment if needed
            for (int i = process.dataBase; i < process.mainMemoryBase + process.maxMemoryNeeded; i++) {
                if (i < mainMemory.size()) {
                    mainMemory[i] = 0; // Initialize data to 0
                }
            }
            
            // Set process state to READY and update in memory
            process.state = READY;
            mainMemory[process.mainMemoryBase + 1] = READY;
            
            // Add process to ready queue
            readyQueue.push(process.mainMemoryBase);
            
            // Update memory position for next process
            currentMemoryPosition += process.maxMemoryNeeded;
        } else {
            // Not enough memory for this process
            cout << "Not enough memory for process " << process.processID << endl;
        }
    }
}

// Execute instructions for a process
void executeCPU(int startAddress, vector<int>& mainMemory, queue<int>& readyQueue, queue<PCB>& ioWaitingQueue) {
    PCB process;
    
    // Extract PCB from main memory
    process.processID = mainMemory[startAddress];
    process.state = static_cast<ProcessState>(mainMemory[startAddress + 1]);
    process.programCounter = mainMemory[startAddress + 2];
    process.instructionBase = mainMemory[startAddress + 3];
    process.dataBase = mainMemory[startAddress + 4];
    process.memoryLimit = mainMemory[startAddress + 5];
    process.cpuCyclesUsed = mainMemory[startAddress + 6];
    process.registerValue = mainMemory[startAddress + 7];
    process.maxMemoryNeeded = mainMemory[startAddress + 8];
    process.mainMemoryBase = mainMemory[startAddress + 9];
    
    // Calculate the number of instructions based on dataBase - instructionBase
    process.numInstructions = (process.dataBase - process.instructionBase) / 3; // Each instruction takes 3 integers
    
    // Set process state to RUNNING
    process.state = RUNNING;
    mainMemory[startAddress + 1] = RUNNING;
    
    // Record start time if this is the first time the process is running
    if (process.cpuCyclesUsed == 0) {
        process.startTime = globalCPUClock;
    }
    
    cout << "Process " << process.processID << " has moved to Running." << endl;
    
    // Add context switch time
    globalCPUClock += contextSwitchTime;
    
    // Execute instructions until timeout, I/O operation, or completion
    int currentCPUTime = 0;
    bool terminated = false;
    
    while (process.programCounter < process.numInstructions && currentCPUTime < CPUAllocated && !terminated) {
        // Calculate memory address of current instruction
        int instructionAddress = process.instructionBase + (process.programCounter * 3);
        
        // Get instruction type
        int instructionType = mainMemory[instructionAddress];
        
        // Execute instruction based on type
        if (instructionType == COMPUTE) {
            int iterations = mainMemory[instructionAddress + 1];
            int cycles = mainMemory[instructionAddress + 2];
            int totalCycles = iterations * cycles;
            
            cout << "compute" << endl;
            
            // Update CPU cycles used
            process.cpuCyclesUsed += totalCycles;
            mainMemory[startAddress + 6] = process.cpuCyclesUsed;
            
            // Update current CPU time
            currentCPUTime += totalCycles;
            
            // Update global CPU clock
            globalCPUClock += totalCycles;
            
            // Increment program counter
            process.programCounter++;
            mainMemory[startAddress + 2] = process.programCounter;
        } else if (instructionType == PRINT) {
            int cycles = mainMemory[instructionAddress + 1];
            
            cout << "Process " << process.processID << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
            
            // Update CPU cycles used
            process.cpuCyclesUsed += 1;  // Count 1 cycle for initiating the I/O
            mainMemory[startAddress + 6] = process.cpuCyclesUsed;
            
            // Update global CPU clock
            globalCPUClock += 1;
            
            // Set state to IOWAITING
            process.state = IOWAITING;
            mainMemory[startAddress + 1] = IOWAITING;
            
            // Set I/O return time
            process.ioReturnTime = globalCPUClock + cycles;
            
            // Increment program counter
            process.programCounter++;
            mainMemory[startAddress + 2] = process.programCounter;
            
            // Move process to I/O waiting queue
            ioWaitingQueue.push(process);
            
            // Break execution to handle I/O
            return;
        } else if (instructionType == STORE) {
            int value = mainMemory[instructionAddress + 1];
            int address = mainMemory[instructionAddress + 2];
            
            // Calculate the actual memory address
            int actualAddress = process.dataBase + address;
            
            // Check if address is within bounds
            if (address >= 0 && actualAddress < process.mainMemoryBase + process.maxMemoryNeeded) {
                mainMemory[actualAddress] = value;
                cout << "stored" << endl;
                
                // Update register value
                process.registerValue = value;
                mainMemory[startAddress + 7] = value;
            } else {
                cout << "store error!" << endl;
            }
            
            // Update CPU cycles used
            process.cpuCyclesUsed += 1; // Store takes 1 CPU cycle
            mainMemory[startAddress + 6] = process.cpuCyclesUsed;
            
            // Update current CPU time
            currentCPUTime += 1;
            
            // Update global CPU clock
            globalCPUClock += 1;
            
            // Increment program counter
            process.programCounter++;
            mainMemory[startAddress + 2] = process.programCounter;
        } else if (instructionType == LOAD) {
            int address = mainMemory[instructionAddress + 1];
            
            // Calculate the actual memory address
            int actualAddress = process.dataBase + address;
            
            // Check if address is within bounds
            if (address >= 0 && actualAddress < process.mainMemoryBase + process.maxMemoryNeeded) {
                int value = mainMemory[actualAddress];
                cout << "loaded" << endl;
                
                // Update register value
                process.registerValue = value;
                mainMemory[startAddress + 7] = value;
            } else {
                cout << "load error!" << endl;
            }
            
            // Update CPU cycles used
            process.cpuCyclesUsed += 1; // Load takes 1 CPU cycle
            mainMemory[startAddress + 6] = process.cpuCyclesUsed;
            
            // Update current CPU time
            currentCPUTime += 1;
            
            // Update global CPU clock
            globalCPUClock += 1;
            
            // Increment program counter
            process.programCounter++;
            mainMemory[startAddress + 2] = process.programCounter;
        }
        
        // Check if process has completed all instructions
        if (process.programCounter >= process.numInstructions) {
            // Set process state to TERMINATED
            process.state = TERMINATED;
            mainMemory[startAddress + 1] = TERMINATED;
            
            // Record end time
            process.endTime = globalCPUClock;
            
            // Output PCB details
            cout << "Process ID: " << process.processID << endl;
            cout << "State: " << getStateString(process.state) << endl;
            cout << "Program Counter: " << process.programCounter << endl;
            cout << "Instruction Base: " << process.instructionBase << endl;
            cout << "Data Base: " << process.dataBase << endl;
            cout << "Memory Limit: " << process.memoryLimit << endl;
            cout << "CPU Cycles Used: " << process.cpuCyclesUsed << endl;
            cout << "Register Value: " << process.registerValue << endl;
            cout << "Max Memory Needed: " << process.maxMemoryNeeded << endl;
            cout << "Main Memory Base: " << process.mainMemoryBase << endl;
            cout << "Total CPU Cycles Consumed: " << process.cpuCyclesUsed << endl;
            
            // Output termination message with execution times
            cout << "Process " << process.processID << " terminated. Entered running state at: "
                 << process.startTime << ". Terminated at: " << process.endTime
                 << ". Total Execution Time: " << (process.endTime - process.startTime) << "." << endl;
            
            terminated = true;
        }
    }
    
    // If process timed out, move it back to the ready queue
    if (!terminated && currentCPUTime >= CPUAllocated) {
        cout << "Process " << process.processID << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
        
        // Set process state back to READY
        process.state = READY;
        mainMemory[startAddress + 1] = READY;
        
        // Add process back to ready queue
        readyQueue.push(startAddress);
    }
}

// Check if any I/O operations have completed
void checkIOWaitingQueue(queue<PCB>& ioWaitingQueue, queue<int>& readyQueue, vector<int>& mainMemory) {
    queue<PCB> tempQueue;
    
    while (!ioWaitingQueue.empty()) {
        PCB process = ioWaitingQueue.front();
        ioWaitingQueue.pop();
        
        // Check if I/O operation has completed
        if (globalCPUClock >= process.ioReturnTime) {
            cout << "Process " << process.processID << " completed I/O and is moved to the ReadyQueue." << endl;
            
            // Update process state in main memory
            process.state = READY;
            mainMemory[process.mainMemoryBase + 1] = READY;
            
            // Add process to ready queue
            readyQueue.push(process.mainMemoryBase);
            
            cout << "print" << endl;
        } else {
            // I/O operation not completed, keep process in I/O waiting queue
            tempQueue.push(process);
        }
    }
    
    // Restore I/O waiting queue
    ioWaitingQueue = tempQueue;
}

// Print the content of main memory
void printMainMemory(vector<int>& mainMemory) {
    for (int i = 0; i < mainMemory.size(); i++) {
        cout << i << " : " << mainMemory[i] << endl;
    }
}

// Convert ProcessState enum to string
string getStateString(ProcessState state) {
    switch (state) {
        case NEW:
            return "NEW";
        case READY:
            return "READY";
        case RUNNING:
            return "RUNNING";
        case TERMINATED:
            return "TERMINATED";
        case IOWAITING:
            return "IOWAITING";
        default:
            return "UNKNOWN";
    }
}