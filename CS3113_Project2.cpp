#include <iostream>
#include <queue>
#include <vector>
#include <map>
using namespace std;

// Process states
enum ProcessState { 
    NEW = 1,          // Just created
    READY = 2,        // Ready to execute
    RUNNING = 3,      // Currently executing 
    IOWAITING = 4,    // Waiting for I/O
    TERMINATED = 5    // Finished execution
};

// Process Control Block - stores all process information
struct PCB {
    // Basic information
    int processID;
    int state;
    int programCounter;      // Current instruction
    int instructionBase;     // Where instructions start in memory
    int dataBase;            // Where data starts in memory
    int memoryLimit;         // Total memory allocated
    int cpuCyclesUsed;       // Total CPU time used
    int registerValue;       // Current register value
    int maxMemoryNeeded;     // Total memory needed
    int mainMemoryBase;      // Start address in main memory
    
    // For simulation
    vector<vector<int>> instructions;  // Process instructions
    int currentInstruction;            // Current instruction index
    int remainingInstructions;         // Instructions left to execute
    int startRunningTime;              // When process first started
    int terminationTime;               // When process finished
    int ioReleaseTime;                 // When I/O will complete
    int CPUAllocated;                  // CPU time allocation
    bool pendingPrint;                 // Track if a print operation is pending
    
    // Constructor with defaults
    PCB() : processID(0), state(NEW), programCounter(0), 
            instructionBase(0), dataBase(0), memoryLimit(0), 
            cpuCyclesUsed(0), registerValue(0), maxMemoryNeeded(0),
            mainMemoryBase(0), currentInstruction(0), 
            remainingInstructions(0), startRunningTime(-1),
            terminationTime(0), ioReleaseTime(0), CPUAllocated(0),
            pendingPrint(false) {}
};

// Check I/O waiting queue for completed I/O operations
void checkIOQueue(int currentTime, queue<PCB*>& ioQueue, queue<PCB*>& readyQueue) {
    queue<PCB*> tempQueue;
    
    // Check each process in I/O queue
    while (!ioQueue.empty()) {
        PCB* process = ioQueue.front();
        ioQueue.pop();
        
        // If I/O is complete
        if (currentTime >= process->ioReleaseTime) {
            cout << "print" << endl;
            cout << "Process " << process->processID 
                 << " completed I/O and is moved to the ReadyQueue." << endl;
            
            // Change state and move to ready queue
            process->state = READY;
            readyQueue.push(process);
        } else {
            // Still waiting for I/O
            tempQueue.push(process);
        }
    }
    
    // Restore remaining processes to I/O queue
    while (!tempQueue.empty()) {
        ioQueue.push(tempQueue.front());
        tempQueue.pop();
    }
}

// Load processes into memory
void loadMemory(vector<PCB*>& processes, vector<int>& memory, int memorySize) {
    // Initialize memory with -1
    for (int i = 0; i < memorySize; i++) {
        memory.push_back(-1);
    }
    
    // Load each process
    for (PCB* proc : processes) {
        int base = proc->mainMemoryBase;
        
        // Store PCB header in memory
        memory[base] = proc->processID;
        memory[base + 1] = proc->state;
        memory[base + 2] = proc->programCounter;
        memory[base + 3] = proc->instructionBase;
        memory[base + 4] = proc->dataBase;
        memory[base + 5] = proc->memoryLimit;
        memory[base + 6] = proc->cpuCyclesUsed;
        memory[base + 7] = proc->registerValue;
        memory[base + 8] = proc->maxMemoryNeeded;
        memory[base + 9] = proc->mainMemoryBase;
        
        // Store instructions - just store opcodes in instruction section
        int instrAddr = proc->instructionBase;
        for (const auto& instr : proc->instructions) {
            memory[instrAddr++] = instr[0]; // Opcode only
        }
        
        // Store data separately - all parameters
        int dataAddr = proc->dataBase;
        for (const auto& instr : proc->instructions) {
            for (size_t i = 1; i < instr.size(); i++) {
                memory[dataAddr++] = instr[i];
            }
        }
    }
}

