#ifndef HEX_TOOL_DIALOG_H
#define HEX_TOOL_DIALOG_H

#include <QDialog>
#include <QFile>
#include <QMessageBox>
#include "baseconversion.h"

namespace Ui {
class Hex_Tool_Dialog;
}

class Hex_Tool_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Hex_Tool_Dialog(QWidget *parent = nullptr);
    ~Hex_Tool_Dialog();

private slots:
    void on_pushButton_toHex_clicked();

    void on_pushButton_toAscii_clicked();

    void on_pushButton_clear_clicked();

private:
    Ui::Hex_Tool_Dialog *ui;
};

#endif // HEX_TOOL_DIALOG_H
