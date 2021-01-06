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

void Hex_Tool_Dialog::on_pushButton_AsciiToHex_clicked()
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
    //每两个数字字符进行分割
    replace_spliter_to_space(input, input, "\\d\\d[\\d]+");
    //中英文分隔符换空格
    replace_spliter_to_space(input, input, "[,.;\\\\/]");
    replace_spliter_to_space(input, input, "[，。；、、]");
    //删除多余空格
    output = input.simplified();
    return 0;
}

void Hex_Tool_Dialog::on_pushButton_HexToAscii_clicked()
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

//替换分隔符为空格
int32_t Hex_Tool_Dialog::replace_spliter_to_space(QString input, QString &output, QString RegExp)
{
    QRegularExpression reg;
    QRegularExpressionMatch match;
    int scanIndex = 0;

    scanIndex = 0;
    reg.setPattern(RegExp);
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

int32_t Hex_Tool_Dialog::float_data_pre_formatter(QString input, QString &output)
{
    replace_spliter_to_space(input, input, "[,;\\\\/]");
    replace_spliter_to_space(input, input, "[，。；、、]");
    replace_spliter_to_space(input, input, "\n");
    output = input.simplified();
    return 0;
}

void Hex_Tool_Dialog::on_pushButton_FloatToHex_clicked()
{
    if(ui->asciiBrowser->toPlainText().isEmpty())
        return;

    QString str;

    float_data_pre_formatter(ui->asciiBrowser->toPlainText().toLocal8Bit(), str);

    QStringList list = str.split(' ');

    float num;
    QString numStr;
    QString showStr;
    bool ok;
    for(int32_t i = 0; i < list.size(); i++)
    {
        num = list.at(i).toFloat(&ok);
        if(!ok)
        {
            QMessageBox::information(this, tr("提示"), tr("转换失败，数据错误：") + list.at(i));
            break;
        }

        QByteArray array = QByteArray::fromRawData(reinterpret_cast<char *>(&num), sizeof(float));
        numStr = toHexDisplay(array);
        numStr.remove(' ');
        showStr.append(numStr);
        showStr.append(' ');
    }
    ui->hexBrowser->setPlainText(showStr);
}

bool Hex_Tool_Dialog::byteArrayToFloat(const QByteArray& array, float& result)
{
    if(array.size()<4)
        return false;

    char num[4];
    for(int i = 0; i<4; i++)
        num[i] = array.at(i);//
//    qDebug("%.2f", *(reinterpret_cast<float*>(num)));

    result = *(reinterpret_cast<float*>(num));
    return true;
}

void Hex_Tool_Dialog::on_pushButton_HexToFloat_clicked()
{
    if(ui->hexBrowser->toPlainText().isEmpty())
    {
        return;
    }

    QString str = ui->hexBrowser->toPlainText().toLocal8Bit();
    replace_spliter_to_space(str, str, "[,;\\\\/]");
    replace_spliter_to_space(str, str, "[，。；、、]");
    replace_spliter_to_space(str, str, "\n");
    str = str.simplified();

    QStringList list = str.split(' ');
    QString subStr;
    QString showStr;
    float num;
    bool ok;
    for(int32_t i = 0; i < list.size(); i++)
    {
        subStr = list.at(i);
        if(subStr.size() != 8)
        {
            QMessageBox::information(this, tr("提示"),
                                     tr("转换失败，数据错误：") + list.at(i));
            break;
        }
        subStr.insert(6, ' ');
        subStr.insert(4, ' ');
        subStr.insert(2, ' ');
        QByteArray array;
        array = HexStringToByteArray(subStr, ok);
        if(!byteArrayToFloat(array, num))
        {
            QMessageBox::information(this, tr("提示"),
                                     tr("转换失败，数据错误：") + list.at(i));
            break;
        }
        showStr.append(QString::number(num));
        showStr.append(" ");
    }
    ui->asciiBrowser->setPlainText(showStr);
}

void Hex_Tool_Dialog::on_pushButton_FloatToHex_BigEndian_clicked()
{
    if(ui->asciiBrowser->toPlainText().isEmpty())
        return;

    QString str;

    float_data_pre_formatter(ui->asciiBrowser->toPlainText().toLocal8Bit(), str);

    QStringList list = str.split(' ');

    float num;
    QString numStr;
    QString showStr;
    bool ok;
    for(int32_t i = 0; i < list.size(); i++)
    {
        num = list.at(i).toFloat(&ok);
        if(!ok)
        {
            QMessageBox::information(this, tr("提示"), tr("转换失败，数据错误：") + list.at(i));
            break;
        }

        QByteArray array = QByteArray::fromRawData(reinterpret_cast<char *>(&num), sizeof(float));
        QByteArray invertedArray;
        if(array.size() != 4)
        {
            QMessageBox::information(this, tr("提示"), tr("转换失败，数据错误：") + list.at(i));
            break;
        }
        invertedArray.append(array.at(3));
        invertedArray.append(array.at(2));
        invertedArray.append(array.at(1));
        invertedArray.append(array.at(0));
        numStr = toHexDisplay(invertedArray);
        numStr.remove(' ');
        showStr.append(numStr);
        showStr.append(' ');
    }
    ui->hexBrowser->setPlainText(showStr);
}

void Hex_Tool_Dialog::on_pushButton_HexToFloat_BigEndian_clicked()
{
    if(ui->hexBrowser->toPlainText().isEmpty())
    {
        return;
    }

    QString str = ui->hexBrowser->toPlainText().toLocal8Bit();
    replace_spliter_to_space(str, str, "[,;\\\\/]");
    replace_spliter_to_space(str, str, "[，。；、、]");
    replace_spliter_to_space(str, str, "\n");
    str = str.simplified();

    QStringList list = str.split(' ');
    QString subStr;
    QString showStr;
    float num;
    bool ok;
    for(int32_t i = 0; i < list.size(); i++)
    {
        subStr = list.at(i);
        if(subStr.size() != 8)
        {
            QMessageBox::information(this, tr("提示"),
                                     tr("转换失败，数据错误：") + list.at(i));
            break;
        }
        QString invertedSubStr;
        invertedSubStr.append(subStr.mid(6, 2));
        invertedSubStr.append(subStr.mid(4, 2));
        invertedSubStr.append(subStr.mid(2, 2));
        invertedSubStr.append(subStr.mid(0, 2));
        subStr = invertedSubStr;
        subStr.insert(6, ' ');
        subStr.insert(4, ' ');
        subStr.insert(2, ' ');
        QByteArray array;
        array = HexStringToByteArray(subStr, ok);
        if(!byteArrayToFloat(array, num))
        {
            QMessageBox::information(this, tr("提示"),
                                     tr("转换失败，数据错误：") + list.at(i));
            break;
        }
        showStr.append(QString::number(num));
        showStr.append(" ");
    }
    ui->asciiBrowser->setPlainText(showStr);
}
