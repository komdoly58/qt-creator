/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "stackwindow.h"
#include "stackhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerdialogs.h"
#include "memoryagent.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>

namespace Debugger {
namespace Internal {

static DebuggerEngine *currentEngine()
{
    return debuggerCore()->currentEngine();
}

StackWindow::StackWindow(QWidget *parent)
    : QTreeView(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);

    QAction *act = debuggerCore()->action(UseAlternatingRowColors);
    setWindowTitle(tr("Stack"));

    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    header()->setDefaultAlignment(Qt::AlignLeft);

    connect(this, SIGNAL(activated(QModelIndex)),
        SLOT(rowActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
    connect(debuggerCore()->action(UseAddressInStackView), SIGNAL(toggled(bool)),
        SLOT(showAddressColumn(bool)));
    connect(debuggerCore()->action(ExpandStack), SIGNAL(triggered()),
        SLOT(reloadFullStack()));
    connect(debuggerCore()->action(MaximalStackDepth), SIGNAL(triggered()),
        SLOT(reloadFullStack()));
    connect(debuggerCore()->action(AlwaysAdjustStackColumnWidths),
        SIGNAL(triggered(bool)),
        SLOT(setAlwaysResizeColumnsToContents(bool)));
    showAddressColumn(false);
}

void StackWindow::showAddressColumn(bool on)
{
    setColumnHidden(4, !on);
}

void StackWindow::rowActivated(const QModelIndex &index)
{
    currentEngine()->activateFrame(index.row());
}

void StackWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    //resizeColumnsToContents();
    resizeColumnToContents(0);
    resizeColumnToContents(3);
}

void StackWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    DebuggerEngine *engine = currentEngine();
    StackHandler *handler = engine->stackHandler();
    const QModelIndex index = indexAt(ev->pos());
    const int row = index.row();
    const unsigned engineCapabilities = engine->debuggerCapabilities();
    StackFrame frame;
    if (row >= 0 && row < handler->stackSize())
        frame = handler->frameAt(row);
    const quint64 address = frame.address;

    QMenu menu;
    menu.addAction(debuggerCore()->action(ExpandStack));

    QAction *actCopyContents = menu.addAction(tr("Copy Contents to Clipboard"));
    actCopyContents->setEnabled(model() != 0);

    if (engineCapabilities & CreateFullBacktraceCapability)
        menu.addAction(debuggerCore()->action(CreateFullBacktrace));

    QAction *actShowMemory = menu.addAction(QString());
    if (address == 0) {
        actShowMemory->setText(tr("Open Memory Editor"));
        actShowMemory->setEnabled(false);
    } else {
        actShowMemory->setText(tr("Open Memory Editor at 0x%1").arg(address, 0, 16));
        actShowMemory->setEnabled(engineCapabilities & ShowMemoryCapability);
    }

    QAction *actShowDisassemblerAt = menu.addAction(QString());
    QAction *actShowDisassembler = menu.addAction(tr("Open Disassembler..."));
    actShowDisassembler->setEnabled(engineCapabilities & DisassemblerCapability);
    if (address == 0) {
        actShowDisassemblerAt->setText(tr("Open Disassembler"));
        actShowDisassemblerAt->setEnabled(false);
    } else {
        actShowDisassemblerAt->setText(tr("Open Disassembler at 0x%1").arg(address, 0, 16));
        actShowDisassemblerAt->setEnabled(engineCapabilities & DisassemblerCapability);
    }

    QAction *actLoadSymbols = 0;
    if (engineCapabilities & ShowModuleSymbolsCapability)
        actLoadSymbols = menu.addAction(tr("Try to Load Unknown Symbols"));

    menu.addSeparator();
#if 0 // @TODO: not implemented
    menu.addAction(debuggerCore()->action(UseToolTipsInStackView));
#endif
    menu.addAction(debuggerCore()->action(UseAddressInStackView));

    QAction *actAdjust = menu.addAction(tr("Adjust Column Widths to Contents"));
    menu.addAction(debuggerCore()->action(AlwaysAdjustStackColumnWidths));
    menu.addSeparator();

    menu.addAction(debuggerCore()->action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (!act)
        ;
    else if (act == actCopyContents)
        copyContentsToClipboard();
    else if (act == actAdjust)
        resizeColumnsToContents();
    else if (act == actShowMemory) {
        const QString title = tr("Memory at Frame #%1 (%2) 0x%3)").
        arg(row).arg(frame.function).arg(address, 0, 16);
        QList<MemoryMarkup> ml;
        ml.push_back(MemoryMarkup(address, 1, QColor(Qt::blue).lighter(),
                                  tr("Frame #%1 (%2)").arg(row).arg(frame.function)));
        engine->openMemoryView(address, 0, ml, QPoint(), title);
    } else if (act == actShowDisassembler) {
        AddressDialog dialog;
        if (address)
            dialog.setAddress(address);
        if (dialog.exec() == QDialog::Accepted)
            currentEngine()->openDisassemblerView(Location(dialog.address()));
    } else if (act == actShowDisassemblerAt)
        engine->openDisassemblerView(frame);
    else if (act == actLoadSymbols)
        engine->loadSymbolsForStack();
}

void StackWindow::copyContentsToClipboard()
{
    QString str;
    int n = model()->rowCount();
    int m = model()->columnCount();
    for (int i = 0; i != n; ++i) {
        for (int j = 0; j != m; ++j) {
            QModelIndex index = model()->index(i, j);
            str += model()->data(index).toString();
            str += '\t';
        }
        str += '\n';
    }
    QClipboard *clipboard = QApplication::clipboard();
#    ifdef Q_WS_X11
    clipboard->setText(str, QClipboard::Selection);
#    endif
    clipboard->setText(str, QClipboard::Clipboard);
}

void StackWindow::reloadFullStack()
{
    currentEngine()->reloadFullStack();
}

void StackWindow::resizeColumnsToContents()
{
    for (int i = model()->columnCount(); --i >= 0; )
        resizeColumnToContents(i);
}

void StackWindow::setAlwaysResizeColumnsToContents(bool on)
{
    QHeaderView::ResizeMode mode =
        on ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    for (int i = model()->columnCount(); --i >= 0; )
        header()->setResizeMode(i, mode);
}

} // namespace Internal
} // namespace Debugger
