#include "client_app.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QPixmap>
#include <QImage>
#include <QDir>
#include <QProcess>
#include <opencv2/opencv.hpp>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

#define PORT 8080

ClientApp::ClientApp(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Grading Client");
    resize(400, 400);

    auto *layout = new QVBoxLayout(this);

    ipInput = new QLineEdit(this);
    ipInput->setPlaceholderText("Enter server IP address");

    auto *connectBtn = new QPushButton("Connect", this);

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setText("Logo will appear here after connecting.");
    imageLabel->setFixedSize(300, 200);

    layout->addWidget(new QLabel("Server IP:"));
    layout->addWidget(ipInput);
    layout->addWidget(connectBtn);
    layout->addWidget(imageLabel);

    connect(connectBtn, &QPushButton::clicked, this, &ClientApp::connectToServer);
}

void ClientApp::connectToServer() {
    QString ip = ipInput->text();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter an IP address.");
        return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ip.toStdString().c_str(), &serv_addr.sin_addr) <= 0) {
        QMessageBox::critical(this, "Error", "Invalid IP address.");
        return;
    }

    if (::connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        QMessageBox::critical(this, "Error", "Connection failed.");
        return;
    }

    auto receiveFile = [&](QString fileName) {
        int file_size = 0;
        if (recv(sock, &file_size, sizeof(int), 0) <= 0) return false;

        std::vector<char> buffer(file_size);
        int bytes_received = 0;
        while (bytes_received < file_size) {
            int ret = recv(sock, buffer.data() + bytes_received, file_size - bytes_received, 0);
            if (ret <= 0) break;
            bytes_received += ret;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) return false;
        file.write(buffer.data(), buffer.size());
        file.setPermissions(QFile::ExeUser | QFile::ReadUser | QFile::WriteUser);
        file.close();
        return true;
    };

    // Receive image
    if (!receiveFile("logo.png")) {
        QMessageBox::critical(this, "Error", "Failed to receive image.");
        return;
    }

    // Load and display logo
    cv::Mat img = cv::imread("logo.png");
    if (!img.empty()) {
        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
        QImage qimg(img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);
        imageLabel->setPixmap(QPixmap::fromImage(qimg).scaled(imageLabel->size(), Qt::KeepAspectRatio));
    }

    // Receive and launch executable
    if (receiveFile("Page")) {
        QProcess *pageProc = new QProcess(this);
        pageProc->start(QDir::currentPath() + "/Page");
    } else {
        QMessageBox::critical(this, "Error", "Failed to receive or run Page.");
    }

    ::close(sock);
}
