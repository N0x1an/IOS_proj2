#include <iostream>
#include <queue>
#include <vector>
#include <map>

/*
Project 2: CPU Scheduling and Memory Management
CS 3113
Spring 2025
Jishan Rahman
*/
								  
using namespace std;

// Process States.
enum ProcessState { 
	NEW = 1,
	READY,
	RUNNING,
	IOWAITING,
	TERMINATED };

// PCB structure
struct PCB {
    int processID;
    int state;            // current state of PCB
    int programCounter;   // Stored at mainMemoryBase + 2
    int instructionBase;
    int dataBase;
    int memoryLimit;
    int cpuCyclesUsed;    // total cpu cycles used 
    int registerValue;
    int maxMemoryNeeded; // max amount of memory needed
    int mainMemoryBase;
    vector<vector<int> > jobOperations;
    
    // variables needed to get infomation about the processes
    int CPUAllocated;            // amount of time cpu is allowed
    int runningTimeStart;        // when process started time
    int terminationTime;         // when process ended time
    int currentInstructionIndex; // where the next instruction is indexed at
    int remainingInstructions;   // number of instructions left
    int ioReleaseTime;           // global clock time when I/O wait ends
    bool pendingPrint;           // condition if a print is pending
    
    PCB() :
		processID(0),
		state(NEW),
		programCounter(0),
		instructionBase(0),
		dataBase(0),
        memoryLimit(0),
		cpuCyclesUsed(0),
		registerValue(0),
		maxMemoryNeeded(0),
        mainMemoryBase(0),
		CPUAllocated(0),
		runningTimeStart(-1),
		terminationTime(0),
        currentInstructionIndex(0),
		remainingInstructions(0),
		ioReleaseTime(0),
        pendingPrint(false) {}
};

// Check the IOWaitingQueue
// If ioReleaseTime has passed
// print I/O message while moving to readyQueue
void IOQueueCheck(int globalClock, queue<PCB*>& ioWaitingQueue, queue<PCB*>& readyQueue) {
    queue<PCB*> temp;
    while (!ioWaitingQueue.empty()) {
        PCB* ioProcesses = ioWaitingQueue.front();
        ioWaitingQueue.pop();
        if (globalClock >= (*ioProcesses).ioReleaseTime) {
            cout << "print" << endl;
													
            cout << "Process " << (*ioProcesses).processID << " completed I/O and is moved to the ReadyQueue." << endl;
            (*ioProcesses).state = READY;
            readyQueue.push(ioProcesses);
        } else {
            temp.push(ioProcesses);
        }
    }
    while (!temp.empty()) {
        ioWaitingQueue.push(temp.front());
        temp.pop();
    }
}

// Load PCB ID, instructions, and data into memory
  
void loadJobsToMemory(queue<PCB>& newJobQueue, queue<int>& readyMemoryQueue, vector<int>& mainMemory, int maxMemory) {
    
    // Initialize main memory with -1 to indicate empty slots
    for (int i = 0; i < maxMemory; i++)
        mainMemory.push_back(-1);
    
    // Process jobs from the new job queue
    while (!newJobQueue.empty()) {
        PCB job = newJobQueue.front();  // Get the next job
        newJobQueue.pop();  // Remove it from the queue

        int memoryIndex = job.mainMemoryBase;  // Starting index in memory
        
        // Store PCB header fields in memory
        mainMemory[memoryIndex] = job.processID;
        mainMemory[memoryIndex + 1] = job.state;
        mainMemory[memoryIndex + 2] = job.programCounter;
        mainMemory[memoryIndex + 3] = job.instructionBase;
        mainMemory[memoryIndex + 4] = job.dataBase;
        mainMemory[memoryIndex + 5] = job.memoryLimit;
        mainMemory[memoryIndex + 6] = job.cpuCyclesUsed;
        mainMemory[memoryIndex + 7] = job.registerValue;
        mainMemory[memoryIndex + 8] = job.maxMemoryNeeded;
        mainMemory[memoryIndex + 9] = job.mainMemoryBase;
        
        memoryIndex += 10;  // Move past PCB metadata

        // Load job instructions into memory at the instruction base index
        memoryIndex = job.instructionBase;
        for (size_t i = 0; i < job.jobOperations.size(); i++) {
            mainMemory[memoryIndex] = job.jobOperations[i][0];  // Store operation code
            memoryIndex++;
        }

        // Load job data into memory at the data base index
        memoryIndex = job.dataBase;
        for (size_t i = 0; i < job.jobOperations.size(); i++) {
            for (size_t j = 1; j < job.jobOperations[i].size(); j++) {  // Skip first element (already stored)
                mainMemory[memoryIndex] = job.jobOperations[i][j];  // Store data
                memoryIndex++;
            }
        }
    }
}


