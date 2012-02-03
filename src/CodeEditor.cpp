#include "CodeEditor.h"

#include <QScrollBar>
#include <QStringListModel>
#include "Settings.h"
#include "Rockete.h"

// Public:

CodeEditor::CodeEditor() : QTextEdit()
{
    QFile 
        tags("tag_list.txt"),
        customs("custom_list.txt"),
        keywords("keyword_list.txt");

    tags.open(QFile::ReadOnly);
    while (!tags.atEnd())
    {
        QByteArray line = tags.readLine();
        if (!line.isEmpty())
        {
            tag_list << line.trimmed();
        }
    }

    customs.open(QFile::ReadOnly);
    while (!customs.atEnd())
    {
        QByteArray line = customs.readLine();
        if (!line.isEmpty())
        {
            custom_list << line.trimmed();
        }
    }

    keywords.open(QFile::ReadOnly);
    while (!keywords.atEnd())
    {
        QByteArray line = keywords.readLine();
        if (!line.isEmpty())
        {
            keyword_list << line.trimmed();
        }
    }

    QStringList full_list;

    full_list = keyword_list + custom_list + tag_list;
    full_list.sort();

    AutoCompleter = new QCompleter(full_list, this);
    AutoCompleter->setWidget(this);
    AutoCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    AutoCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

    QObject::connect(AutoCompleter, SIGNAL(activated(const QString &)), this, SLOT(completeText(const QString &)));
}

bool CodeEditor::CheckCssCorrectness(QString & error_message)
{
    QTextCursor parsingTextCursor = textCursor();
    int opened_brace_counter = 0;

    parsingTextCursor.setPosition(0);

    while(!parsingTextCursor.atEnd())
    {
        if(toPlainText()[parsingTextCursor.position()] == '{')
        {
            opened_brace_counter++;
        }
        else if(toPlainText()[parsingTextCursor.position()] == '}')
        {
            opened_brace_counter--;
        }

        parsingTextCursor.movePosition(QTextCursor::Right);
    }

    if(opened_brace_counter!=0)
    {
        error_message = ( opened_brace_counter < 0 ? "too many '}'" : "too many '{'" );
        error_message += " search for '{|}' to highlight all '{' and '}'";
    }

    return opened_brace_counter == 0;
}

