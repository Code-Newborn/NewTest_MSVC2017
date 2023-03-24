#include "advanced_window.h"
#include "ui_advanced_window.h"

Advanced_Window::Advanced_Window(QWidget *parent) : QMainWindow(parent),
                                                    ui(new Ui::Advanced_Window)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
}

Advanced_Window::~Advanced_Window()
{
    delete ui;
}