// Main function
int main() {
    int maxMemory, CPUAllocated, contextSwitchTime, numProcesses;																	  
    
    // Read system parameters from input
    cin >> maxMemory >> CPUAllocated >> contextSwitchTime >> numProcesses;					   
    
    // Declare necessary queues and memory structures
    queue<PCB> newJobQueue;          // Queue for newly arriving jobs
    queue<int> readyMemoryQueue;      // Queue to manage memory allocation
    vector<int> mainMemory;           // Simulated main memory
    vector<PCB*> processes;         // List of dynamically allocated processes
    queue<PCB*> readyQueue;           // Queue for ready processes
    queue<PCB*> ioWaitingQueue;       // Queue for processes waiting for I/O
    
    int processID;
	int	instructionCount;
    int inputType;
	int	incomingInput;
    int totalMem = 0; // Tracks memory usage
						 
    for (int i = 0; i < numProcesses; i++) {
        PCB jobProcess;  // Create a new PCB instance																  
        
        // Read process ID, memory limit, and instruction count
        cin >> jobProcess.processID >> jobProcess.memoryLimit >> instructionCount;									 

        // Initialize PCB fields
        jobProcess.remainingInstructions = instructionCount;
        jobProcess.currentInstructionIndex = 0;
        jobProcess.CPUAllocated = CPUAllocated;
        jobProcess.runningTimeStart = -1;  // Process has not yet started running
        jobProcess.ioReleaseTime = 0;      // No pending I/O operations
        jobProcess.pendingPrint = false;   // No pending print operations
        
        // Memory layout setup
        jobProcess.mainMemoryBase = totalMem;
        jobProcess.maxMemoryNeeded = jobProcess.memoryLimit;
        jobProcess.instructionBase = jobProcess.mainMemoryBase + 10; // Reserve first 10 slots for PCB header
        jobProcess.dataBase = jobProcess.instructionBase + instructionCount;
        totalMem += jobProcess.maxMemoryNeeded;

        // Read process instructions
        for (int j = 0; j < instructionCount; j++) {
            cin >> inputType;
            vector<int> opcode;
            opcode.push_back(inputType);

            // Read instruction parameters based on instruction type
            switch (inputType) {
                case 1: // Example: Operation requiring two parameters
                    cin >> incomingInput;
                    opcode.push_back(incomingInput);
                    cin >> incomingInput;
                    opcode.push_back(incomingInput);
                    break;
                case 2: // Example: Operation requiring one parameter
                    cin >> incomingInput;
                    opcode.push_back(incomingInput);
                    break;
                case 3: // Example: Another operation with two parameters
                    cin >> incomingInput;
                    opcode.push_back(incomingInput);
                    cin >> incomingInput;
                    opcode.push_back(incomingInput);
                    break;
                case 4: // Example: Operation requiring one parameter
                    cin >> incomingInput;
                    opcode.push_back(incomingInput);
                    break;
            }

            // Store the instruction in the job's operation list
            jobProcess.jobOperations.push_back(opcode);
        }

        // Add job to new job queue
        newJobQueue.push(jobProcess);

        // Reserve extra space in memory for the PCB metadata
        totalMem += 10;  

        // Dynamically allocate a new PCB object and store it in the process list
        PCB* pProc = new PCB(jobProcess);
        processes.push_back(pProc);

        // Add the process to the ready queue
        readyQueue.push(pProc);
    }
    
    // Load jobs into memory
    loadJobsToMemory(newJobQueue, readyMemoryQueue, mainMemory, maxMemory);
    
    // print all of memory to console
	for (int i = 0; i < maxMemory; i++) {
        cout << i << " : " << mainMemory[i] << endl;
    }
    
    int globalClock = 0;

    //map termination times and its process
    map<int,int> terminationTimes;
								  
    
    // Run the simulation loop until all processes have terminated
    while (!readyQueue.empty() || !ioWaitingQueue.empty()) {
        // If readyQueue empty but processes are awaiting on I/O update the clock
	
        while (readyQueue.empty() && !ioWaitingQueue.empty()) {
            IOQueueCheck(globalClock, ioWaitingQueue, readyQueue);
            if(readyQueue.empty())
                globalClock += contextSwitchTime;
            continue;
        }
        
        // context switch out to next process
        PCB* currentProc = readyQueue.front();
        readyQueue.pop();
        globalClock += contextSwitchTime; // add in context switch time
        cout << "Process " << (*currentProc).processID << " has moved to Running." << endl;
        
        // Record start time if this is the first time the process is scheduled
        if ((*currentProc).runningTimeStart == -1) (*currentProc).runningTimeStart = globalClock;
        
        int sliceCycles = 0;       // Tracks the number of CPU cycles used within the current time slice
        bool ioOccurred = false;   // Flag to indicate if an I/O operation occurred during execution
        bool timeoutOccurred = false; // Flag to indicate if the process has reached its time slice limit

        // Execute until the time expires or an I/O event comes in
        while ((*currentProc).remainingInstructions > 0 && sliceCycles < (*currentProc).CPUAllocated) {
            vector<int>& instr = (*currentProc).jobOperations[(*currentProc).currentInstructionIndex];
            int instrType = instr[0];
            
            // if instruction is COMPUTE
            if (instrType == 1) { 
                int cost = instr[2];
                cout << "compute" << endl;
                sliceCycles += cost;
                (*currentProc).cpuCyclesUsed += cost;
                globalClock += cost;
                mainMemory[(*currentProc).mainMemoryBase + 6] = (*currentProc).cpuCyclesUsed;
                (*currentProc).currentInstructionIndex++;
                (*currentProc).remainingInstructions--;
                mainMemory[(*currentProc).mainMemoryBase + 2] = (*currentProc).currentInstructionIndex;
                if (sliceCycles >= (*currentProc).CPUAllocated)
                    timeoutOccurred = true;
            }
            // if instruction is PRINT
            else if (instrType == 2) { 
                int printCycles = instr[1];
                (*currentProc).cpuCyclesUsed += printCycles;
                mainMemory[(*currentProc).mainMemoryBase + 6] = (*currentProc).cpuCyclesUsed;
                (*currentProc).pendingPrint = true;
                (*currentProc).ioReleaseTime = globalClock + printCycles;										 
                cout << "Process " << (*currentProc).processID << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
                (*currentProc).currentInstructionIndex++;
                (*currentProc).remainingInstructions--;
                mainMemory[(*currentProc).mainMemoryBase + 2] = (*currentProc).currentInstructionIndex;
                ioOccurred = true;
                break;
            }
            // if instruction is STORE
            else if (instrType == 3) { 
                cout << "stored" << endl;
                sliceCycles += 1;
                (*currentProc).cpuCyclesUsed += 1;
                globalClock += 1;
                mainMemory[(*currentProc).mainMemoryBase + 6] = (*currentProc).cpuCyclesUsed;
                int value = instr[1];
                int address = instr[2];
                if (address < (*currentProc).memoryLimit &&
                    ((*currentProc).mainMemoryBase + address) < mainMemory.size()) {
                    mainMemory[(*currentProc).mainMemoryBase + address] = value;
                    (*currentProc).registerValue = value;
                    mainMemory[(*currentProc).mainMemoryBase + 7] = value;
                } else {
                    cout << "store error!" << endl;
                }
                (*currentProc).currentInstructionIndex++;
                (*currentProc).remainingInstructions--;
                mainMemory[(*currentProc).mainMemoryBase + 2] = (*currentProc).currentInstructionIndex;
                if (sliceCycles >= (*currentProc).CPUAllocated)
                    timeoutOccurred = true;
            }
            // if instruction is LOAD
            else if (instrType == 4) { 
                cout << "loaded" << endl;
                sliceCycles += 1;
                (*currentProc).cpuCyclesUsed += 1;
                globalClock += 1;
                mainMemory[(*currentProc).mainMemoryBase + 6] = (*currentProc).cpuCyclesUsed;
                int offset = instr[1];
                if (offset < (*currentProc).memoryLimit &&
                    ((*currentProc).mainMemoryBase + offset) < mainMemory.size()) {
                    (*currentProc).registerValue = mainMemory[(*currentProc).mainMemoryBase + offset];
                    mainMemory[(*currentProc).mainMemoryBase + 7] = (*currentProc).registerValue;
                } else {
                    cout << "load error!" << endl;
                    (*currentProc).registerValue = -1;
                    mainMemory[(*currentProc).mainMemoryBase + 7] = -1;
                }
                (*currentProc).currentInstructionIndex++;
                (*currentProc).remainingInstructions--;
                mainMemory[(*currentProc).mainMemoryBase + 2] = (*currentProc).currentInstructionIndex;
                if (sliceCycles >= (*currentProc).CPUAllocated)
                    timeoutOccurred = true;
            }
        }
        
        // check if any more instructions are left, if io push to ioWaitingQueue, if timeout push to readyQueue
        if ((*currentProc).remainingInstructions > 0) {
            if (ioOccurred) {
                ioWaitingQueue.push(currentProc);
                IOQueueCheck(globalClock, ioWaitingQueue, readyQueue);
            }
            else if (timeoutOccurred) {
                cout << "Process " << (*currentProc).processID 
                     << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
                readyQueue.push(currentProc);
                IOQueueCheck(globalClock, ioWaitingQueue, readyQueue);
            }
            else {
                cout << "ERROR" << endl;
                readyQueue.push(currentProc);
            }
        }
        else {  
            int pc = (*currentProc).mainMemoryBase + 9;
            mainMemory[(*currentProc).mainMemoryBase + 2] = pc;
            (*currentProc).terminationTime = globalClock;

            int totalCyclesConsumed = (*currentProc).terminationTime - (*currentProc).runningTimeStart;
            cout << "Process ID: " << (*currentProc).processID << endl;
            cout << "State: TERMINATED" << endl;
            cout << "Program Counter: " << pc << endl;
            cout << "Instruction Base: " << (*currentProc).instructionBase << endl;
            cout << "Data Base: " << (*currentProc).dataBase << endl;
            cout << "Memory Limit: " << (*currentProc).memoryLimit << endl;
            cout << "CPU Cycles Used: " << (*currentProc).cpuCyclesUsed << endl;
            cout << "Register Value: " << (*currentProc).registerValue << endl;
            cout << "Max Memory Needed: " << (*currentProc).maxMemoryNeeded << endl;
            cout << "Main Memory Base: " << (*currentProc).mainMemoryBase << endl;
            cout << "Total CPU Cycles Consumed: " << totalCyclesConsumed << endl;
            cout << "Process " << (*currentProc).processID << " terminated. Entered running state at: " << (*currentProc).runningTimeStart << ". Terminated at: " << (*currentProc).terminationTime << ". Total Execution Time: " << totalCyclesConsumed << "." << endl;
																		 
            // add in final termination time
            terminationTimes[(*currentProc).processID] = (*currentProc).terminationTime;
            IOQueueCheck(globalClock, ioWaitingQueue, readyQueue);
        }
    }
    
    // total CPU time used by all processes
    cout << "Total CPU time used: " << (globalClock + contextSwitchTime) << "." << endl;
    
    return 0;
}