#include "com.h"

Com::Com(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QList<FloatDatas>>("QList<FloatDatas>");

    // 初始化缓冲区
    buffCircle.r_Ptr = 0;      // 读指针
    buffCircle.w_Ptr = 0;      // 写指针
    buffCircle.datalength = 0; // 缓冲区内数据长度
    framecount = 0;            // 每帧数据的计数戳
    errorframecount = 0;       // 错误帧计数

    m_serialPort = new QSerialPort(); // 实例化串口对象
}

FloatDatas Com::unpackData(QByteArray dataframe)
{
    uchar verifyflag = 0;
    FloatDatas datas_float = FloatDatas(); // 实例化数据结构float

    if (dataframe.size() == dataFrameFormat.framebytes)
    {
        for (int i = dataFrameFormat.frameHead2_index + 1; i < dataFrameFormat.verifyFlag_index; i++)
        {
            verifyflag += dataframe[i];
        }
        if ((uchar)verifyflag == (uchar)(dataframe.data()[dataFrameFormat.verifyFlag_index]))
        {
            int dataStart = 0;
            dataStart = sizeof(dataFrameFormat.frameHead1) +
                        sizeof(dataFrameFormat.frameHead2);
            dataFrameFormat.ctrlMode = dataframe[dataStart]; // 控制模式
            // float占4字节数据转换需要颠倒顺序
            for (int i = 0; i < 4; i++)
            {
                dataFrameFormat.data1[i] = dataframe[dataStart + 4 - i];  // 数据1
                dataFrameFormat.data2[i] = dataframe[dataStart + 8 - i];  // 数据2
                dataFrameFormat.data3[i] = dataframe[dataStart + 12 - i]; // 数据3
                dataFrameFormat.data4[i] = dataframe[dataStart + 16 - i]; // 数据4
            }

#if DataTYPE == FIX10_22_TYPE
            // Int定点数字节获取，QbyteArray一定要写上数据长度，否则出现‘/0’会截断数据
            memcpy(&datas_float.fix10_22_char1_1, QByteArray(dataFrameFormat.data1, 4), 4);
            memcpy(&datas_float.fix10_22_char2_1, QByteArray(dataFrameFormat.data2, 4), 4);
            memcpy(&datas_float.fix10_22_char1_2, QByteArray(dataFrameFormat.data3, 4), 4);
            memcpy(&datas_float.fix10_22_char2_2, QByteArray(dataFrameFormat.data4, 4), 4);

            // 【定点数10_22转32位浮点数】
            datas_float.fix10_22_data1_1 = (datas_float.fix10_22_char1_1 / (float)(1 << 22));
            datas_float.fix10_22_data2_1 = (datas_float.fix10_22_char2_1 / (float)(1 << 22));
            datas_float.fix10_22_data1_2 = (datas_float.fix10_22_char1_2 / (float)(1 << 22));
            datas_float.fix10_22_data2_2 = (datas_float.fix10_22_char2_2 / (float)(1 << 22));

            datas_float.float_data1_1 = datas_float.fix10_22_data1_1;
            datas_float.float_data2_1 = datas_float.fix10_22_data2_1;
            datas_float.float_data1_2 = datas_float.fix10_22_data1_2;
            datas_float.float_data2_2 = datas_float.fix10_22_data2_2;

#elif DataTYPE == FLOAT_TYPE

            // NOTE 数据帧解析及记录
            memcpy(&datas_float.float_data1_1, QByteArray(dataFrameFormat.data1, 4), 4);
            memcpy(&datas_float.float_data2_1, QByteArray(dataFrameFormat.data2, 4), 4);
            memcpy(&datas_float.float_data1_4, QByteArray(dataFrameFormat.data3, 4), 4);
            memcpy(&datas_float.float_data2_4, QByteArray(dataFrameFormat.data4, 4), 4);
            // char[]转float，QbyteArray一定要写上数据长度，否则出现‘/0’会截断数据

#endif
            // 解析浮点数据调试显示
            qDebug() << QString("%1").arg(datas_float.float_data1_1, 0, 'f', 10)
                     << QString("%1").arg(datas_float.float_data2_1, 0, 'f', 10)
                     << QString("%1").arg(datas_float.float_data1_4, 0, 'f', 10)
                     << QString("%1").arg(datas_float.float_data2_4, 0, 'f', 10);

            // 解析浮点数据调试显示
            // qDebug()<<datas_float.float_data1_1<<datas_float.float_data2_1<<datas_float.float_data1_4<<datas_float.float_data2_4;
            qDebug() << dataframe.toHex();
            datas_float.framecountTag = ++framecount; // 计数从1开始，等效为时间戳
        }
        else
        {
            ++errorframecount;
            qDebug() << "检验不通过" << dataframe.toHex() << "计数:" << errorframecount;
        }
    }
    return datas_float;
}

