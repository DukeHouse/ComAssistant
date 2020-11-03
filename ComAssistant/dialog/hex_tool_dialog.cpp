#include "hex_tool_dialog.h"
#include "ui_hex_tool_dialog.h"

Hex_Tool_Dialog::Hex_Tool_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Hex_Tool_Dialog)
{
    ui->setupUi(this);
}

Hex_Tool_Dialog::~Hex_Tool_Dialog()
{
    delete ui;
}

void Hex_Tool_Dialog::on_pushButton_toHex_clicked()
{
    ui->hexBrowser->setPlainText(toHexDisplay(ui->asciiBrowser->toPlainText().toLocal8Bit()));
}

void Hex_Tool_Dialog::on_pushButton_toAscii_clicked()
{
    bool ok;
    QString temp;
    QString temp2;
    temp = ui->hexBrowser->toPlainText();
    temp2 = toAsciiDisplay(temp, ok);
    if(!ok)
    {
        QMessageBox::information(this, tr("提示"), tr("HEX转ASCII失败"));
        return;
    }
    ui->asciiBrowser->setPlainText(temp2);
}

void Hex_Tool_Dialog::on_pushButton_clear_clicked()
{
    ui->asciiBrowser->clear();
    ui->hexBrowser->clear();
}
