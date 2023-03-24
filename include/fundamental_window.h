#ifndef FUNDAMENTAL_WINDOW_H
#define FUNDAMENTAL_WINDOW_H

#include <QMainWindow>

namespace Ui
{
    class Fundamental_Window;
}

class Fundamental_Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Fundamental_Window(QWidget *parent = nullptr);
    ~Fundamental_Window();

private:
    Ui::Fundamental_Window *ui;
};

#endif // FUNDAMENTAL_WINDOW_H
