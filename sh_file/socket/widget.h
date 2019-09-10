#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
//#include <communication.h>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QStringList>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QFile>
#include <myconfig.h>
#include <modbustcpclient.h>
#include <QTimer>
#include <QThread>
#include <QElapsedTimer>
#include <sys/time.h>
namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_readbutton_clicked();

    void on_writebutton_clicked();

    void on_readbutton_2_clicked();

private:
    Ui::Widget *ui;
    //Communication * socket;
    QTcpSocket *socket;
    QJsonObject json; //ＪＳＯＮ数据结构
    QJsonDocument docment;
    QNetworkReply *_reply;
    QNetworkAccessManager *_uploadManager;
    QString ip;
    int port;
    QString url_str;
    QString modbus_url;
    ModbusTcpClient *modbus;
    QTimer *timer;
    QTimer *timer_run_main;
    QString m_on_color;
    QString m_off_color;
    bool stop_flag;
    bool run_flag;
    bool emit_run;
    int read_times;
    QByteArray tcp_read_message;
    const int FRIST_READ = 20000;
private:
    bool CreatClient(QString ip,int port);
    void HandleMessage();
    void StringToHex(QString str, QByteArray &senddata);
    char ConvertHexChar(char ch);
    void replyFinished(QNetworkReply *reply);
    void postServer(QUrl url,QString message);
    void SaveTxt(QString path,QString str);
    void ReadTxt(QString path);
    void getIniFile(QString path);
    void RefreshIo();
    void run_main();
    void start_timer();
    void colse_handle();
    void TcpClientDisconnect();
signals:
    void runMain();
};

#endif // WIDGET_H
