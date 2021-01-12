#include "about_me_dialog.h"
#include "ui_about_me_dialog.h"

About_Me_Dialog::About_Me_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About_Me_Dialog)
{
    ui->setupUi(this);

#ifdef Q_OS_WIN32
    QFont font("Microsoft YaHei", 10);
    ui->textBrowser->document()->setDefaultFont(font);
#endif

    //不要使用高亮器，因为html显示会出问题
}

About_Me_Dialog::~About_Me_Dialog()
{
    delete ui;
}

void About_Me_Dialog::getVersionString(QString str)
{
    if(str.isEmpty())
        return;
    ui->version->setText(str);
}

void About_Me_Dialog::showManualDoc(void)
{
    QFile file(":/manual.html");
    QString html;
    if (QLocale::system().name() != "zh_CN")
    {
        file.setFileName(":/manual_EN.html");
    }
    if(file.exists()){
        if(file.open(QFile::ReadOnly)){
            html = file.readAll();
            file.close();

            ui->textBrowser->clear();
            ui->textBrowser->setHtml(html);

            //滚动到最前面，不知道为什么设置滚动条没效果
            QTextCursor cursor = ui->textBrowser->textCursor();
            cursor.setPosition(0);
            ui->textBrowser->setTextCursor(cursor);
        }else{
            QMessageBox::information(this, "提示", "帮助文件被占用。");
        }
    }else{
        QMessageBox::information(this, "提示", "帮助文件丢失。");
    }
}

void About_Me_Dialog::showMarkdown(QString &markdown)
{
    ui->textBrowser->clear();
    ui->textBrowser->setMarkdown(markdown);
}

void About_Me_Dialog::on_okButton_clicked()
{
    this->close();
}

void About_Me_Dialog::on_email_linkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

void About_Me_Dialog::on_QtUrl_linkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

void About_Me_Dialog::on_QtUrl_Compiler_linkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

void About_Me_Dialog::on_QtUrl_SupportUs_linkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}
