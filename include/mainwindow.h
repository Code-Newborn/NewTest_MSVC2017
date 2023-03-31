#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <fundamental_window.h>
#include <advanced_window.h>
#include <qcustomplot.h>
#include <com.h>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QtDebug>
#include <QDoubleValidator>

#include "guicamera.h"

#define dataRecvFreq 200 // NOTE 数据接收频率
#define dataSendFreq 200 // NOTE 数据发送频率

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initialPlot(QCustomPlot *plot, int low_limit, int up_limit); // 初始化图表
    void scanPort();                                                  // 检索串口
    uchar *floatTo4Bytes(float floatdata);                            // 浮点数转4字节
    void curveLegendSet(QLabel *curvelegend, int curveIndex);

    void initialValidator();

    int EnumerateDevices();                            // 枚举相机
    void MainWindow::ShowWarning(QString warningText); // 显示警告
    void UpdateCameraDialog(int cameraId);             // 更新和相机有关的控件
    bool InternalOpenCamera(const Pylon::CDeviceInfo &devInfo, int cameraId);
    void InternalCloseCamera(int cameraId);
    void UpdateSlider(QSlider *pCtrl, Pylon::IIntegerEx &integerParameter);
    void UpdateEnumeration(QComboBox *pCtrl, Pylon::IEnumerationEx &enumParameter);
        void ClearSlider(QSlider *pCtrl, QLabel *pString);
        void ClearEnumeration(QComboBox *pCtrl);

public:
    QList<FloatDatas> datalist; // 全部数据帧存储
    int showPtr;                // 显示指针
    int dataPtr;                // 数据指针

    // 曲线图例样式表
    const QString m_bulue_SheetStyle = "min-width:40px; min-height:4px; max-width:40px; max-height:4px; border:1px solid black; border-radius:2px; background:rgb(74,144,226)";   // 蓝色
    const QString m_red_SheetStyle = "min-width:40px; min-height:4px; max-width:40px; max-height:4px; border:1px solid black; border-radius:2px; background:rgb(230,0,57)";       // 红色
    const QString m_yellow_SheetStyle = "min-width:40px; min-height:4px; max-width:40px; max-height:4px; border:1px solid black; border-radius:2px; background:rgb(255,255,36)";  // 黄色
    const QString m_green_SheetStyle = "min-width:40px; min-height:4px; max-width:40px; max-height:4px; border:1px solid black; border-radius:2px; background:rgb(2,179,64)";     // 绿色
    const QString m_orange_SheetStyle = "min-width:40px; min-height:4px; max-width:40px; max-height:4px; border:1px solid black; border-radius:2px; background:rgb(254,98,31)";   // 橙色
    const QString m_purple_SheetStyle = "min-width:40px; min-height:4px; max-width:40px; max-height:4px; border:1px solid black; border-radius:2px; background:rgb(230,102,255)"; // 紫色

    // 串口开关状态显示样式表
    const QString port_close_SheetStyle = "min-width:60px; min-height:20px; max-width:60px; max-height:20px; border:none; border-radius:5px; background:rgb(231,76,60)"; // 红色：串口关闭
    const QString port_open_SheetStyle = "min-width:60px; min-height:20px; max-width:60px; max-height:20px; border:none; border-radius:5px; background:rgb(118,255,3)";  // 绿色：串口打开

    // 子窗口
    Fundamental_Window *Fwindow;
    Advanced_Window *Awindow;

private:
    QTimer *refreshTimer;                // 刷新计时器
    QTimer *sendTimer;                   // 发送计时器
    int sendFrameCount = 1;              // 发送计数器
    FrameToSendFormat frameToSendFormat; // 发送数据帧

    char controlMode_toSend; // 待发送的控制模式指令
    float data1_toSend;      // 待发送的数据指令1
    float data2_toSend;      // 待发送的数据指令2
    float data3_toSend;      // 待发送的数据指令3
    float data4_toSend;      // 待发送的数据指令4

signals:
    void openSerial(QString portname);          // 打开串口 信号
    void closeSerial(QString portname);         // 关闭串口 信号
    void sendFrameData(QByteArray frameToSend); // 向子线程发送串口数据 信号

public slots:
    void recvfloats(QList<FloatDatas> recvDatasList); // 收到子线程解析的数据 槽函数

    void sendFrame();  // 发送帧数据
    void plotUpdate(); // 主线程曲线添加显示数据 槽函数

private slots:
    void on_pushButton_scanPort_clicked();
    void on_pushButton_openPort_clicked();
    void on_pushButton_saveData_clicked();
    void on_comboBox_chart1_1_currentIndexChanged(int index);
    void on_comboBox_chart1_2_currentIndexChanged(int index);
    void on_comboBox_chart2_1_currentIndexChanged(int index);
    void on_comboBox_chart2_2_currentIndexChanged(int index);
    void on_pushButton_fundamental_clicked();
    void on_pushButton_advanced_clicked();

    void on_pushButton_send_clicked();

    void on_pushButton_stopSend_clicked();

    void on_scanButton_clicked();

    void on_cameraList_currentIndexChanged(int index);

    void on_openSelected_clicked();

    void on_continuous_1_clicked();

    // Slots for GuiCamera signals
    void OnNewGrabResult(int userHint);

    void on_stop_1_clicked();

    void on_close_1_clicked();

protected:
    //    virtual void showEvent(QShowEvent *event) override;
    virtual void paintEvent(QPaintEvent *) override;

private:
    Ui::MainWindow *ui;
    QThread *subThread;
    Com *readwriteCom;

    Pylon::DeviceInfoList_t m_devices; // Pylon相机列表
    static const int MaxCamera = 1;    // 最大相机数
    CGuiCamera m_camera[MaxCamera];    // 相机操作类列表
};
#endif // MAINWINDOW_H
