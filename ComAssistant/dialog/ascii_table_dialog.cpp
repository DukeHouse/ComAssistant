#include "ascii_table_dialog.h"
#include "ui_ascii_table_dialog.h"

Ascii_Table_Dialog::Ascii_Table_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Ascii_Table_Dialog)
{
    ui->setupUi(this);

    QFile file(":AsciiTable.html");
    if (QLocale::system().name() != "zh_CN")
    {
        file.setFileName(":/AsciiTable_EN.html");
    }
    QString html;
    if(file.exists()){
        if(file.open(QFile::ReadOnly)){
            html = file.readAll();
            file.close();

            ui->textBrowser->clear();
            ui->textBrowser->setFont(QFont("Arial"));
            ui->textBrowser->setText(html);
        }else{
            QMessageBox::information(this, "提示", "帮助文件被占用。");
        }
    }else{
        QMessageBox::information(this, "提示", "帮助文件丢失。");
    }
}

Ascii_Table_Dialog::~Ascii_Table_Dialog()
{
    delete ui;
}
