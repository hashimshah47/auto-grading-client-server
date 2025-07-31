#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <tuple>
#include <thread>
#include <chrono>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 9090

void sendFile(int client_sock, const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return;
    file.seekg(0, std::ios::end);
    int size = file.tellg();
    file.seekg(0);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    send(client_sock, &size, sizeof(int), 0);
    send(client_sock, buffer.data(), size, 0);
}

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

std::string runTest(const std::string &exePath, const std::string &testFilePath, std::string &expectedOutput) {
    std::ifstream testFile(testFilePath);
    if (!testFile.is_open()) return "Test file not found";

    std::string line, input1, input2, star, expected;
    std::vector<std::tuple<std::string, std::string, std::string>> tests;

    while (std::getline(testFile, line)) {
        if (line == "#") {
            if (!input1.empty() && !input2.empty() && !expected.empty()) {
                tests.emplace_back(input1, input2, expected);
                input1 = input2 = expected = "";
            }
            continue;
        }
        if (input1.empty()) input1 = line;
        else if (input2.empty()) input2 = line;
        else if (star.empty() && line == "*") continue;
        else expected = line;
    }

    if (!input1.empty() && !input2.empty() && !expected.empty()) {
        tests.emplace_back(input1, input2, expected);  // Last test if no trailing #
    }

    int testNum = 1;
    std::ostringstream reportStream;
    bool allPassed = true;

    for (const auto& [a, b, exp] : tests) {
        std::ofstream tempInput("temp_input.txt");
        tempInput << a << "\n" << b << std::endl;
        tempInput.close();

        pid_t pid = fork();
        if (pid == 0) {
            // In child process
            freopen("temp_input.txt", "r", stdin);
            freopen("temp_output.txt", "w", stdout);
            execl(exePath.c_str(), exePath.c_str(), (char*)NULL);
            perror("exec failed");
            exit(1);
        } else if (pid > 0) {
            // In parent process
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                std::ifstream out("temp_output.txt");
                std::string actualOutput;
                std::getline(out, actualOutput);
                out.close();

                if (actualOutput != exp) {
                    allPassed = false;
                    reportStream << "Test " << testNum << ": Failed (Expected: " << exp << ", Got: " << actualOutput << ")\n";
                } else {
                    reportStream << "Test " << testNum << ": Passed\n";
                }
            } else {
                allPassed = false;
                reportStream << "Test " << testNum << ": Execution failed.\n";
            }
        } else {
            allPassed = false;
            reportStream << "Test " << testNum << ": Fork failed.\n";
        }

        ++testNum;
    }

    std::filesystem::remove("temp_input.txt");
    std::filesystem::remove("temp_output.txt");

    return allPassed ? "Pass\n" + reportStream.str() : "Fail\n" + reportStream.str();
}


void handleClient(int client_sock, const std::string &testFilePath) {
    int mode;
    recv(client_sock, &mode, sizeof(int), 0);

    if (mode == 0) {
        // Send logo + Page
        sendFile(client_sock, "logo.png");
        sendFile(client_sock, "Page");
        std::cout << "Sent logo and Page to client.\n";
    } else if (mode == 1) {
        // Handle file submission
        int usernameLen = 0, filenameLen = 0, fileLen = 0;
        recv(client_sock, &usernameLen, sizeof(int), 0);
        recv(client_sock, &filenameLen, sizeof(int), 0);
        recv(client_sock, &fileLen, sizeof(int), 0);

        std::vector<char> usernameBuf(usernameLen);
        recv(client_sock, usernameBuf.data(), usernameLen, 0);
        std::string username(usernameBuf.begin(), usernameBuf.end());

        std::vector<char> filenameBuf(filenameLen);
        recv(client_sock, filenameBuf.data(), filenameLen, 0);
        std::string filename(filenameBuf.begin(), filenameBuf.end());

        std::vector<char> fileData(fileLen);
        int bytesReceived = 0;
        while (bytesReceived < fileLen) {
            int n = recv(client_sock, fileData.data() + bytesReceived, fileLen - bytesReceived, 0);
            if (n <= 0) break;
            bytesReceived += n;
        }

        std::filesystem::create_directory(username);
        std::string fullPath = username + "/" + filename;
        std::ofstream outFile(fullPath, std::ios::binary);
        outFile.write(fileData.data(), fileData.size());
        outFile.close();

        std::cout << "Received and saved: " << fullPath << std::endl;

        // Grading
        std::string reportPath = username + "/report.txt";
        std::ofstream report(reportPath);
        std::string exePath = username + "/student_exec";

        if (filename.size() >= 2 && filename.substr(filename.size() - 2) == ".c") {
            std::string compileCmd = "gcc " + fullPath + " -o " + exePath + " 2> " + username + "/compile_errors.txt";
            int compileStatus = std::system(compileCmd.c_str());

            if (compileStatus == 0) {
                report << "Compilation: Success\n";
                std::string expected;
                std::string result = runTest("./" + exePath, testFilePath, expected);
                report << "Code Testing: " << result << "\nExpected: " << expected << "\n";
            } else {
                report << "Compilation: Failed\n";
                std::ifstream err(username + "/compile_errors.txt");
                std::stringstream buffer;
                buffer << err.rdbuf();
                report << "Error:\n" << buffer.str();
            }
        } else {
            report << "Non-C file submitted. Skipping compilation.\n";
        }
        report.close();

        // Send back report
        std::ifstream reportIn(reportPath);
        std::stringstream buffer;
        buffer << reportIn.rdbuf();
        std::string reportStr = buffer.str();
        int reportLen = reportStr.size();

        send(client_sock, &reportLen, sizeof(int), 0);
        send(client_sock, reportStr.c_str(), reportLen, 0);
        std::cout << "Sent report.\n";
    }

    close(client_sock);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./unified_server test.txt\n";
        return 1;
    }

    std::string testFilePath = argv[1];
    int server_fd, client_sock;
    struct sockaddr_in address{};
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    std::cout << "Unified server running on port " << PORT << "...\n";

    while (true) {
        client_sock = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        std::thread(handleClient, client_sock, testFilePath).detach();
    }

    close(server_fd);
    return 0;
}

//g++ server.cpp -std=c++17 -o server

// g++ server.cpp -o server
// ./server