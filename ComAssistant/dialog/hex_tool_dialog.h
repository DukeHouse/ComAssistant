#ifndef HEX_TOOL_DIALOG_H
#define HEX_TOOL_DIALOG_H

#include <QDialog>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
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
    void on_pushButton_AsciiToHex_clicked();

    void on_pushButton_HexToAscii_clicked();

    void on_pushButton_clear_clicked();

    void on_pushButton_FloatToHex_clicked();

    void on_pushButton_HexToFloat_clicked();

    void on_pushButton_FloatToHex_BigEndian_clicked();

    void on_pushButton_HexToFloat_BigEndian_clicked();

private:
    Ui::Hex_Tool_Dialog *ui;
    int32_t replace_spliter_to_space(QString input, QString &output, QString RegExp);
    int32_t hex_data_pre_formatter(QString input, QString &output);
    int32_t float_data_pre_formatter(QString input, QString &output);
    bool byteArrayToFloat(const QByteArray& array, float& result);
};

#endif // HEX_TOOL_DIALOG_H
