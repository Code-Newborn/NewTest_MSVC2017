#include "fundamental_window.h"
#include "ui_fundamental_window.h"

Fundamental_Window::Fundamental_Window(QWidget *parent) : QMainWindow(parent),
                                                          ui(new Ui::Fundamental_Window)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
}

Fundamental_Window::~Fundamental_Window()
{
    delete ui;
}