int main() {
    // Read simulation parameters
    int memorySize, timeSlice, contextSwitchTime, numProcesses;
    cin >> memorySize >> timeSlice >> contextSwitchTime >> numProcesses;
    
    // Setup data structures
    vector<int> mainMemory;
    vector<PCB*> allProcesses;
    queue<PCB*> readyQueue;
    queue<PCB*> ioQueue;
    
    // For tracking termination times
    map<int, int> terminationTimes;
    
    // Read process information
    int memoryOffset = 0;
    for (int i = 0; i < numProcesses; i++) {
        PCB* proc = new PCB();
        int id, memory, instructionCount;
        cin >> id >> memory >> instructionCount;
        
        // Set basic process info
        proc->processID = id;
        proc->state = READY;
        proc->memoryLimit = memory;
        proc->maxMemoryNeeded = memory;
        proc->CPUAllocated = timeSlice;
        proc->remainingInstructions = instructionCount;
        
        // Set memory locations
        proc->mainMemoryBase = memoryOffset;
        proc->instructionBase = memoryOffset + 10; // After PCB
        proc->dataBase = proc->instructionBase + instructionCount;
        
        // Read instructions
        for (int j = 0; j < instructionCount; j++) {
            int opcode;
            cin >> opcode;
            
            vector<int> instruction;
            instruction.push_back(opcode);
            
            // Read parameters based on opcode
            switch (opcode) {
                case 1: // Compute
                    {
                        int iterations, cycles;
                        cin >> iterations >> cycles;
                        instruction.push_back(iterations);
                        instruction.push_back(cycles);
                    }
                    break;
                    
                case 2: // Print
                    {
                        int cycles;
                        cin >> cycles;
                        instruction.push_back(cycles);
                    }
                    break;
                    
                case 3: // Store
                    {
                        int value, address;
                        cin >> value >> address;
                        instruction.push_back(value);
                        instruction.push_back(address);
                    }
                    break;
                    
                case 4: // Load
                    {
                        int address;
                        cin >> address;
                        instruction.push_back(address);
                    }
                    break;
            }
            
            proc->instructions.push_back(instruction);
        }
        
        // Update memory offset for next process
        memoryOffset += memory;
        
        // Add to process list and ready queue
        allProcesses.push_back(proc);
        readyQueue.push(proc);
    }
    
    // Load processes into memory
    loadMemory(allProcesses, mainMemory, memorySize);
    
    // Print memory contents (for debugging)
    for (int i = 0; i < memorySize; i++) {
        cout << i << " : " << mainMemory[i] << endl;
    }
    
    // Main simulation loop
    int globalClock = 0;
    while (!readyQueue.empty() || !ioQueue.empty()) {
        // Handle idle CPU (only I/O waiting)
        while (readyQueue.empty() && !ioQueue.empty()) {
            checkIOQueue(globalClock, ioQueue, readyQueue);
            if (readyQueue.empty()) {
                globalClock += contextSwitchTime;
            }
        }
        
        // If no processes left, exit
        if (readyQueue.empty()) break;
        
        // Get next process
        PCB* currentProc = readyQueue.front();
        readyQueue.pop();
        
        // Context switch
        globalClock += contextSwitchTime;
        cout << "Process " << currentProc->processID << " has moved to Running." << endl;
        
        // First time running?
        if (currentProc->startRunningTime == -1) {
            currentProc->startRunningTime = globalClock;
        }
        
        // Execute time slice
        int sliceCycles = 0;
        bool ioOccurred = false;
        bool timeoutOccurred = false;
        
        // Execute until time slice expires or process finishes
        while (currentProc->remainingInstructions > 0 && sliceCycles < currentProc->CPUAllocated) {
            // Get current instruction
            vector<int>& instr = currentProc->instructions[currentProc->currentInstruction];
            int opcode = instr[0];
            
            switch (opcode) {
                case 1: // Compute
                    {
                        int iterations = instr[1];
                        int cycles = instr[2];
                        int totalCycles = iterations * cycles;
                        
                        cout << "compute" << endl;
                        
                        // Update timers
                        sliceCycles += totalCycles;
                        currentProc->cpuCyclesUsed += totalCycles;
                        globalClock += totalCycles;
                        
                        // Update memory
                        mainMemory[currentProc->mainMemoryBase + 6] = currentProc->cpuCyclesUsed;
                        
                        // Move to next instruction
                        currentProc->currentInstruction++;
                        currentProc->remainingInstructions--;
                        mainMemory[currentProc->mainMemoryBase + 2] = currentProc->currentInstruction;
                        
                        // Check if time slice expired
                        if (sliceCycles >= currentProc->CPUAllocated) {
                            timeoutOccurred = true;
                        }
                    }
                    break;
                    
                case 2: // Print
                    {
                        int printCycles = instr[1];
                        
                        // Set up I/O
                        currentProc->ioReleaseTime = globalClock + printCycles;
                        currentProc->state = IOWAITING;
                        currentProc->pendingPrint = true;
                        
                        cout << "Process " << currentProc->processID 
                             << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
                        
                        // Move to next instruction
                        currentProc->currentInstruction++;
                        currentProc->remainingInstructions--;
                        mainMemory[currentProc->mainMemoryBase + 2] = currentProc->currentInstruction;
                        
                        ioOccurred = true;
                        break;
                    }
                    
                case 3: // Store
                    {
                        int value = instr[1];
                        int address = instr[2];
                        
                        cout << "stored" << endl;
                        
                        // Update timers
                        sliceCycles += 1;
                        currentProc->cpuCyclesUsed += 1;
                        globalClock += 1;
                        
                        // Perform store operation
                        if (address < currentProc->memoryLimit) {
                            mainMemory[currentProc->mainMemoryBase + address] = value;
                            currentProc->registerValue = value;
                            mainMemory[currentProc->mainMemoryBase + 7] = value;
                        } else {
                            cout << "store error!" << endl;
                        }
                        
                        // Move to next instruction
                        currentProc->currentInstruction++;
                        currentProc->remainingInstructions--;
                        mainMemory[currentProc->mainMemoryBase + 2] = currentProc->currentInstruction;
                        
                        // Check if time slice expired
                        if (sliceCycles >= currentProc->CPUAllocated) {
                            timeoutOccurred = true;
                        }
                    }
                    break;
                    
                case 4: // Load
                    {
                        int address = instr[1];
                        
                        cout << "loaded" << endl;
                        
                        // Update timers
                        sliceCycles += 1;
                        currentProc->cpuCyclesUsed += 1;
                        globalClock += 1;
                        
                        // Perform load operation
                        if (address < currentProc->memoryLimit) {
                            currentProc->registerValue = mainMemory[currentProc->mainMemoryBase + address];
                            mainMemory[currentProc->mainMemoryBase + 7] = currentProc->registerValue;
                        } else {
                            cout << "load error!" << endl;
                            currentProc->registerValue = -1;
                            mainMemory[currentProc->mainMemoryBase + 7] = -1;
                        }
                        
                        // Move to next instruction
                        currentProc->currentInstruction++;
                        currentProc->remainingInstructions--;
                        mainMemory[currentProc->mainMemoryBase + 2] = currentProc->currentInstruction;
                        
                        // Check if time slice expired
                        if (sliceCycles >= currentProc->CPUAllocated) {
                            timeoutOccurred = true;
                        }
                    }
                    break;
            }
            
            // If I/O occurred, break out of the instruction loop
            if (ioOccurred) {
                break;
            }
        }
        
        // Process next steps
        if (currentProc->remainingInstructions > 0) {
            if (ioOccurred) {
                // Move to I/O queue
                ioQueue.push(currentProc);
                checkIOQueue(globalClock, ioQueue, readyQueue);
            } else if (timeoutOccurred) {
                // Time slice expired - return to ready queue
                cout << "Process " << currentProc->processID 
                     << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
                readyQueue.push(currentProc);
                checkIOQueue(globalClock, ioQueue, readyQueue);
            } else {
                // Should not happen, but added for safety
                cout << "ERROR" << endl;
                readyQueue.push(currentProc);
            }
        } else {
            // Process completed
            // Set final program counter to point to last field of PCB
            int finalPC = currentProc->mainMemoryBase + 9;
            mainMemory[currentProc->mainMemoryBase + 2] = finalPC;
            
            currentProc->state = TERMINATED;
            currentProc->terminationTime = globalClock;
            
            // Calculate total execution time
            int totalExecTime = currentProc->terminationTime - currentProc->startRunningTime;
            
            // Output process completion information
            cout << "Process ID: " << currentProc->processID << endl;
            cout << "State: TERMINATED" << endl;
            cout << "Program Counter: " << finalPC << endl;
            cout << "Instruction Base: " << currentProc->instructionBase << endl;
            cout << "Data Base: " << currentProc->dataBase << endl;
            cout << "Memory Limit: " << currentProc->memoryLimit << endl;
            cout << "CPU Cycles Used: " << currentProc->cpuCyclesUsed << endl;
            cout << "Register Value: " << currentProc->registerValue << endl;
            cout << "Max Memory Needed: " << currentProc->maxMemoryNeeded << endl;
            cout << "Main Memory Base: " << currentProc->mainMemoryBase << endl;
            cout << "Total CPU Cycles Consumed: " << totalExecTime << endl;
            
            cout << "Process " << currentProc->processID << " terminated. Entered running state at: " 
                 << currentProc->startRunningTime << ". Terminated at: " 
                 << currentProc->terminationTime << ". Total Execution Time: " 
                 << totalExecTime << "." << endl;
                 
            // Record termination time
            terminationTimes[currentProc->processID] = currentProc->terminationTime;
            
            // Check I/O queue for any completed operations
            checkIOQueue(globalClock, ioQueue, readyQueue);
        }
    }
    
    // Report total CPU time used (including final context switch)
    cout << "Total CPU time used: " << (globalClock + contextSwitchTime) << "." << endl;
    
    // Clean up memory
    for (PCB* proc : allProcesses) {
        delete proc;
    }
    
    return 0;
}