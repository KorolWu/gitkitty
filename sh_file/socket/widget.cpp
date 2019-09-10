#include "widget.h"
#include "ui_widget.h"#include <QDebug>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    socket = nullptr;
    m_on_color = "background-color:rgb(0,255,0)";
    m_off_color = "background-color:rgb(255,0,0)";
    getIniFile("");
    stop_flag = true;
    run_flag = true;
    emit_run = true;
    read_times = 0;
    bool Connect = CreatClient(ip,port);
   if(Connect)
   {
       ui->textEdit->append("connect success");
       qDebug()<<"connect success!";
   }
   else
   {
        ui->textEdit->append("connect failed");
       qDebug()<<"faild connect";
   }
    _uploadManager = new QNetworkAccessManager(this);
     connect(_uploadManager,&QNetworkAccessManager::finished,this,&Widget::replyFinished);

     timer = new QTimer(this);
     connect(timer,&QTimer::timeout,this,&Widget::RefreshIo);
     modbus = new ModbusTcpClient();
     connect(modbus,&ModbusTcpClient::connect_success_signal,this,&Widget::start_timer);
     if(modbus->GetConnect(modbus_url))
     {
         qDebug()<<"modbus connect successfull";
     }
     else
         qDebug()<<"modbus connect fail";
//     QElapsedTimer et;
//     et.start();
//     while(et.elapsed()<3000)
//         QCoreApplication::processEvents();
//     run_main();
     connect(this,&Widget::destroyed,this,&Widget::colse_handle);
     timer_run_main = new QTimer(this);
     connect(timer_run_main,&QTimer::timeout,this,&Widget::run_main);
     this->setWindowFlags(Qt::FramelessWindowHint);
}

Widget::~Widget()
{
    timer->stop();
    timer->deleteLater();
    delete socket;
    modbus->deleteLater();
    delete ui;
}
///
/// \brief Widget::HandleMessage 接收到的卡片信息
///
void Widget::HandleMessage()
{
    QByteArray message = socket->readAll();
    if(message == tcp_read_message)
    {
        read_times++;
    }
    else
    {
        tcp_read_message = message;
    }

    QByteArray HexStr = message.toHex();
//    StringToHex(message,HexStr);
    qDebug()<<"HexStr Size :"<<HexStr.size();
    ui->textEdit->clear();
    QByteArray show_byte = HexStr.mid(15,13);

    ui->textEdit->append(show_byte);
    qDebug()<<HexStr ;

}

bool Widget::CreatClient(QString ip,int port)
{
     bool isconnect = false;
        if(socket == NULL)
        {
            socket = new QTcpSocket(this);
            connect(socket,&QTcpSocket::readyRead,this,&Widget::HandleMessage);
            connect(socket,&QTcpSocket::disconnected,this,&Widget::TcpClientDisconnect);
            socket->connectToHost(QHostAddress(ip),port);
            isconnect = socket->waitForConnected(8000);
            qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd:hh:mm:ss.zzz")<<" TcpClientConnect";
        }
        return isconnect;

}
void Widget::StringToHex(QString str, QByteArray &senddata)
{
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len/2);
    char lstr,hstr;
    for(int i=0; i<len; )
    {
        //char lstr,
        hstr=str[i].toLatin1();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toLatin1();
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    senddata.resize(hexdatalen);
}
char Widget::ConvertHexChar(char ch)
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    // else return (-1);
    else return ch-ch;//不在0-f范围内的会发送成0
}

void Widget::replyFinished(QNetworkReply *reply)
{
    QString ResiveStr = "";
    if(reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();  //获取字节
       // QString result(bytes);  //转化为字符串
        ResiveStr = bytes;
        qDebug()<<"Server reply: "<<ResiveStr;
        QStringList resive_list = ResiveStr.split(":");
        QString result = QString (resive_list.last());
        if(result.contains("0"))
        {
           qDebug()<<"send for server ok";
//           ui->pushButton->setAutoFillBackground(true);
//           ui->pushButton->setFlat(true);
//           QPalette palette = ui->pushButton->palette();
//           palette.setColor(QPalette::Window,Qt::red);
//           ui->pushButton->setPalette(palette);
           ui->pushButton->setStyleSheet("background-color:rgb(0,255,0)");
        }
        else
            ui->pushButton->setStyleSheet("background-color:rgb(255,0,0)");

    }
    else{
       ResiveStr = reply->errorString();
       ui->pushButton->setStyleSheet("background-color:rgb(255,0,0)");
    }
   // m_resiveflag = true;
    QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    qDebug()<<"reply finished status_Code "<<status_code;
}
///
/// \brief Widget::postServer
/// \param url
/// \param message
///
void Widget::postServer(QUrl url, QString message)
{
    json.insert("lotId",message);
    docment.setObject(json);
    QByteArray buf = docment.toJson(QJsonDocument::Compact);
    //http://192.168.188.26:8080
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    //request.setRawHeader("filename", uploadfilename.section('/', -1, -1).toUtf8() );
    _reply = _uploadManager->post(request,buf);
    QVariant status_code = _reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    qDebug()<<"post status_Code "<<status_code;
    //get status ? save : notsave
//    if(status_code == 400)
//    {
//        SaveTxt("","");
//    }
}
///
/// \brief Widget::SaveTxt
/// \param path
/// \param str
///
void Widget::SaveTxt(QString path,QString str)
{
    QFile file(path);
    bool isok = file.open(QIODevice::Append);
    if(true == isok)
    {
        QByteArray buf = str.toLatin1();
        buf += '/n';
        file.write(buf);
    }
    file.close();
}
///
/// \brief Widget::ReadTxt 断开链接时 读取存储的文件重新上传
/// \param path
///
void Widget::ReadTxt(QString path)
{

}

