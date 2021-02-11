/**
 * @brief   高亮器
 * @file    highlighter.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    构造函数中设定了各种高亮规则
 */
#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

//! [0]
class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat separateFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat singleLineCommentFormat;
};
//! [0]

#endif // HIGHLIGHTER_H
