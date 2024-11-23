// Windows

#include <iostream>
#include <windows.h>
#include <sstream>
#include <vector>
#include <string>

using namespace std; 

#define BUFFER_SIZE 4096

bool is_prime(int num) {
    if (num <= 1) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

void child_process(int start, int end, HANDLE writePipe) {
    ostringstream oss;
    for (int i = start; i <= end; ++i) {
        if (is_prime(i)) {
            oss << i << " ";
        }
    }
    string primes = oss.str();
    DWORD bytesWritten;
    WriteFile(writePipe, primes.c_str(), primes.size(), &bytesWritten, NULL);
    CloseHandle(writePipe);
    ExitProcess(0);
}

int main(int argc, char* argv[]) {
    if (argc == 4) {
        int start = stoi(argv[1]);
        int end = stoi(argv[2]);
        HANDLE writePipe = (HANDLE)stoull(argv[3]); // Interpretăm corect handle-ul
        child_process(start, end, writePipe);
        return 0;
    }

    const int num_processes = 10;
    const int interval_size = 1000;

    HANDLE readPipes[num_processes], writePipes[num_processes];
    PROCESS_INFORMATION procInfo[num_processes];
    STARTUPINFOA startInfo[num_processes];

    for (int i = 0; i < num_processes; ++i) {
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
        HANDLE readPipe, writePipe;
        if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) {
            cerr << "Error creating pipe for process " << i << endl;
            return 1;
        }

        readPipes[i] = readPipe;
        writePipes[i] = writePipe;

        ZeroMemory(&startInfo[i], sizeof(STARTUPINFOA));
        startInfo[i].cb = sizeof(STARTUPINFOA);

        int start = i * interval_size + 1;
        int end = (i + 1) * interval_size;
       ostringstream command;
        command << argv[0] << " " << start << " " << end << " " << (uintptr_t)writePipe;

        string cmdStr = command.str();

        ZeroMemory(&procInfo[i], sizeof(PROCESS_INFORMATION));
        if (!CreateProcessA(NULL, const_cast<char*>(cmdStr.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &startInfo[i], &procInfo[i])) {
            cerr << "Error creating process. Command: " << cmdStr << endl;
            CloseHandle(writePipe);
            CloseHandle(readPipe);
            return 1;
        }
        CloseHandle(writePipe);
    }

    cout << "Prime numbers found by each process:\n";
    for (int i = 0; i < num_processes; ++i) {
        char buffer[BUFFER_SIZE];
        DWORD bytesRead;

        cout << "Process " << i + 1 << " (" << i * interval_size + 1 << "-" << (i + 1) * interval_size << "): ";
        while (ReadFile(readPipes[i], buffer, BUFFER_SIZE - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            cout << buffer;
        }
        cout << "\n";
        cout << endl; 
        CloseHandle(readPipes[i]);
        WaitForSingleObject(procInfo[i].hProcess, INFINITE);
        CloseHandle(procInfo[i].hProcess);
        CloseHandle(procInfo[i].hThread);
    }

    return 0;
}
