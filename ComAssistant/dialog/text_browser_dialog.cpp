#include "text_browser_dialog.h"
#include "ui_text_browser_dialog.h"

Text_Browser_Dialog::Text_Browser_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Text_Browser_Dialog)
{
    ui->setupUi(this);
}

void Text_Browser_Dialog::loadHtml(QString chinesePath, QString englishPath)
{
    QFile file(chinesePath);
    if (QLocale::system().name() != "zh_CN" && !englishPath.isEmpty())
    {
        file.setFileName(englishPath);
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
            QMessageBox::information(this, tr("提示"), tr("文件被占用。"));
        }
    }else{
        QMessageBox::information(this, tr("提示"), tr("文件丢失。"));
    }
}

void Text_Browser_Dialog::showAsciiTable()
{
    loadHtml(":AsciiTable.html", ":/AsciiTable_EN.html");
    this->setWindowTitle("ASCII Table");
}
void Text_Browser_Dialog::showPriorityTable()
{
    loadHtml(":PriorityTable.html", ":/PriorityTable.html");
    this->setWindowTitle("Operation Priority Table");
}

Text_Browser_Dialog::~Text_Browser_Dialog()
{
    delete ui;
}
