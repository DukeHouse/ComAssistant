#ifndef ASCII_TABLE_DIALOG_H
#define ASCII_TABLE_DIALOG_H

#include <QDialog>
#include <QFile>
#include <QMessageBox>

namespace Ui {
class Ascii_Table_Dialog;
}

class Ascii_Table_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Ascii_Table_Dialog(QWidget *parent = nullptr);
    ~Ascii_Table_Dialog();

private:
    Ui::Ascii_Table_Dialog *ui;
};

#endif // ASCII_TABLE_DIALOG_H