void Widget::getIniFile(QString path)
{
    My_Config myconfig ;
    myconfig.Config(path);
    qDebug()<<"ip:  "<<myconfig.Get("socket","ip").toString();
    qDebug()<<"port:  "<<myconfig.Get("socket","port").toInt();
    qDebug()<<"url:  "<<myconfig.Get("http","url").toString();
    qDebug()<<"path:  "<<myconfig.Get("file","path").toString();

    ip = myconfig.Get("socket","ip").toString();
    port = myconfig.Get("socket","port").toInt();
    url_str = myconfig.Get("http","url").toString();
    modbus_url = myconfig.Get("modbus","url").toString();

}

void Widget::RefreshIo()
{
    modbus->init_bitMap();
    modbus->getBitValue(0) == 1?ui->button_DI1->setStyleSheet(m_on_color):ui->button_DI1->setStyleSheet(m_off_color);
    modbus->getBitValue(1) == 1?ui->button_DI2->setStyleSheet(m_on_color):ui->button_DI2->setStyleSheet(m_off_color);
    modbus->getBitValue(2) == 1?ui->button_DI3->setStyleSheet(m_on_color):ui->button_DI3->setStyleSheet(m_off_color);
    modbus->getBitValue(3) == 1?ui->button_DI4->setStyleSheet(m_on_color):ui->button_DI4->setStyleSheet(m_off_color);

    modbus->getBitValue(16) == 1?ui->button_DO1->setStyleSheet(m_on_color):ui->button_DO1->setStyleSheet(m_off_color);
    modbus->getBitValue(17) == 1?ui->button_DO2->setStyleSheet(m_on_color):ui->button_DO2->setStyleSheet(m_off_color);
    modbus->getBitValue(18) == 1?ui->button_DO3->setStyleSheet(m_on_color):ui->button_DO3->setStyleSheet(m_off_color);
    modbus->getBitValue(19) == 1?ui->button_DO4->setStyleSheet(m_on_color):ui->button_DO4->setStyleSheet(m_off_color);

    if(true == emit_run)
    {
        emit_run = false;
        timer_run_main->start(100);
    }
}

void Widget::run_main()
{
    emit_run = false;
    // all output off
    for(int i = 16;i< 20;i++)
    {
        modbus->write(i,0);
        QCoreApplication::processEvents();
             QElapsedTimer et;
             et.start();
             while(et.elapsed()<30)
                 QCoreApplication::processEvents();
    }
    while(stop_flag)
    {
        while(stop_flag)
        {
            if(modbus->getBitValue(0) == 1)
            {
                struct timeval tpStart,tpEnd;
                float timeUse = 0;
                gettimeofday(&tpStart,NULL);
                qDebug()<<"wait DI 2";
                read_times = 0;
                while(timeUse < FRIST_READ)
                {
                    if(modbus->getBitValue(1) == 1)
                    {
                        modbus->write(17,1);
                        break;
                    }
                    if(read_times >= 5)
                    {
                        qDebug()<<"the same message read five times";
                        modbus->write(18,1);
                        read_times = 0;
                        QUrl url(url_str);
                        postServer(url,"HexStr");
                        break;
                    }
                    gettimeofday(&tpEnd,NULL);
                    timeUse = 1000 *(tpEnd.tv_sec - tpStart.tv_sec) + 0.001*(tpEnd.tv_usec - tpStart.tv_usec);
                    QCoreApplication::processEvents();
                    if(timeUse >= FRIST_READ)
                       break;
                }
                modbus->write(16,1);
                break;
            }
            QThread::msleep(10);
            QCoreApplication::processEvents();
        }
        while(stop_flag)
        {
            if(modbus->getBitValue(0) == 0)
            {
                modbus->write(16,0);
                modbus->write(17,0);
                modbus->write(18,0);
                modbus->write(19,0);
                break;
            }
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        QCoreApplication::processEvents();
        QThread::msleep(30);
        for(int j = 16;j< 20;j++)
        {
            modbus->write(j,1);
            QCoreApplication::processEvents();
            QThread::msleep(30);
        }
        QElapsedTimer et;
        et.start();
        while(et.elapsed()<3000)
            QCoreApplication::processEvents();
        qDebug()<<"all off";

    }
}

void Widget::start_timer()
{
    timer->start(100);
}

void Widget::colse_handle()
{
    modbus->disconnect();
    socket->disconnect();
    stop_flag = false;
    timer_run_main->stop();
    this->close();
    this->deleteLater();
}

void Widget::TcpClientDisconnect()
{
    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd:hh:mm:ss.zzz")<<" TcpClientDisconnect";
}

void Widget::on_readbutton_clicked()
{
    modbus->disconnect();
    socket->disconnect();
    stop_flag = false;
    timer_run_main->stop();
    this->close();
    this->deleteLater();
}

void Widget::on_writebutton_clicked()
{
    modbus->write(ui->spinBox->value(),ui->spinBox_2->value());
}

void Widget::on_readbutton_2_clicked()
{
    modbus->readCoils(0,19);
}