void Com::openPort(QString portname)
{
    m_serialPort = new QSerialPort(); // 切换端口重新实例化串口对象

    // 通过主线程传递过来串口名称，开启串口
    m_serialPort->setPortName(portname);
    if (m_serialPort->open(QIODevice::ReadWrite)) // 用ReadWrite 的模式尝试打开串口
    {
        m_serialPort->setBaudRate(QSerialPort::BaudRate(230400), QSerialPort::AllDirections); // 设置波特率和读写方向
        m_serialPort->setDataBits(QSerialPort::Data8);                                        // 数据位为8位
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);                             // 无流控制
        m_serialPort->setParity(QSerialPort::NoParity);                                       // 无校验位
        m_serialPort->setStopBits(QSerialPort::OneStop);                                      // 一位停止位

        // 连接信号槽 当下位机发送数据QSerialPortInfo 会发送个 readyRead 信号,我们定义个槽void receiveInfo()解析数据
        connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(receiveInfo()));
    }
    else
    {
        qDebug() << "串口打开失败";
    }
}

void Com::closePort(QString portname)
{
    if (m_serialPort->portName() == portname)
    {
        if (m_serialPort->isOpen())
        {
            m_serialPort->clear();
            m_serialPort->close();
        }
    }
}

void Com::receiveInfo()
{
    QByteArray info = m_serialPort->readAll(); // 字节流接收，长度不定

    if (buffCircle.datalength < buffCircle.Maxbuff) // 缓冲区未满，可以往里写字节数据，满了则继续进行读取解析，一定保证缓冲区不可满（可修改缓冲区大小）
    {
        for (int i = 0; i < info.size(); i++)
        {
            buffCircle.buffdata[(buffCircle.w_Ptr)] = info[i];
            buffCircle.w_Ptr = (buffCircle.w_Ptr + 1) % buffCircle.Maxbuff; // 写指针移动，循环写入
            buffCircle.datalength++;                                        // 数据长度+1
        }
    }

    recDatalList.clear();
    // 缓冲区内的数据长度大于一帧字节长度
    while (buffCircle.datalength >= dataFrameFormat.framebytes)
    {
        // 验证头、尾，读取数据
        if ((buffCircle.buffdata[(buffCircle.r_Ptr) % buffCircle.Maxbuff] == dataFrameFormat.frameHead1) &&
            (buffCircle.buffdata[(buffCircle.r_Ptr + 1) % buffCircle.Maxbuff] == dataFrameFormat.frameHead2) &&
            (buffCircle.buffdata[(buffCircle.r_Ptr + dataFrameFormat.framebytes - 2) % buffCircle.Maxbuff] == dataFrameFormat.frameTail1) &&
            (buffCircle.buffdata[(buffCircle.r_Ptr + dataFrameFormat.framebytes - 1) % buffCircle.Maxbuff] == dataFrameFormat.frameTail2))
        {
            // **********获取数据************
            QByteArray framedata;
            for (int i = 0; i < dataFrameFormat.framebytes; i++)
            {
                framedata += buffCircle.buffdata[(buffCircle.r_Ptr) % buffCircle.Maxbuff];
                buffCircle.r_Ptr = (buffCircle.r_Ptr + 1) % buffCircle.Maxbuff;
                buffCircle.datalength--;
            }
            FloatDatas data = unpackData(framedata);
            recDatalList.append(data);
        }
        else
        {
            buffCircle.r_Ptr = (buffCircle.r_Ptr + 1) % buffCircle.Maxbuff;
            buffCircle.datalength--;
        }
    }
    if (recDatalList.size() != 0)
    {
        emit sendfloats(recDatalList);
    }
}

void Com::sendbytedata(QByteArray frameToSendBytes)
{
    if (m_serialPort->isOpen())
    {
        m_serialPort->write(frameToSendBytes);
    }
    else
    {
        QMessageBox::information(NULL, "提示", "未打开串口，请打开");
    }
}
