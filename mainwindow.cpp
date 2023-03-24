#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "com.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    curveLegendSet(ui->curvelegend1_1, ui->comboBox_chart1_1->currentIndex());
    curveLegendSet(ui->curvelegend1_2, ui->comboBox_chart1_2->currentIndex() + 3);
    curveLegendSet(ui->curvelegend2_1, ui->comboBox_chart2_1->currentIndex());
    curveLegendSet(ui->curvelegend2_2, ui->comboBox_chart2_2->currentIndex() + 3);

    Fwindow = new Fundamental_Window;
    Fwindow->setFocusPolicy(Qt::StrongFocus);
    Awindow = new Advanced_Window;

    // 初始化图表
    initialPlot(ui->chartplot1, -360, 360);
    initialPlot(ui->chartplot2, -360, 360);

    // 初始化输入框验证器
    initialValidator();

    subThread = new QThread();             // 创建子线程
    readwriteCom = new Com();              // 创建操作对象
    readwriteCom->moveToThread(subThread); // 将操作对象移入子线程中
    subThread->start();                    // 开启线程 (readwriteCom归属于子线程)

    refreshTimer = new QTimer(); // 界面刷新定时器 (归属于主线程)
    sendTimer = new QTimer();    // 命令发送定时器 (归属于主线程)
    scanPort();                  // 检索串口

    // ***************主界面 信号和槽的连接***************
    // 线程间访问，信号槽实现
    connect(this, &MainWindow::openSerial, readwriteCom, &Com::openPort, Qt::BlockingQueuedConnection);   // 主线程发送“打开串口”，子线程接收“打开串口”，直连确定串口已经打开
    connect(this, &MainWindow::closeSerial, readwriteCom, &Com::closePort, Qt::BlockingQueuedConnection); // 主线程发送“关闭串口”，子线程接收“关闭串口”，直连确定串口已经关闭
    connect(readwriteCom, &Com::sendfloats, this, &MainWindow::recvfloats, Qt::QueuedConnection);         // 子线程发送“发送数据”，主线程接收“接收数据”
    connect(this, SIGNAL(sendFrameData(QByteArray)), readwriteCom, SLOT(sendbytedata(QByteArray)), Qt::QueuedConnection);

    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(plotUpdate())); // 定时刷新数据显示
    connect(sendTimer, SIGNAL(timeout()), this, SLOT(sendFrame()));     // 定时发送帧数据
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initialPlot(QCustomPlot *plot, int low_limit, int up_limit)
{
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);            // 允许拖动和缩放
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime); // 曲线X、Y轴刻度显示格式
    timeTicker->setTimeFormat("%m:%s:%z");

    plot->xAxis->setTicker(timeTicker);
    plot->yAxis->setRange(low_limit, up_limit); // 设置Y轴显示范围
    plot->axisRect()->setupFullAxesBox();       // 图表四周均显示轴刻度

    plot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine)); // 网格线(对应刻度)画笔
    plot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    plot->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine)); // 子网格线(对应子刻度)画笔
    plot->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    plot->xAxis->grid()->setSubGridVisible(true); // 显示子网格线
    plot->yAxis->grid()->setSubGridVisible(true);
    plot->axisRect()->setBackground(QColor(62, 64, 63)); // 设置图表背景颜色

    // 曲线0  编码器角度
    plot->addGraph();
    plot->graph(0)->setPen(QPen(QColor(74, 144, 226)));                                  // 蓝色
    plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCrossCircle, 5)); // 0曲线点标记样式
    // 曲线1  编码器角速度
    plot->addGraph();
    plot->graph(1)->setPen(QPen(QColor(230, 0, 57)));                               // 红色
    plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 5)); // 1曲线点标记样式
    // 曲线2  陀螺角速度
    plot->addGraph();
    plot->graph(2)->setPen(QPen(QColor(255, 255, 36)));                            // 黄色
    plot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 5)); // 2曲线点标记样式
    // 曲线3  编码器角度输入
    plot->addGraph();
    plot->graph(3)->setPen(QPen(QColor(2, 179, 64)));                                    // 绿色
    plot->graph(3)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCrossSquare, 5)); // 3曲线点标记样式
    // 曲线4  编码器角速度输入
    plot->addGraph();
    plot->graph(4)->setPen(QPen(QColor(254, 98, 31)));                               // 橙色
    plot->graph(4)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDiamond, 5)); // 4曲线点标记样式
    // 曲线5  陀螺角速度输入
    plot->addGraph();
    plot->graph(5)->setPen(QPen(QColor(230, 102, 255)));                              // 紫色
    plot->graph(5)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssTriangle, 5)); // 5曲线点标记样式

    // 默认显示曲线0、1
    plot->graph(0)->setVisible(true); // 数据组1
    plot->graph(1)->setVisible(false);
    plot->graph(2)->setVisible(false);

    plot->graph(3)->setVisible(true); // 数据组2
    plot->graph(4)->setVisible(false);
    plot->graph(5)->setVisible(false);

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), plot->yAxis2, SLOT(setRange(QCPRange)));
}

