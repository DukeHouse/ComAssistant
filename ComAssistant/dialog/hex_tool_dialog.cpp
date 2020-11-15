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
    //空数据不做转换以免覆盖对侧内容
    if(ui->asciiBrowser->toPlainText().isEmpty())
    {
        return;
    }
    ui->hexBrowser->setPlainText(toHexDisplay(ui->asciiBrowser->toPlainText().toLocal8Bit()));
}

int32_t Hex_Tool_Dialog::hex_data_pre_formatter(QString input, QString &output)
{
    QRegularExpression reg;
    QRegularExpressionMatch match;
    int scanIndex = 0;

    //出现3个以上的连续数字则按每2个进行切分
    scanIndex = 0;
    reg.setPattern("\\d\\d[\\d]+");
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    do {
            match = reg.match(input, scanIndex);
            if(match.hasMatch()) {
                scanIndex = match.capturedStart() + 2;
                input.insert(scanIndex, ' ');
                continue;
            }
            else
            {
                break;
            }
    } while(scanIndex < input.size());

    //替换英文分隔符为空格
    scanIndex = 0;
    reg.setPattern("[,.;\\\\/+-]");
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    do {
            match = reg.match(input, scanIndex);
            if(match.hasMatch()) {
                scanIndex = match.capturedStart();
                input.replace(scanIndex, 1, ' ');
                continue;
            }
            else
            {
                break;
            }
    } while(scanIndex < input.size());

    //替换中文分隔符为空格
    scanIndex = 0;
    reg.setPattern("[，。；、、]");
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    do {
            match = reg.match(input, scanIndex);
            if(match.hasMatch()) {
                scanIndex = match.capturedStart();
                input.replace(scanIndex, 1, ' ');
                continue;
            }
            else
            {
                break;
            }
    } while(scanIndex < input.size());

    output = input;
    return 0;
}

void Hex_Tool_Dialog::on_pushButton_toAscii_clicked()
{
    if(ui->hexBrowser->toPlainText().isEmpty())
    {
        return;
    }
    bool ok;
    QString temp;
    QString temp2;
    temp = ui->hexBrowser->toPlainText();
    hex_data_pre_formatter(temp, temp);
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
