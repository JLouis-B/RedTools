#ifndef GUI_ABOUT_H
#define GUI_ABOUT_H

#include <QDialog>

namespace Ui {
class GUI_About;
}

class GUI_About : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_About(QWidget *parent = nullptr);
    ~GUI_About();

private:
    Ui::GUI_About *_ui;
};

#endif // GUI_ABOUT_H
