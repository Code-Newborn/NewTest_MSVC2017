#ifndef ADVANCED_WINDOW_H
#define ADVANCED_WINDOW_H

#include <QMainWindow>

namespace Ui
{
    class Advanced_Window;
}

class Advanced_Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Advanced_Window(QWidget *parent = nullptr);
    ~Advanced_Window();

private:
    Ui::Advanced_Window *ui;
};

#endif // ADVANCED_WINDOW_H
