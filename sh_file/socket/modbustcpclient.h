#ifndef MODBUSTCPCLIENT_H
#define MODBUSTCPCLIENT_H

#include <QObject>
#include <QModbusClient>
#include <QModbusTcpClient>
#include <QModbusDataUnit>
#include <QUrl>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QThread>
class ModbusTcpClient : public QObject
{
    Q_OBJECT
public:
    explicit ModbusTcpClient(QObject *parent = 0);

signals:

public slots:
public:
    bool GetConnect(const QString url_str);
    void write(int start_bit,int value,int address = 1);
    void readCoils (int start_bit, int address = 1);
    void readAll();
    void readRegisters(int start_bit,int adress =1);
    void readReady();
    int getBitValue(int start_bit);
    void init_bitMap();

private:
   // QModbusReply *lastRequest;
    QModbusClient *modbusDevice;
    //startbit startbitValue
    QMap<int,int> bit_map;
    QMutex mutex;
    QMutex modbus_mutex;
signals:
    void connect_success_signal();
};

#endif // MODBUSTCPCLIENT_H
