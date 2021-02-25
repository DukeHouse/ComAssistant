#ifndef TEXT_BROWSER_DIALOG_H
#define TEXT_BROWSER_DIALOG_H

#include <QDialog>
#include <QFile>
#include <QMessageBox>

namespace Ui {
class Text_Browser_Dialog;
}

class Text_Browser_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Text_Browser_Dialog(QWidget *parent = nullptr);
    ~Text_Browser_Dialog();
    void showAsciiTable();
    void showPriorityTable();

private:
    void loadHtml(QString chinesePath, QString englishPath = "");
    Ui::Text_Browser_Dialog *ui;
};

#endif // TEXT_BROWSER_DIALOG_H