void MainWindow::scanPort()
{
    // 检索可用的串口
    ui->comboBox_portNames->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QSerialPort serial;
        serial.setPort(info);
        if (serial.open(QIODevice::ReadWrite))
        {
            qDebug() << "serialPortName:" << serial.portName();
            ui->comboBox_portNames->addItem(serial.portName()); // 下拉框中添加串口名称
            serial.close();
        }
    }
}

// NOTE 浮点数转4字节
uchar *MainWindow::floatTo4Bytes(float floatdata)
{
    QByteArray floatBytesArray;
    floatBytesArray.resize(sizeof(floatdata));
    uchar float4uchar[4];
    memcpy(floatBytesArray.data(), &floatdata, sizeof(floatdata));
    QString hexString = floatBytesArray.toHex();
    for (int i = 0; i < hexString.size() / 2; i++)
    {
        float4uchar[i] = hexString.mid(i * 2, 2).toUInt(NULL, 16);
    }

    uchar *temp = new uchar[sizeof(floatdata)];
    for (unsigned int i = 0; i < sizeof(floatdata); i++)
        temp[i] = float4uchar[i];
    return temp;
}

void MainWindow::curveLegendSet(QLabel *curvelegend, int curveIndex)
{
    switch (curveIndex)
    {
    case 0:
        curvelegend->setStyleSheet(m_bulue_SheetStyle); // 曲线0蓝色
        break;
    case 1:
        curvelegend->setStyleSheet(m_red_SheetStyle); // 曲线1红色
        break;
    case 2:
        curvelegend->setStyleSheet(m_yellow_SheetStyle); // 曲线2黄色
        break;
    case 3:
        curvelegend->setStyleSheet(m_green_SheetStyle); // 曲线3绿色
        break;
    case 4:
        curvelegend->setStyleSheet(m_orange_SheetStyle); // 曲线4橙色
        break;
    case 5:
        curvelegend->setStyleSheet(m_purple_SheetStyle); // 曲线5紫色
        break;
    default:
        curvelegend->setText("显示错误");
        break;
    }
}

void MainWindow::initialValidator()
{
    QDoubleValidator *pos_validator = new QDoubleValidator(-999, 999, 3, this); // 0到360度
    pos_validator->setNotation(QDoubleValidator::StandardNotation);
    QDoubleValidator *vel_validator = new QDoubleValidator(-999, 999, 3, this); // 0到360度
    vel_validator->setNotation(QDoubleValidator::StandardNotation);
    ui->lineEdit_motor1pos->setValidator(pos_validator);
    ui->lineEdit_motor2pos->setValidator(pos_validator);
    ui->lineEdit_motor1vel->setValidator(vel_validator);
    ui->lineEdit_motor2vel->setValidator(vel_validator);
}

void MainWindow::recvfloats(QList<FloatDatas> recvDatasList)
{
    foreach (FloatDatas floatdatas, recvDatasList)
    {
        datalist.append(floatdatas); // 向列表中添加数据结构体(每一个数据附有计数戳)
        dataPtr++;
    }
}

