#ifndef MYCONFIG_H
#define MYCONFIG_H
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
class My_Config
{
private:
    QString m_qstrFileName;
    QSettings *m_psetting;
public:
    My_Config()
    {

    }
    ~My_Config()
    {
        delete m_psetting;
        m_psetting = 0;
    }
    void Config(QString qstrfilename = "")
    {
        if (qstrfilename.isEmpty())
           {
               m_qstrFileName = QCoreApplication::applicationDirPath() + "/config.ini";
           }
           else
           {
               m_qstrFileName = qstrfilename;
           }

           m_psetting = new QSettings(m_qstrFileName, QSettings::IniFormat);
           qDebug() << m_qstrFileName;
    }
    void Set(QString nodename, QString strkeyname, QVariant qvarvalue)
    {
        m_psetting->setValue(QString("/%1/%2").arg(nodename).arg(strkeyname),qvarvalue);
    }

    QVariant Get(QString qstrnodename, QString qstrkeyname)
    {
        QVariant qvar = m_psetting->value(QString("/%1/%2").arg(qstrnodename).arg(qstrkeyname));
        return qvar;
    }

};

#endif // MYCONFIG_H
