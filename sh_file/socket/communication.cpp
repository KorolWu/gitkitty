#include "communication.h"
#include <QDataStream>
int Communication::iRead = 0;
unsigned char* Communication::readBuffer = new unsigned char[256];
const int bufLen = 256;
Communication::Communication()
{
    serial = NULL;
    server = NULL;
    socket = NULL;
    socketmap.clear();
    peerportNum = 0;
    connect(this,&Communication::SendData,this,&Communication::SendBuff);
}

///
/// \brief Communication::FoundPort 寻找串口
/// \return true 找到可用串口；  false 没有找到串口
///
bool Communication::FoundPort()
{
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QSerialPort serialfound;
        serialfound.setPort(info);
        if(serialfound.open(QIODevice::ReadWrite))
        {
            qDebug() << info.portName();
            serialfound.close();
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}
///
/// \brief Communication::ReadDate 读取串口数据 用reseive信号将数据发送出去
///
void Communication::ReadDate()
{
    m_serial_read = "";
    if(m_CheckStr.size()>1)
    {
        QStringList list = m_CheckStr.split(";");
        QByteArray buffstart = list[0].toLatin1();//strat
        QByteArray buffend = list[1].toLatin1();//end
        if(buffstart.size() > 2 || buffend.size() > 2)
        {
            return;
        }
        else
        {
            if (buffend[0] == '/')
            {
                buffend[0] = 10;
            }
        }

        static bool isData = false;
        static char cResult[bufLen] = {0};
        static int iLen = 0;

        char cSerialRead[bufLen+1];
        memset(cSerialRead,0,bufLen+1);

        qint64 serialLen = serial->read(cSerialRead,bufLen);
        char cSerial[serialLen]  = {0};
        memcpy(cSerial,cSerialRead,serialLen);

        for(int i = 0;i < serialLen;i++)
        {
            // 判断当前是否处于接收写入数据状态,
            //是:直接接收写入;
            //否:判断头校验符

            if(!isData)
            {
                if(buffstart.size() > 1)  // 2个头校验符
                {
                    // cResult为空,且当前字符是第一个头校验符,则写入cResult
                    if((cSerial[i] == buffstart[0]) && (cResult[0] == 0))
                    {
                        cResult[0] = cSerial[i];
                        continue;
                    }
                    // cResult不为空时
                    if(cResult[0] == buffstart[0])
                    {
                        // 判断当前字符是否是第二个头校验符,若是,则开始接收数据
                        if((cSerial[i] == buffstart[1]))
                        {
                            cResult[1] = cSerial[i];
                            isData = true;
                            iLen = 2;
                            continue;
                        }
                        // 判断当前字符是否是第一个头校验符,若是,则覆盖写入之前的第一个头校验符
                        else if(cSerial[i] == buffstart[0])
                        {
                            cResult[0] = cSerial[i];
                            continue;
                        }
                        // 当前字符既不是第一个,也不是第二个头校验,则将cResult置空
                        else
                        {
                            cResult[0] = 0;
                        }
                    }
                }
                else  // 1个头校验符
                {
                    if(cSerial[i] == buffstart[0])
                    {
                        cResult[0] = cSerial[i];
                        isData = true;
                        iLen = 1;
                    }
                }
            }
            else
            {
                // 先接收写入cResult,再校验末位字符
                int maxLen = bufLen - buffend.size() - 1;

                if(iLen > maxLen)//判断数据超范围的接收连续收到多个无效数据
                {
                    for(int i = 0;i < buffend.size(); i++)
                    {
                        cResult[maxLen+i+1] = buffend[i];
                    }

                    iLen = bufLen - 1;
                }
                else
                {
                    cResult[iLen] = cSerial[i];
                }

                qDebug() << "Data:" << cResult<<"iLen:"<<iLen;
                // 校验末位
                if(buffend.size() > 1)  //2个尾校验
                {
                    // 校验末2位字符
                    if((cResult[iLen-1] == buffend[0]) && (cResult[iLen] == buffend[1]))
                    {
                        //数据写入Map
                        isData = false;
                        QString ReadStr="";

                        if(m_show_style == "16")
                        {
                            ReadStr = changeTohex(cResult,iLen+1);
                        }
                        else
                        {
                            ReadStr = QString(cResult);
                        }
                        if((!isData) && (!ReadStr.isEmpty()))
                        {
                             m_Wmutex.lock();
                            if(Serial_Read.contains(serial->portName()))
                            {
                                Serial_Read[serial->portName()] = ReadStr;
                            }
                            else
                            {
                                Serial_Read.insert(serial->portName(),ReadStr);
                            }
                            m_Wmutex.unlock();
                            memset(cResult ,0,bufLen);
                            iLen = 0;
                        }
                    }
                    else
                    {
                        ++iLen;
                    }
                }
                else  //1个尾校验
                {
                    // 校验末1位字符
                    if(cResult[iLen] == buffend[0])
                    {
                        isData = false;
                        // 数据写入Map
                        QString ReadStr="";
                        if(m_show_style == "16")
                        {
                            ReadStr = changeTohex(cResult,iLen+1);
                        }
                        else
                        {
                            ReadStr = QString(cResult);
                        }

                        if((!isData) && (!ReadStr.isEmpty()))
                        {
                              m_Wmutex.lock();
                            if(Serial_Read.contains(serial->portName()))
                            {
                                Serial_Read[serial->portName()] = ReadStr;
                            }
                            else
                            {
                                Serial_Read.insert(serial->portName(),ReadStr);
                            }
                            m_Wmutex.unlock();
                           memset(cResult ,0,bufLen);
                            iLen = 0;
                        }
                    }
                    else
                    {
                        ++iLen;
                    }
                }
            }
        }
    }
    else
    {
        QByteArray buff = serial->readAll();
        while (true)
        {
            bool readok = serial->waitForReadyRead(50);
            if(!readok)
            {
                break;
            }
            buff += serial->readAll();
        }
        if(m_show_style == "16")
        {
            QDataStream out(&buff,QIODevice::ReadWrite);    //将字节数组读入
            QString re_str = "";
            while(!out.atEnd())
            {
                qint8 outChar = 0;
                out>>outChar;   //每字节填充一次，直到结束
                QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0'));
                re_str = re_str +str +"";
            }
            m_serial_read = re_str;
        }
        else
        {
            m_serial_read = QString::fromUtf8(buff);
        }
        m_Wmutex.lock();
        if(Serial_Read.contains(serial->portName()))
       {
           Serial_Read[serial->portName()] = m_serial_read;
       }
       else
       {
           Serial_Read.insert(serial->portName(),m_serial_read);
       }
         m_Wmutex.unlock();
    }
}
void Communication::ClosePort()
{
    if(serial != NULL)
    {
        serial->close();
        delete serial;
        serial = NULL;
    }
}

///
/// \brief Communication::OpenPort 打开并设置串口  true = 设置成功  false = 设置失败
/// \param name      串口名字
/// \param BaudRate  波特率
/// \param DataBits  数据位
/// \param Parity    奇偶校验
/// \param StopBits  停止位
///
///

bool Communication::OpenPort(QString name, int BaudRate, int DataBits, int Parity, int StopBits)
{
    if(serial==NULL)
    {
        QStringList portlist;
        serial = new QSerialPort;
        foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {
            serial->setPort(info);
          //  if(serial->open(QIODevice::ReadWrite))
            {
                portlist<<info.portName();
         //       serial->close();
            }
        }
        serial->close();
        if(portlist.contains(name))
        {
            //设置串口名
            serial->setPortName(name);
            //打开串口
            serial->open(QIODevice::ReadWrite);
            serial->setReadBufferSize(bufLen);
            //设置波特率
            //serial->setBaudRate(ui->BaudBox->currentText().toInt());
            serial->setBaudRate(BaudRate);
            //设置数据位数
            switch(DataBits)
            {
            case 8: serial->setDataBits(QSerialPort::Data8); break;
            case 7: serial->setDataBits(QSerialPort::Data7);break;
            case 6: serial->setDataBits(QSerialPort::Data6);break;
            case 5: serial->setDataBits(QSerialPort::Data5);break;
            default: break;
            }
            //设置奇偶校验
            switch(Parity)
            {
             case 0: serial->setParity(QSerialPort::NoParity); break;
             case 1: serial->setParity(QSerialPort::EvenParity);break;

            default: break;
            }
            //设置停止位
            switch(StopBits)
            {
            case 1: serial->setStopBits(QSerialPort::OneStop); break;
            case 2: serial->setStopBits(QSerialPort::TwoStop); break;
            default: break;
            }
            //设置流控制
     //       serial->setFlowControl(QSerialPort::HardwareControl);
//           serial->waitForReadyRead(20);

            //连接信号槽
            QObject::connect(serial, &QSerialPort::readyRead, this, &Communication::ReadDate);
            return true;
        }
        else
        {
            return false;
        }


    }
    return true;
}

///
/// \brief Communication::WriteDate 写入数据
/// \param buf  要发送的数据
///
void Communication::WriteDate(QByteArray buf)
{
    serial->write(buf);
}
///
/// \brief Communication::Server建立服务器链接
/// \param port  服务器端口号
///
void Communication::Server(quint16 port ,int & quportname)
{
    if(server == NULL)
    {
        server = new QTcpServer(this);
        server->listen(QHostAddress::Any,port);
        connect(server,&QTcpServer::newConnection,   //有新链接过来
                [&](){
            QTcpSocket *com = NULL;
            com = server->nextPendingConnection();
            if(com != NULL)
            {
                 quint16 peerport = com->peerPort();   //获取链接的端口号
                 quportname = peerport;
                 peerportNum = peerport;
                 m_peerip = com->peerAddress().toString();
                 m_peerip = m_peerip.mid(7);
                 if(!socketmap.contains(m_peerip))
                 {
                    socketmap.insert(m_peerip,com);
                 }
                 else
                 {
                     socketmap[m_peerip] = com;
                 }
                 connect(com,&QTcpSocket::readyRead,
                         [=](){
                     QByteArray buff = com->readAll();
                     if(!buff.isEmpty())
                     {
                         peerportNum =  com->peerPort();
                         m_ReadBuff = buff;
                         QString strip = com->peerAddress().toString().mid(7);
                         if(ReadMassage.contains(strip))
                         {
                             ReadMassage[strip] = m_ReadBuff;
                         }
                         else
                         {
                             ReadMassage.insert(strip,m_ReadBuff);
                         }
                         emit TCPResive(buff);
                     }
                     buff.clear();
                 });
            }
        });
    }
}

///
/// \brief Communication::Client 建立客户端
/// \param ip  IPv4地址
/// \param port 端口号
///
bool Communication::Client(QString ip, quint16 port)
{
    bool isconnect = false;
    if(socket == NULL)
    {
        socket = new QTcpSocket(this);
        connect(socket,&QTcpSocket::readyRead,
                [=](){
            QByteArray buff = socket->readAll();
            Client_Read = buff;
            buff.clear();
        });
        socket->connectToHost(QHostAddress(ip),port);
       isconnect = socket->waitForConnected(1000);
    }
    return isconnect;
}


///
/// \brief Communication::SendBuff TCP发送数据
/// \param data 要发送的数据
/// \return  返回发送的的字节数  = -1 说明发送失败
///
int Communication::SendBuff(QByteArray buff)
{
    if(socket == NULL)
    {
        return -1;
    }
//    return socket->write(data);
    int iresult = socket->write(buff);
    return iresult;
}
///
/// \brief Communication::CloseTCP关闭socket
///
void Communication::CloseTCP()
{
    if(socket != NULL)
    {
        socket->disconnectFromHost();
        socket->close();
        delete socket;
        socket = NULL;
    }
    if(server != NULL)
    {
        server->close();
        delete server;
        server = NULL;
    }

}
int Communication::SendBuff_Server(char *data,QString IpAddress)
{
    if(socketmap[IpAddress] != NULL)
    {
       return socketmap[IpAddress]->write(data);
    }
    else
    {
        return -1;
    }
}

QString Communication::changeTohex(char cr[],int lenth)
{
    //   QByteArray buff_read = serial->readAll();
    QByteArray byteArray;
    byteArray.resize(lenth);
    for(int i = 0;i < lenth;i++)
    {
        byteArray[i] = cr[i];
    }
        //显示16进制
        QDataStream out(&byteArray,QIODevice::ReadWrite);    //将字节数组读入
        QString re_str = "";
        while(!out.atEnd())
        {
            qint8 outChar = 0;
            out>>outChar;   //每字节填充一次，直到结束
            QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0'));
           re_str = re_str +str +"";
        }
        return re_str;
}
