// This file is part of Agros2D.
//
// Agros2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Agros2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Agros2D.  If not, see <http://www.gnu.org/licenses/>.
//
// hp-FEM group (http://hpfem.org/)
// University of Nevada, Reno (UNR) and University of West Bohemia, Pilsen
// Email: agros2d@googlegroups.com, home page: http://hpfem.org/agros2d/

#ifndef QSCRIPTSYNTAXHIGHLIGHTER_H
#define QSCRIPTSYNTAXHIGHLIGHTER_H

#include <QtCore/QVector>
#include <QtCore/QSet>
#include <QtGui/QSyntaxHighlighter>

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>

#include "util.h"

class QTextDocument;

struct ParenthesisInfo
{
    char character;
    int position;
};

class TextBlockData : public QTextBlockUserData
{
public:
    TextBlockData();

    QVector<ParenthesisInfo *> parentheses();
    void insert(ParenthesisInfo *info);

private:
    QVector<ParenthesisInfo *> m_parentheses;
};

class QScriptSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    QScriptSyntaxHighlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);
    void highlightBlockParenthesis(const QString &text, char left, char right);

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegExp commentStartExpression;
    QRegExp commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat operatorFormat;   
    QTextCharFormat numberFormat;
};

#endif // QSCRIPTSYNTAXHIGHLIGHTER_H