void MainWindow::sendFrame()
{
    // NOTE 定时发送指令数据
    if ((ui->comboBox_mode->currentIndex() >= 0) && (readwriteCom->m_serialPort->isOpen())) // 串口打开了
    {
        frameToSendFormat.ctrlMode = controlMode_toSend;

        memcpy(frameToSendFormat.data1.data(), &data1_toSend, sizeof(data1_toSend)); // float转4字节QByteArray
        memcpy(frameToSendFormat.data2.data(), &data2_toSend, sizeof(data2_toSend));
        memcpy(frameToSendFormat.data3.data(), &data3_toSend, sizeof(data3_toSend));
        memcpy(frameToSendFormat.data4.data(), &data4_toSend, sizeof(data4_toSend));

        QByteArray frameToSend = "";
        frameToSendFormat.verifyFlag = 0; // 重新计算校验位
        // 帧数据
        frameToSend.append(frameToSendFormat.frameHead1);
        frameToSend.append(frameToSendFormat.frameHead2);

        // frameToSend.append(frameToSendFormat.databytes);
        // frameToSendFormat.verifyFlag += frameToSendFormat.databytes;

        // frameToSend.append(frameToSendFormat.FrameCount);
        // frameToSendFormat.verifyFlag += frameToSendFormat.FrameCount;

        frameToSend.append(frameToSendFormat.ctrlMode);
        frameToSendFormat.verifyFlag += frameToSendFormat.ctrlMode;

        frameToSend.append(frameToSendFormat.data1);
        frameToSendFormat.verifyFlag += frameToSendFormat.data1[0];
        frameToSendFormat.verifyFlag += frameToSendFormat.data1[1];
        frameToSendFormat.verifyFlag += frameToSendFormat.data1[2];
        frameToSendFormat.verifyFlag += frameToSendFormat.data1[3];

        frameToSend.append(frameToSendFormat.data2);
        frameToSendFormat.verifyFlag += frameToSendFormat.data2[0];
        frameToSendFormat.verifyFlag += frameToSendFormat.data2[1];
        frameToSendFormat.verifyFlag += frameToSendFormat.data2[2];
        frameToSendFormat.verifyFlag += frameToSendFormat.data2[3];

        frameToSend.append(frameToSendFormat.data3);
        frameToSendFormat.verifyFlag += frameToSendFormat.data3[0];
        frameToSendFormat.verifyFlag += frameToSendFormat.data3[1];
        frameToSendFormat.verifyFlag += frameToSendFormat.data3[2];
        frameToSendFormat.verifyFlag += frameToSendFormat.data3[3];

        frameToSend.append(frameToSendFormat.data4);
        frameToSendFormat.verifyFlag += frameToSendFormat.data4[0];
        frameToSendFormat.verifyFlag += frameToSendFormat.data4[1];
        frameToSendFormat.verifyFlag += frameToSendFormat.data4[2];
        frameToSendFormat.verifyFlag += frameToSendFormat.data4[3];

        frameToSend.append(frameToSendFormat.verifyFlag); // 校验位
        frameToSend.append(frameToSendFormat.frameTail1);
        frameToSend.append(frameToSendFormat.frameTail2);

        qDebug() << frameToSend.toHex();

        sendFrameCount++;
        frameToSendFormat.FrameCount = (frameToSendFormat.FrameCount + 1) % 255; // 帧计数(可以不用取余，uchar类型+1默认255循环);
        emit sendFrameData(frameToSend);
    }
}

