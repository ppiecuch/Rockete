/**********************************************************************************
 * License notice
 * ------------------------
 * rmlsyntaxhighlighter.cpp
 * ------------------------
 * Copyright (c) 2012 Alexander Kasper (alexander.limubei.kasper@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ***********************************************************************************/

#include "rmlsyntaxhighlighter.hpp"

CRMLSyntaxHighlighter::CRMLSyntaxHighlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent)
{
    HighlightingRule Rule;

    mTagFormat.setForeground(Qt::darkRed);
    mTagFormat.setFontWeight(QFont::Bold);

    mAttributeFormat.setForeground(Qt::darkBlue);

    mStringFormat.setForeground(Qt::darkGreen);
    mStringFormat.setFontItalic(true);

    Rule.mPattern = QRegExp("<([A-Z][A-Z0-9]*)\\b");
    Rule.mPattern.setCaseSensitivity(Qt::CaseInsensitive);
    Rule.mFormat = mTagFormat;
    mHighlightingRules.append(Rule);

    Rule.mPattern = QRegExp("</([A-Z][A-Z0-9]*)\\b");
    Rule.mPattern.setCaseSensitivity(Qt::CaseInsensitive);
    Rule.mFormat = mTagFormat;
    mHighlightingRules.append(Rule);

    Rule.mPattern = QRegExp("/*>");
    Rule.mPattern.setCaseSensitivity(Qt::CaseInsensitive);
    Rule.mFormat = mTagFormat;
    mHighlightingRules.append(Rule);

    Rule.mPattern = QRegExp("([A-Z][A-Z0-9]*)\\b=");
    Rule.mPattern.setCaseSensitivity(Qt::CaseInsensitive);
    Rule.mFormat = mAttributeFormat;
    mHighlightingRules.append(Rule);

    Rule.mPattern = QRegExp("\"[^\"\\r\\n]*\"");
    Rule.mPattern.setCaseSensitivity(Qt::CaseInsensitive);
    Rule.mFormat = mStringFormat;
    mHighlightingRules.append(Rule);

    mCommentStartExpression =  QRegExp("<!--");
    mCommentEndExpression = QRegExp("-->");

    mMultiLineCommentFormat.setForeground(Qt::lightGray);
    mMultiLineCommentFormat.setFontItalic(true);
}

void CRMLSyntaxHighlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, mHighlightingRules)
    {
        QRegExp expression(rule.mPattern);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.mFormat);
            index = expression.indexIn(text, index + length);
        }
    }

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = mCommentStartExpression.indexIn(text);

    while (startIndex >= 0)
    {
        int endIndex = mCommentEndExpression.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1)
        {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex
                    + mCommentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, mMultiLineCommentFormat);
        startIndex = mCommentStartExpression.indexIn(text, startIndex + commentLength);
    }
}
