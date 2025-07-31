#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>

#define SERVER_PORT 9090  // Use a separate port for upload

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("Upload Homework");

    QVBoxLayout *layout = new QVBoxLayout(&window);

    QLineEdit *username = new QLineEdit();
    username->setPlaceholderText("Enter your username");

    QPushButton *uploadBtn = new QPushButton("Upload File");
    QPushButton *submitBtn = new QPushButton("Submit");

    QLabel *fileLabel = new QLabel("No file selected.");
    QString selectedFile;

    layout->addWidget(new QLabel("Username:"));
    layout->addWidget(username);
    layout->addWidget(uploadBtn);
    layout->addWidget(fileLabel);
    layout->addWidget(submitBtn);

    QObject::connect(uploadBtn, &QPushButton::clicked, [&]() {
        selectedFile = QFileDialog::getOpenFileName(&window, "Select file");
        if (!selectedFile.isEmpty()) {
            fileLabel->setText("Selected: " + selectedFile);
        }
    });

   QObject::connect(submitBtn, &QPushButton::clicked, [&]() {
    if (username->text().isEmpty() || selectedFile.isEmpty()) {
        QMessageBox::warning(&window, "Error", "Enter username and select a file.");
        return;
    }

    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", SERVER_PORT);  // Update IP if needed
    if (!socket.waitForConnected(3000)) {
        QMessageBox::critical(&window, "Error", "Failed to connect to server.");
        return;
    }

    QFile file(selectedFile);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(&window, "Error", "Cannot open selected file.");
        return;
    }
int mode = 1;
socket.write(reinterpret_cast<const char*>(&mode), sizeof(int));

    QByteArray fileData = file.readAll();
    QByteArray usernameData = username->text().toUtf8();
    QByteArray filename = QFileInfo(file).fileName().toUtf8();

    int usernameLen = usernameData.size();
    int filenameLen = filename.size();
    int fileLen = fileData.size();

    // Send data to server
    socket.write(reinterpret_cast<const char *>(&usernameLen), sizeof(int));
    socket.write(reinterpret_cast<const char *>(&filenameLen), sizeof(int));
    socket.write(reinterpret_cast<const char *>(&fileLen), sizeof(int));
    socket.write(usernameData);
    socket.write(filename);
    socket.write(fileData);

    socket.flush();

    // 🔁 WAIT for grading report from server (this part was not functioning reliably)
    int reportLen = 0;
    QElapsedTimer timer;
    timer.start();

    while (socket.bytesAvailable() < sizeof(int)) {
        if (!socket.waitForReadyRead(1000)) {
            if (timer.elapsed() > 30000) {
                QMessageBox::warning(&window, "Error", "Timeout waiting for grading report.");
                return;
            }
        }
    }

    if (socket.read(reinterpret_cast<char*>(&reportLen), sizeof(int)) <= 0) {
        QMessageBox::warning(&window, "Error", "Failed to read report length.");
        return;
    }


        QByteArray reportData;
        timer.restart();
        while (reportData.size() < reportLen) {
            if (!socket.waitForReadyRead(1000)) {
                if (timer.elapsed() > 30000) {
                    QMessageBox::warning(&window, "Error", "Timeout reading report content.");
                    return;
                }
            }
            reportData.append(socket.read(reportLen - reportData.size()));
        }


    QString report = QString::fromUtf8(reportData);
    QMessageBox::information(&window, "Grading Report", report);

    socket.disconnectFromHost();
});


    window.show();
    return app.exec();
}

// /opt/homebrew/opt/qt/bin/qmake page.pro  
// make
// cp page.app/Contents/MacOS/page ../server/Page