void MainWindow::plotUpdate()
{
    while (dataPtr > showPtr)
    {
        FloatDatas showFloatsData = datalist[showPtr];
        double sec = showFloatsData.framecountTag * (1.0 / dataRecvFreq); // 接收到数据的计数戳，根据下位机发送频率转换成等效时间

        //***************ChartPlot1******************
        switch (ui->comboBox_chart1_1->currentIndex())
        {
        case 0:
            ui->lineEdit_chart1_1->setText(QString::number(showFloatsData.float_data1_1, 'f', 3));
            break;
        case 1:
            ui->lineEdit_chart1_1->setText(QString::number(showFloatsData.float_data1_2, 'f', 3));
            break;
        case 2:
            ui->lineEdit_chart1_1->setText(QString::number(showFloatsData.float_data1_3, 'f', 3));
            break;
        default:
            break;
        }
        switch (ui->comboBox_chart1_2->currentIndex())
        {
        case 0:
            ui->lineEdit_chart1_2->setText(QString::number(showFloatsData.float_data1_3, 'f', 3));
            break;
        case 1:
            ui->lineEdit_chart1_2->setText(QString::number(showFloatsData.float_data1_4, 'f', 3));
            break;
        case 2:
            ui->lineEdit_chart1_2->setText(QString::number(showFloatsData.float_data1_5, 'f', 3));
            break;
        default:
            break;
        }
        ui->chartplot1->graph(0)->addData(sec, showFloatsData.float_data1_1); // 【曲线0】编码器1角度数据
        ui->chartplot1->graph(1)->addData(sec, showFloatsData.float_data1_2); // 【曲线1】编码器1角速度数据
        ui->chartplot1->graph(2)->addData(sec, showFloatsData.float_data1_3); // 【曲线2】
        ui->chartplot1->graph(3)->addData(sec, showFloatsData.float_data1_4); // 【曲线3】
        ui->chartplot1->graph(4)->addData(sec, showFloatsData.float_data1_5); // 【曲线4】
        ui->chartplot1->graph(5)->addData(sec, showFloatsData.float_data1_6); // 【曲线5】
        ui->chartplot1->xAxis->setRange(sec, 5, Qt::AlignRight);              // 设置曲线x显示范围,右对齐

        //***************ChartPlot2***************
        switch (ui->comboBox_chart2_1->currentIndex())
        {
        case 0:
            ui->lineEdit_chart2_1->setText(QString::number(showFloatsData.float_data2_1, 'f', 3));
            break;
        case 1:
            ui->lineEdit_chart2_1->setText(QString::number(showFloatsData.float_data2_2, 'f', 3));
            break;
        case 2:
            ui->lineEdit_chart2_1->setText(QString::number(showFloatsData.float_data2_3, 'f', 3));
            break;
        default:
            break;
        }
        switch (ui->comboBox_chart2_2->currentIndex())
        {
        case 0:
            ui->lineEdit_chart2_2->setText(QString::number(showFloatsData.float_data2_3, 'f', 3));
            break;
        case 1:
            ui->lineEdit_chart2_2->setText(QString::number(showFloatsData.float_data2_4, 'f', 3));
            break;
        case 2:
            ui->lineEdit_chart2_2->setText(QString::number(showFloatsData.float_data2_5, 'f', 3));
            break;
        default:
            break;
        }
        ui->chartplot2->graph(0)->addData(sec, showFloatsData.float_data2_1); // 【曲线0】编码器2角度数据
        ui->chartplot2->graph(1)->addData(sec, showFloatsData.float_data2_2); // 【曲线1】编码器2角速度数据
        ui->chartplot2->graph(2)->addData(sec, showFloatsData.float_data2_3); // 【曲线2】
        ui->chartplot2->graph(3)->addData(sec, showFloatsData.float_data2_4); // 【曲线3】
        ui->chartplot2->graph(4)->addData(sec, showFloatsData.float_data2_5); // 【曲线4】
        ui->chartplot2->graph(5)->addData(sec, showFloatsData.float_data2_6); // 【曲线5】
        ui->chartplot2->xAxis->setRange(sec, 5, Qt::AlignRight);

        showPtr++; // 显示指针移动
    }
    ui->chartplot1->replot(QCustomPlot::rpQueuedReplot); // 图表重绘
    ui->chartplot2->replot(QCustomPlot::rpQueuedReplot);
}

void MainWindow::on_pushButton_scanPort_clicked()
{
    scanPort(); // 检索串口
}

