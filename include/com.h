#ifndef COM_H
#define COM_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QMetaType>
#include <QMessageBox>

#define FLOAT_TYPE 0U
#define FIX10_22_TYPE 1U
#define DataTYPE FLOAT_TYPE // 选择数据格式【浮点型 或 定点型10:22】

// 环形缓冲区结构
struct buffZone
{
    const static int Maxbuff = 6000; // 环形缓冲区大小
    int w_Ptr, r_Ptr;                // 读写指针
    uchar buffdata[Maxbuff] = {0};   // 环形缓冲区
    int datalength;                  // 缓冲区内存在的数据长度
    buffZone() {}
};

// NOTE 定义接收帧数据格式
struct FrameFormat
{
    int framebytes = 23; // 接收数据全帧字节长度

    // 数据帧格式
    uchar frameHead1 = 0xEB;
    uchar frameHead1_index = 0; // 帧头1及帧内位置
    uchar frameHead2 = 0x90;
    uchar frameHead2_index = 1; // 帧头2及帧内位置
    uchar ctrlMode = 0;
    uchar ctrlMode_index = 2; // 控制模式及帧内位置

    // uchar转QByteArray有障碍 本质二进制相同不影响float转换
    char data1[4] = {0, 0, 0, 0}; // 数据1
    char data2[4] = {0, 0, 0, 0}; // 数据2
    char data3[4] = {0, 0, 0, 0}; // 数据3
    char data4[4] = {0, 0, 0, 0}; // 数据4

    uchar errorFlag = 0;
    uchar errorFlag_index = 19; // 故障标志及帧内位置
    uchar verifyFlag = 0;
    uchar verifyFlag_index = 20; // 校验位及帧内位置
    uchar frameTail1 = 0x0D;
    uchar frameTail1_index = 21; // 帧尾1及帧内位置
    uchar frameTail2 = 0x0A;
    uchar frameTail2_index = 22; // 帧尾2及帧内位置

    FrameFormat() {}
};

struct FrameToSendFormat
{
    int framebytes = 24; // 发送帧数据长度

    // 数据帧格式
    uchar frameHead1 = 0xEB; // 帧头1
    uchar frameHead2 = 0x90; // 帧头2
    uchar databytes = 0x12;  // 有效数据长度
    uchar FrameCount = 0x00; // 帧计数

    uchar ctrlMode = 0; // 控制模式

    // uchar转QByteArray有障碍 本质二进制相同不影响float转换
    QByteArray data1 = QByteArray("\x00\x00\x00\x00", 4); // 目镜角度 后端编码器
    QByteArray data2 = QByteArray("\x00\x00\x00\x00", 4); // 物镜角度 前端编码器
    QByteArray data3 = QByteArray("\x00\x00\x00\x00", 4); // 目镜速度
    QByteArray data4 = QByteArray("\x00\x00\x00\x00", 4); // 物镜速度

    uchar verifyFlag = 0;    // 校验位
    uchar frameTail1 = 0x0D; // 帧尾1
    uchar frameTail2 = 0x0A; // 帧尾2

    FrameToSendFormat() {}
};

// 数据结构
struct FloatDatas
{
    int framecountTag = 0;

    // 上图表数据 浮点数
    uchar float_char1_1[4];
    float float_data1_1 = 0.0;
    uchar float_char1_2[4];
    float float_data1_2 = 0.0;
    uchar float_char1_3[4];
    float float_data1_3 = 90.0;
    uchar float_char1_4[4];
    float float_data1_4 = 30.0;
    uchar float_char1_5[4];
    float float_data1_5 = -30.0;
    uchar float_char1_6[4];
    float float_data1_6 = -90.0;

    // 定点数
    int fix10_22_char1_1;
    float fix10_22_data1_1;
    int fix10_22_char1_2;
    float fix10_22_data1_2;
    int fix10_22_char1_3;
    float fix10_22_data1_3;
    int fix10_22_char1_4;
    float fix10_22_data1_4;
    int fix10_22_char1_5;
    float fix10_22_data1_5;
    int fix10_22_char1_6;
    float fix10_22_data1_6;

    // 下图表数据 浮点数
    uchar float_char2_1[4];
    float float_data2_1 = 0.0;
    uchar float_char2_2[4];
    float float_data2_2 = 0.0;
    uchar float_char2_3[4];
    float float_data2_3 = 90.0;
    uchar float_char2_4[4];
    float float_data2_4 = 30.0;
    uchar float_char2_5[4];
    float float_data2_5 = -30.0;
    uchar float_char2_6[4];
    float float_data2_6 = -90.0;

    // 定点数
    int fix10_22_char2_1;
    float fix10_22_data2_1;
    int fix10_22_char2_2;
    float fix10_22_data2_2;
    int fix10_22_char2_3;
    float fix10_22_data2_3;
    int fix10_22_char2_4;
    float fix10_22_data2_4;
    int fix10_22_char2_5;
    float fix10_22_data2_5;
    int fix10_22_char2_6;
    float fix10_22_data2_6;

    FloatDatas() {}
};

class Com : public QObject
{
    Q_OBJECT
public:
    explicit Com(QObject *parent = nullptr);

    QSerialPort *m_serialPort;      // 实例化串口
    buffZone buffCircle;            // 实例化环形缓冲区
    FrameFormat dataFrameFormat;    // 实例化数据帧格式
    QList<FloatDatas> recDatalList; // 一次接收到的数据

    int framecount; // 帧计数
    int errorframecount;

public:
    FloatDatas unpackData(QByteArray dataframe); // 解析一帧数据

signals:
    void sendfloats(QList<FloatDatas> floatdatas);

public slots:
    void openPort(QString portname);  // 接收主线程打开串口的信号，槽函数
    void closePort(QString portname); // 接收主线程关闭串口的信号，槽函数
    void receiveInfo();               // 接收串口已准备好数据信号，槽函数

    void sendbytedata(QByteArray frameToSendBytes); // 发送帧数据
};

#endif // COM_H
