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

        QByteArray fileData = file.readAll();
        QByteArray usernameData = username->text().toUtf8();
        QByteArray filename = QFileInfo(file).fileName().toUtf8();

        int usernameLen = usernameData.size();
        int filenameLen = filename.size();
        int fileLen = fileData.size();

        // Send structure: [usernameLen][filenameLen][fileLen][username][filename][file content]
        socket.write(reinterpret_cast<const char *>(&usernameLen), sizeof(int));
        socket.write(reinterpret_cast<const char *>(&filenameLen), sizeof(int));
        socket.write(reinterpret_cast<const char *>(&fileLen), sizeof(int));
        socket.write(usernameData);
        socket.write(filename);
        socket.write(fileData);

        socket.flush();
        socket.waitForBytesWritten();
        socket.disconnectFromHost();

        QMessageBox::information(&window, "Success", "File submitted to server.");
    });

    window.show();
    return app.exec();
}



// #include <QApplication>
// #include <QVBoxLayout>
// #include <QPushButton>
// #include <QFileDialog>
// #include <QLineEdit>
// #include <QLabel>
// #include <QMessageBox>

// int main(int argc, char *argv[]) {
//     QApplication app(argc, argv);
//     QWidget window;
//     window.setWindowTitle("Upload Homework");

//     QVBoxLayout *layout = new QVBoxLayout(&window);

//     QLineEdit *username = new QLineEdit();
//     username->setPlaceholderText("Enter your username");

//     QPushButton *uploadBtn = new QPushButton("Upload File");
//     QLabel *fileLabel = new QLabel("No file selected.");

//     layout->addWidget(new QLabel("Username:"));
//     layout->addWidget(username);
//     layout->addWidget(uploadBtn);
//     layout->addWidget(fileLabel);

//     QObject::connect(uploadBtn, &QPushButton::clicked, [&]() {
//         QString filePath = QFileDialog::getOpenFileName(&window, "Select file");
//         if (!filePath.isEmpty()) {
//             fileLabel->setText("Selected: " + filePath);
//             QMessageBox::information(&window, "Upload", "File uploaded successfully (simulated).");
//         }
//     });

//     window.show();
//     return app.exec();
// }

// /opt/homebrew/opt/qt/bin/qmake page.pro  
// make
// cp page.app/Contents/MacOS/page ../server/Page