void MainWindow::on_pushButton_openPort_clicked()
{
    // 创建串口资源
    if (ui->pushButton_openPort->text() == tr("打开串口"))
    {
        // 设置串口名字，通过ui选择串口名字
        if (ui->comboBox_portNames->currentText() != NULL)
        {
            emit openSerial(ui->comboBox_portNames->currentText()); // 发送打开串口信号
            if ((readwriteCom->m_serialPort->portName() == ui->comboBox_portNames->currentText()) && (readwriteCom->m_serialPort->isOpen() == true))
            { // 成功打开串口后，进行界面变动
                qDebug() << readwriteCom->m_serialPort->portName() << "打开!";
                ui->label_portstate->setStyleSheet(port_open_SheetStyle); // 设置串口打开状态指示
                datalist.clear();
                dataPtr = datalist.size(); // 初始化数据存入指针
                showPtr = datalist.size(); // 初始化数据显示指针
                refreshTimer->start(0);    // 尽可能快地刷新
                ui->pushButton_openPort->setText("关闭串口");
                ui->pushButton_scanPort->setEnabled(false);
                ui->comboBox_portNames->setEnabled(false);
            }
        }
        else
        {
            QMessageBox::critical(this, tr("提示"), tr("未选择串口"));
        }
    }
    else if (ui->pushButton_openPort->text() == "关闭串口")
    {
        emit closeSerial(ui->comboBox_portNames->currentText()); // 发送关闭串口信号
        if (readwriteCom->m_serialPort->portName() == ui->comboBox_portNames->currentText() && readwriteCom->m_serialPort->isOpen() == false)
        { // 成功关闭串口后，进行界面变动
            qDebug() << readwriteCom->m_serialPort->portName() << "关闭!";
            ui->label_portstate->setStyleSheet(port_close_SheetStyle); // 设置串口关闭状态指示
            refreshTimer->stop();
            sendTimer->stop(); // 上位机命令发送
            ui->pushButton_openPort->setText("打开串口");
            ui->pushButton_scanPort->setEnabled(true);
            ui->comboBox_portNames->setEnabled(true);
        }
    }
}

void MainWindow::on_pushButton_saveData_clicked()
{
    if (ui->pushButton_openPort->text() == "关闭串口")
    {
        on_pushButton_openPort_clicked(); // 若串口处于打开状态，关闭串口
    }

    // NOTE 数据保存写入文件
    QString path = QDir::currentPath(); // 获取当前目录
    QString fileName = path + tr("/%1.txt").arg("数据");
    QFile file(fileName);
    QTextStream txtOutput(&file);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) // 写入,添加结束符\n
    {
        QMessageBox::warning(this, tr("错误"), tr("打开文件失败,数据保存失败"));
        return;
    }
    else
    {
        for (int i = 0; i < datalist.size(); i++)
        {
            // 数据保存精度 小数点4位
            QString framecountTag_Int = QString::number(datalist[i].framecountTag, 10);
            QString float_data1_Float = QString::number(datalist[i].float_data1_1, 'f', 4); // 目镜角度 后端编码器角度
            QString float_data2_Float = QString::number(datalist[i].float_data2_1, 'f', 4); // 物镜角度 前端编码器角度
            QString float_data3_Float = QString::number(datalist[i].float_data1_4, 'f', 4); // 目镜速度
            QString float_data4_Float = QString::number(datalist[i].float_data2_4, 'f', 4); // 物镜速度

            txtOutput
                << framecountTag_Int << ','
                << float_data1_Float << ','
                << float_data2_Float << ','
                << float_data3_Float << ','
                << float_data4_Float << endl;
        }
    }
    file.close();
}

// 图表1-1切换曲线
void MainWindow::on_comboBox_chart1_1_currentIndexChanged(int index)
{
    for (int plotindex = 0; plotindex < ui->comboBox_chart1_1->count(); plotindex++)
    {
        ui->chartplot1->graph(plotindex)->setVisible(false); // 取消数据组1的所有曲线显示
    }
    ui->chartplot1->graph(index)->setVisible(true);      // 显示指定曲线
    ui->chartplot1->replot(QCustomPlot::rpQueuedReplot); // 图表重绘
    curveLegendSet(ui->curvelegend1_1, index);           // 曲线图例设置
}

