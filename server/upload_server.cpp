#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstdlib>

#define PORT 9090

std::string runTest(const std::string &exePath, const std::string &testFilePath, std::string &expectedOutput) {
    std::ifstream testFile(testFilePath);
    if (!testFile.is_open()) return "Test file not found";

    std::string input1, input2, star, expected;
    testFile >> input1 >> input2 >> star >> expected;

    expectedOutput = expected;

    std::ofstream tempInput("temp_input.txt");
    tempInput << input1 << "\n" << input2 << std::endl;
    tempInput.close();

    std::string command = exePath + " < temp_input.txt > temp_output.txt";
    int status = std::system(command.c_str());

    if (status != 0) return "Execution failed";

    std::ifstream out("temp_output.txt");
    std::string actualOutput;
    std::getline(out, actualOutput);
    out.close();

    std::filesystem::remove("temp_input.txt");
    std::filesystem::remove("temp_output.txt");

    if (actualOutput == expected) return "Pass";
    else return "Fail (Got: " + actualOutput + ")";
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 1);

    std::cout << "Upload server waiting on port " << PORT << "...\n";

    while (true) {
        client_sock = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        std::cout << "Client connected for upload.\n";

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

        close(client_sock);

        std::filesystem::create_directory(username);
        std::string fullPath = username + "/" + filename;
        std::ofstream outFile(fullPath, std::ios::binary);
        outFile.write(fileData.data(), fileData.size());
        outFile.close();

        std::cout << "Saved file to " << fullPath << std::endl;

        // ==== Start Testing ====
        std::string reportPath = username + "/report.txt";
        std::ofstream report(reportPath);

        if (filename.size() >= 2 && filename.substr(filename.size() - 2) == ".c") {
            std::string exePath = username + "/student_exec";
            std::string compileCmd = "gcc " + fullPath + " -o " + exePath + " 2> " + username + "/compile_errors.txt";
            int compileStatus = std::system(compileCmd.c_str());

            if (compileStatus == 0) {
                report << "Compilation: Success\n";
                std::string expected;
                std::string result = runTest("./" + exePath, "test.txt", expected);
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
    }

    close(server_fd);
    return 0;
}
