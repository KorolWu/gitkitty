#ifndef COMMUNICATION_H
#define COMMUNICATION_H


#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QStringList>
#include <QMap>
#include <QMutex>
//COMMUNICATIONSHARED_EXPORT
class  COMMUNICATIONSHARED_EXPORT Communication:public QObject
{
    Q_OBJECT
public:
    Communication();
    QSerialPort *serial;
    QTcpServer *server;
    QTcpSocket *socket;
    QString m_ReadBuff ="";
    QMap<QString,QString>ReadMassage;
    QString Client_Read = "";
    QString m_emitStr;
    QString m_tempstr = "";
    QString m_CheckStr = "";
    QString m_serial_read="";
    QMap<QString,QString> Serial_Read;
    QString m_show_style="";
    QMutex  m_Wmutex;
public:
    bool FoundPort();//寻找串口
    bool OpenPort(QString name,int BaudRate, int DataBits, int Parity, int StopBits);//打开串口
    void ClosePort();
    void ReadDate(); //读取数据
    void WriteDate(QByteArray buf);//写入数据
    //******************TcpIp***********************
    void Server(quint16 port ,int &);
    bool Client(QString ip, quint16 port);
    int SendBuff(QByteArray buff);
    void CloseTCP();
    int SendBuff_Server(char *data,QString IpAddress);
signals:
    void reseive(char* value);//将读取的数据发送出去
    //void reseive(QByteArray value);
    void TCPResive(QByteArray buff);//tcpip 服务器收到的信息；
    void SendData(QByteArray buff);
private:
     static int iRead;
     int peerportNum;
     QString m_peerip;
     static unsigned char *readBuffer;
     QMap<QString,QTcpSocket*>socketmap;
     QTcpSocket* p_clientsocket;
     QString changeTohex(char cr[],int lenth);
private:
     /// <summary>
     /// 生成CRC校验位
     /// </summary>
     /// <param name="ptr"></param>
     /// <param name="len"></param>
     /// <param name="crc"></param>
    void CommonCrc8(unsigned char ptr[], int len, unsigned char *crc)
     {
         for (unsigned char i = 0; i < len; i++)
             GenerateCrc8(ptr[i], crc);
     }
     void GenerateCrc8(unsigned char c, unsigned char *crc)
     {
         for (unsigned char j = 0x80; j != 0; j >>= 1)
         {
             if (((*crc) & 0x80) != 0)
             {
                 (*crc) <<= 1;
                 (*crc) ^= 0x07;
             }
             else
             {
                 (*crc) <<= 1;
             }
             if (((c) & j) != 0)
             {
                 (*crc) ^= 0x07;
             }
         }
     }



};

#endif // COMMUNICATION_H