// 图表1-2切换曲线
void MainWindow::on_comboBox_chart1_2_currentIndexChanged(int index)
{
    for (int plotindex = 0; plotindex < ui->comboBox_chart1_2->count(); plotindex++)
    {
        ui->chartplot1->graph(plotindex + 3)->setVisible(false);
    }
    ui->chartplot1->graph(index + 3)->setVisible(true);
    ui->chartplot1->replot(QCustomPlot::rpQueuedReplot); // 图表重绘
    curveLegendSet(ui->curvelegend1_2, index + 3);       // 曲线图例设置
}

// 图表2-1切换曲线
void MainWindow::on_comboBox_chart2_1_currentIndexChanged(int index)
{
    for (int plotindex = 0; plotindex < ui->comboBox_chart2_1->count(); plotindex++)
    {
        ui->chartplot2->graph(plotindex)->setVisible(false);
    }
    ui->chartplot2->graph(index)->setVisible(true);
    ui->chartplot2->replot(QCustomPlot::rpQueuedReplot); // 图表重绘
    curveLegendSet(ui->curvelegend2_1, index);           // 曲线图例设置
}

// 图表2-2切换曲线
void MainWindow::on_comboBox_chart2_2_currentIndexChanged(int index)
{
    for (int plotindex = 0; plotindex < ui->comboBox_chart2_2->count(); plotindex++)
    {
        ui->chartplot2->graph(plotindex + 3)->setVisible(false);
    }
    ui->chartplot2->graph(index + 3)->setVisible(true);
    ui->chartplot2->replot(QCustomPlot::rpQueuedReplot); // 图表重绘
    curveLegendSet(ui->curvelegend2_2, index + 3);       // 曲线图例设置
}

// 显示基本环路控制窗口
void MainWindow::on_pushButton_fundamental_clicked()
{
    Fwindow->raise(); // 显示在最上层
    Fwindow->show();
}

// 显示高级环路控制窗口
void MainWindow::on_pushButton_advanced_clicked()
{
    Awindow->raise(); // 显示在最上层
    Awindow->show();
}

void MainWindow::on_pushButton_send_clicked()
{
    if (readwriteCom->m_serialPort->isOpen()) // 打开串口后才能修改界面数据及允许发送
    {

        // 数据指令1
        if (ui->lineEdit_motor1pos->text() == "")
        {
            ui->lineEdit_motor1pos->setText("0");
            data1_toSend = 0;
        }
        else
        {
            data1_toSend = ui->lineEdit_motor1pos->text().toFloat();
        }
        // 数据指令2
        if (ui->lineEdit_motor1vel->text() == "")
        {
            ui->lineEdit_motor1vel->setText("0");
            data2_toSend = 0;
        }
        else
        {
            data1_toSend = ui->lineEdit_motor1vel->text().toFloat();
        }
        // 数据指令3
        if (ui->lineEdit_motor2pos->text() == "")
        {
            ui->lineEdit_motor2pos->setText("0");
            data3_toSend = 0;
        }
        else
        {
            data3_toSend = ui->lineEdit_motor2pos->text().toFloat();
        }
        // 数据指令4
        if (ui->lineEdit_motor2vel->text() == "")
        {
            ui->lineEdit_motor2vel->setText("0");
            data4_toSend = 0;
        }
        else
        {
            data4_toSend = ui->lineEdit_motor2vel->text().toFloat();
        }
        // 控制模式指令
        if (ui->comboBox_mode->currentIndex() == -1)
        {
            ui->comboBox_mode->setCurrentIndex(0); // 未选择模式时，界面更改显示为Combox首个
            controlMode_toSend = 1;                // 默认为禁能模式0x01
        }
        else
        {
            controlMode_toSend = ui->comboBox_mode->currentIndex() + 1;
            sendTimer->start(1 / dataSendFreq);
        }
    }
}

void MainWindow::on_pushButton_stopSend_clicked()
{
    sendTimer->stop();
}
