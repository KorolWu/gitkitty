#include "modbustcpclient.h"

ModbusTcpClient::ModbusTcpClient(QObject *parent) : QObject(parent)
{
    modbusDevice = nullptr;
    bit_map.clear();
    for(int i = 0;i<20;i++)
    {
        bit_map.insert(i,0);
    }
}

bool ModbusTcpClient::GetConnect(const QString url_str)
{
    if(modbusDevice != nullptr)
        return false;

    modbusDevice = new QModbusTcpClient(this);
    connect(modbusDevice, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) {
    qDebug()<<(modbusDevice->errorString(), 5000);
    });
    if(modbusDevice->state() != QModbusDevice::ConnectedState)
    {
        QUrl url = QUrl::fromUserInput(url_str);
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, url.port());
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, url.host());

        modbusDevice->setTimeout(500);
        modbusDevice->setNumberOfRetries(3);
        if (!modbusDevice->connectDevice()) {
            qDebug()<<(tr("Connect failed: ") + modbusDevice->errorString(), 5000);
            return false;
        } else {
           qDebug()<<(tr("Connect successful"));
           emit connect_success_signal();
           return true;
        }
    }

    return true;
}
///
/// \brief ModbusTcpClient::write 写入线圈的值
/// \param start_bit  start_bit
/// \param value  start_bit_value
/// \param address server_address
///
void ModbusTcpClient::write(int start_bit, int value, int address)
{
    QMutexLocker locker(&modbus_mutex);
    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd:hh:mm:ss.zzz")<< " write get clock";

    if (!modbusDevice)
        return;

    QModbusDataUnit writeUnit(QModbusDataUnit::RegisterType::Coils,start_bit,1);
    QModbusDataUnit::RegisterType table = writeUnit.registerType();
    for (uint i = 0; i < writeUnit.valueCount(); i++) {
        if (table == QModbusDataUnit::Coils)
            writeUnit.setValue(i, value);
        else
            writeUnit.setValue(i, value);
    }

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, address)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() == QModbusDevice::ProtocolError) {
                    qDebug()<<(tr("Write response error: %1 (Mobus exception: 0x%2)")
                        .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16),
                        5000);
                } else if (reply->error() != QModbusDevice::NoError) {
                    qDebug()<<(tr("Write response error: %1 (code: 0x%2)").
                        arg(reply->errorString()).arg(reply->error(), -1, 16), 5000);
                }
                reply->deleteLater();
            });
        } else {
            // broadcast replies return immediately
            reply->deleteLater();
        }
    } else {
        qDebug()<<(tr("Write error: ") + modbusDevice->errorString(), 5000);
    }
}

void ModbusTcpClient::readCoils(int start_bit, int address)
{
    QMutexLocker locker(&modbus_mutex);
//    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd:hh:mm:ss.zzz")<< " begin read time  readCoils get clock";

    if (!modbusDevice)
        return;
    if (auto *reply = modbusDevice->sendReadRequest(QModbusDataUnit(QModbusDataUnit::RegisterType::Coils,start_bit,address), 1)) {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, &ModbusTcpClient::readReady);
        }
        else
            delete reply; // broadcast replies return immediately
    } else {
        qDebug()<<(tr("Read error: ") + modbusDevice->errorString(), 5000);
    }
}

void ModbusTcpClient::readAll()
{
//    if (!modbusDevice)
//        return;
//    QModbusDataUnit data_unit;
//    data_unit.setRegisterType(QModbusDataUnit::RegisterType::Coils);
//    for(int i = 0;i <4;i++)
//    {

//    }
//    if (auto *reply = modbusDevice->sendReadRequest(QModbusDataUnit(QModbusDataUnit::RegisterType::Coils,start_bit,1), address)) {
//        if (!reply->isFinished())
//            connect(reply, &QModbusReply::finished, this, &ModbusTcpClient::readReady);
//        else
//            delete reply; // broadcast replies return immediately
//    } else {
//        qDebug()<<(tr("Read error: ") + modbusDevice->errorString(), 5000);
//    }
}

void ModbusTcpClient::readRegisters(int start_bit, int adress)
{
    QMutexLocker locker(&modbus_mutex);
    if (!modbusDevice)
        return;

    if (auto *reply = modbusDevice->sendReadRequest(QModbusDataUnit(QModbusDataUnit::RegisterType::InputRegisters,start_bit,1), adress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusTcpClient::readReady);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qDebug()<<(tr("Read error: ") + modbusDevice->errorString(), 5000);
    }

}

void ModbusTcpClient::readReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;
    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        for (uint i = 0; i < unit.valueCount(); i++) {
            const QString entry = tr("Address: %1, Value: %2").arg(unit.startAddress() + i)
                                     .arg(QString::number(unit.value(i),
                                          unit.registerType() <= QModbusDataUnit::Coils ? 10 : 16));
            if(!bit_map.contains(unit.startAddress()))
            {
                bit_map.insert(unit.startAddress()+i,unit.value(i));
            }
            else
                bit_map[unit.startAddress()+i] = unit.value(i);
            //qDebug()<<entry;
        }
    } else if (reply->error() == QModbusDevice::ProtocolError) {
        qDebug()<<(tr("Read response error: %1 (Mobus exception: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->rawResult().exceptionCode(), -1, 16), 5000);
    } else {
         qDebug()<<(tr("Read response error: %1 (code: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->error(), -1, 16), 5000);
    }
    //qDebug()<<"bit_map : "<<bit_map;
    //qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd:hh:mm:ss.zzz")<< " end read time";
    reply->deleteLater();
}

int ModbusTcpClient::getBitValue(int start_bit)
{
    //QMutexLocker locker(&mutex);
    if(bit_map.contains(start_bit))
    {
       // qDebug()<<"getbitvalue=> bit "<<start_bit<<" = " <<bit_map[start_bit];
        return bit_map[start_bit];
    }
    else
        return -1;
}

void ModbusTcpClient::init_bitMap()
{
    readCoils(0,20);

}
