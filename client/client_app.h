#ifndef CLIENT_APP_H
#define CLIENT_APP_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>

class ClientApp : public QWidget {
    Q_OBJECT

public:
    explicit ClientApp(QWidget *parent = nullptr);

private slots:
    void connectToServer();

private:
    QLineEdit *ipInput;
    QLabel *imageLabel;
    QLabel *reportLabel;
};

#endif // CLIENT_APP_H
