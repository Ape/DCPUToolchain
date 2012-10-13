#include "codeeditor.h"



CodeEditor::CodeEditor(QWidget* parent): QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);
    
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect, int)), this, SLOT(updateLineNumberArea(QRect, int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    emit blockCountChanged(0);
    highlightCurrentLine();

    highlighter = new DCPUHighlighter(document());
}

void CodeEditor::updateLineNumberAreaWidth(int newBlockCount)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());

    while(max >= 10)
    {
        max /= 10;
        digits++;
    }

    return 6 +  fontMetrics().width(QLatin1Char('1')) * digits;
}



void CodeEditor::updateLineNumberArea(const QRect& rect, int dy)
{
    if(dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if(rect.contains(viewport()->rect()))
        emit blockCountChanged(0);
}

void CodeEditor::resizeEvent(QResizeEvent* e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    return;
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while(block.isValid() && top <= event->rect().bottom())
    {
        if(block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);

            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(), Qt::AlignCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
   }
}

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Tab)
    {
        QKeyEvent* space = new QKeyEvent(
            event->type(),
            Qt::Key_Space,
            event->modifiers(),
            QString(" "));

        for(int i = 0; i < 4; i++)
            QPlainTextEdit::keyPressEvent(space);
    }
    else
        QPlainTextEdit::keyPressEvent(event);
}




    


