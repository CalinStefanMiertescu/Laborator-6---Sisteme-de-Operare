//LINUX

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>

#define BUFFER_SIZE 4096

bool is_prime(int num) {
    if (num <= 1) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

void child_process(int start, int end, int write_fd) {
    std::ostringstream oss;
    for (int i = start; i <= end; ++i) {
        if (is_prime(i)) {
            oss << i << " ";
        }
    }
    std::string primes = oss.str();
    write(write_fd, primes.c_str(), primes.size());
    close(write_fd);
    _exit(0);
}

int main(int argc, char* argv[]) {
    if (argc == 4) {
        int start = std::stoi(argv[1]);
        int end = std::stoi(argv[2]);
        int write_fd = std::stoi(argv[3]);
        child_process(start, end, write_fd);
        return 0;
    }

    const int num_processes = 10;
    const int interval_size = 1000;
    int pipes[num_processes][2];
    pid_t pids[num_processes];

    for (int i = 0; i < num_processes; ++i) {
        if (pipe(pipes[i]) == -1) {
            std::cerr << "Error creating pipe for process " << i << std::endl;
            return 1;
        }

        int start = i * interval_size + 1;
        int end = (i + 1) * interval_size;

        pid_t pid = fork();
        if (pid == -1) {
            std::cerr << "Error creating process " << i << std::endl;
            return 1;
        } else if (pid == 0) {
            close(pipes[i][0]); // Închidem capătul de citire al pipe-ului în copil
            std::ostringstream write_fd_str;
            write_fd_str << pipes[i][1];
            execlp(argv[0], argv[0], std::to_string(start).c_str(), std::to_string(end).c_str(), write_fd_str.str().c_str(), nullptr);
            std::cerr << "Error executing child process" << std::endl;
            _exit(1);
        } else {
            close(pipes[i][1]); // Închidem capătul de scriere al pipe-ului în părinte
            pids[i] = pid;
        }
    }

    std::cout << "Prime numbers found by each process:\n";
    for (int i = 0; i < num_processes; ++i) {
        char buffer[BUFFER_SIZE];
        ssize_t bytesRead;

        std::cout << "Process " << i + 1 << " (" << i * interval_size + 1 << "-" << (i + 1) * interval_size << "): ";
        while ((bytesRead = read(pipes[i][0], buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[bytesRead] = '\0';
            std::cout << buffer;
        }
        std::cout << "\n\n";
        close(pipes[i][0]); // Închidem capătul de citire al pipe-ului
        waitpid(pids[i], nullptr, 0); // Așteptăm procesul copil să se termine
    }

    return 0;
}