bool CodeEditor::CheckXmlCorrectness(QString & error_message)
{
    QTextCursor parsingTextCursor = textCursor();
    int tag_delimiter_balance = 0;
    QStringList opened_tag_list;

    parsingTextCursor.setPosition(0);

    while(!parsingTextCursor.atEnd())
    {
        if(toPlainText()[parsingTextCursor.position()] == '<')
        {
            tag_delimiter_balance++;
        }
        else if(toPlainText()[parsingTextCursor.position()] == '>')
        {
            tag_delimiter_balance--;
        }

        if ( tag_delimiter_balance == 0 && parsingTextCursor.hasSelection() )
        {
            QString 
                tag_text = parsingTextCursor.selectedText().trimmed();

            tag_text.remove('<');
            
            if( !tag_text.contains('/') )
            {
                if ( tag_text.contains(' ') )
                {
                    int first_space = tag_text.indexOf(' ');
                    tag_text.chop(tag_text.count() - first_space);
                }

                opened_tag_list.append(tag_text);
            }
            else
            {
                if(tag_text.startsWith('/'))
                {
                    tag_text.remove('/');
                    if(!opened_tag_list.removeOne(tag_text))
                    {
                        error_message = tag_text + " is closed without being opened";
                        setTextCursor(parsingTextCursor);
                        return false;
                    }
                }
            }
        }

        parsingTextCursor.movePosition(QTextCursor::Right, tag_delimiter_balance > 0 ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
    }

    opened_tag_list.removeOne( "!--" );

    if(tag_delimiter_balance!=0)
    {
        error_message = ( tag_delimiter_balance < 0 ? "too many '>'" : "too many '<'" );
        error_message += " search for '<|>' to highlight all '<' and '>'";
    }
    else if(!opened_tag_list.isEmpty())
    {
        error_message = opened_tag_list.first() + " is not closed";
    }
    

    return tag_delimiter_balance == 0 && opened_tag_list.isEmpty();
}

// public slots:

void CodeEditor::completeText(const QString &text)
{
    textCursor().beginEditBlock();
    QString adding_text;
    QTextCursor editingTextCursor = textCursor();

    editingTextCursor.setPosition(textCursor().selectionStart());
    editingTextCursor.movePosition( QTextCursor::EndOfWord, QTextCursor::MoveAnchor );
    editingTextCursor.movePosition( QTextCursor::StartOfWord, QTextCursor::KeepAnchor );
    editingTextCursor.removeSelectedText();
    adding_text = text;
    if(tag_list.contains(text)) // its a tag, check for < & >
    {
        if(toPlainText()[editingTextCursor.position() > 0 ? editingTextCursor.position()-1 : 0] != '<')
        {
            adding_text = "<" + adding_text;
            adding_text += ">";
        }
    }

    editingTextCursor.insertText(adding_text);
    setTextCursor(editingTextCursor);

    textCursor().endEditBlock();
}

// Protected:

void CodeEditor::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_Tab || e->key() == Qt::Key_Backtab) {
        if (AutoCompleter->popup()->isVisible())
        {
            e->ignore();
            return;
        }

        textCursor().beginEditBlock();
        bool shiftIsPressed = e->key() == Qt::Key_Backtab;

        QTextCursor editingTextCursor = textCursor();
        QTextCursor endingTextCursor = textCursor();

        editingTextCursor.setPosition(textCursor().selectionStart());
        endingTextCursor.setPosition(textCursor().selectionEnd());
        endingTextCursor.movePosition( QTextCursor::Down );
        endingTextCursor.movePosition( QTextCursor::StartOfLine );

        do{
            editingTextCursor.movePosition( QTextCursor::StartOfLine );

            if (!shiftIsPressed) {
                for(int j=0; j<Settings::getTabSize(); ++j) {
                    editingTextCursor.insertText(" ");
                }
            }
            else {
                for(int j=0; j<Settings::getTabSize(); ++j) {
                    if (toPlainText()[editingTextCursor.position()] == ' ') {
                        editingTextCursor.deleteChar();
                    }
                }
            }
            editingTextCursor.movePosition( QTextCursor::Down );
            editingTextCursor.movePosition( QTextCursor::StartOfLine );

        } while(editingTextCursor.position() != endingTextCursor.position());
        textCursor().endEditBlock();

    }
    else if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        QStringList lineList = toPlainText().split("\n");
        int currentPosition = 0;
        int lineStartIndex = 0;
        int nextLineStartIndex;
        int spaceCount;

        if (AutoCompleter->popup()->isVisible())
        {
            e->ignore();
            return;
        }

        textCursor().beginEditBlock();
        // TODO: refactor using same kind of functions as the tabbing system
        currentPosition = textCursor().selectionStart();

        for(int i=0; i<lineList.size(); ++i){
            nextLineStartIndex = lineStartIndex + lineList[i].size() + 1;

            if ((lineStartIndex >= currentPosition || (currentPosition >= lineStartIndex && currentPosition < nextLineStartIndex))) {
                for( spaceCount = 0;lineList[i][spaceCount].isSpace(); spaceCount++ );
                textCursor().insertText("\n");
                for( ;spaceCount>0; spaceCount-- ){
                    textCursor().insertText(" ");
                }
                break;
            }
            lineStartIndex = nextLineStartIndex;
        }
        textCursor().endEditBlock();
    }
    else if (e->key() == Qt::Key_Left)
    {
        if(textCursor().hasSelection() && (e->modifiers() & Qt::ShiftModifier) == 0)
        {
            textCursor().beginEditBlock();
            QTextCursor newCursor = textCursor();
            newCursor.setPosition(textCursor().selectionStart());
            setTextCursor(newCursor);
            document()->setModified(false);
            textCursor().endEditBlock();
        }
        else
        {
            QTextEdit::keyPressEvent(e);
        }
    }
    else if (e->key() == Qt::Key_Right)
    {
        if(textCursor().hasSelection() && (e->modifiers() & Qt::ShiftModifier) == 0)
        {
            textCursor().beginEditBlock();
            QTextCursor newCursor = textCursor();
            newCursor.setPosition(textCursor().selectionEnd());
            setTextCursor(newCursor);
            document()->setModified(false);
            textCursor().endEditBlock();
        }
        else
        {
            QTextEdit::keyPressEvent(e);
        }
    }
    else if (e->key() == Qt::Key_Delete)
    {
        if(textCursor().hasSelection())
        {
            QTextEdit::keyPressEvent(e);
            return;
        }

        if(toPlainText()[textCursor().position()] == '\n')
        {
            textCursor().beginEditBlock();
            textCursor().deleteChar();
            QTextCursor editingTextCursor = textCursor();
            do
            {
                editingTextCursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor );
            } while (toPlainText()[editingTextCursor.position()] == ' ');

            editingTextCursor.removeSelectedText();
            textCursor().endEditBlock();
        }
        else if(toPlainText()[textCursor().position()] == ' ')
        {
            QTextCursor editingTextCursor = textCursor();

            textCursor().beginEditBlock();

            for(int i = 0; i < Settings::getTabSize(); i++)
            {
                if( toPlainText()[editingTextCursor.position()] == ' ' )
                {
                    editingTextCursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor );
                }
                else
                {
                    break;
                }
            }

            editingTextCursor.removeSelectedText();
            textCursor().endEditBlock();
        }
        else
        {
            QTextEdit::keyPressEvent(e);
        }
    }
    else if (e->key() == Qt::Key_Backspace)
    {
        if(textCursor().position()>0 && toPlainText()[textCursor().position()-1] == ' ' && !textCursor().hasSelection() )
        {
            QTextCursor editingTextCursor = textCursor();

            textCursor().beginEditBlock();

            for(int i = 0; i < Settings::getTabSize(); i++)
            {
                if( textCursor().position()>0 && toPlainText()[editingTextCursor.position()-1] == ' ' )
                {
                    editingTextCursor.movePosition( QTextCursor::Left, QTextCursor::KeepAnchor );
                }
                else
                {
                    break;
                }
            }

            editingTextCursor.removeSelectedText();
            textCursor().endEditBlock();
        }
        else
        {
            QTextEdit::keyPressEvent(e);
        }
    }
    else if (e->key() == Qt::Key_Home) {

        textCursor().beginEditBlock();
        QTextCursor editingTextCursor = textCursor();
        editingTextCursor.movePosition( QTextCursor::StartOfLine, (e->modifiers() & Qt::ShiftModifier) == 0 ? QTextCursor::MoveAnchor : QTextCursor::KeepAnchor );

        while (toPlainText()[editingTextCursor.position()] == ' ') 
        {
            editingTextCursor.movePosition( QTextCursor::Right, (e->modifiers() & Qt::ShiftModifier) == 0 ? QTextCursor::MoveAnchor : QTextCursor::KeepAnchor );
        }

        setTextCursor(editingTextCursor);
        document()->setModified(false);
        textCursor().endEditBlock();
    }
    else if (e->key() == Qt::Key_Space && (e->modifiers() & Qt::ControlModifier) != 0) {

        QTextCursor editingTextCursor = textCursor();

        editingTextCursor.setPosition(textCursor().selectionStart());
        editingTextCursor.movePosition( QTextCursor::StartOfWord, QTextCursor::KeepAnchor );

        AutoCompleter->setCompletionPrefix( editingTextCursor.selectedText().trimmed() );
        AutoCompleter->complete();
    }
    else
        QTextEdit::keyPressEvent(e);

    if(AutoCompleter->popup()->isVisible())
    {
        QTextCursor editingTextCursor = textCursor();

        editingTextCursor.setPosition(textCursor().selectionStart());
        editingTextCursor.movePosition( QTextCursor::StartOfWord, QTextCursor::KeepAnchor );
        AutoCompleter->setCompletionPrefix( editingTextCursor.selectedText().trimmed() );
    }

    if (document()->isModified())
        Rockete::getInstance().codeTextChanged();

}

void CodeEditor::keyReleaseEvent(QKeyEvent * /*e*/)
{
}