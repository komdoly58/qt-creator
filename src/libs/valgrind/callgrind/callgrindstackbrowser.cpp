/**************************************************************************
**
** This file is part of Qt Creator Analyzer Tools
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "callgrindstackbrowser.h"

using namespace Valgrind::Callgrind;

HistoryItem::HistoryItem(StackBrowser *stack)
{
    if (stack)
        stack->select(this);
}

HistoryItem::~HistoryItem()
{

}


FunctionHistoryItem::FunctionHistoryItem(const Function *function, StackBrowser *stack)
    : HistoryItem(stack)
    , m_function(function)
{
}

FunctionHistoryItem::~FunctionHistoryItem()
{
}

StackBrowser::StackBrowser(QObject *parent)
    : QObject(parent)
{
}

StackBrowser::~StackBrowser()
{
    qDeleteAll(m_stack);
    m_stack.clear();
}

void StackBrowser::clear()
{
    qDeleteAll(m_stack);
    m_stack.clear();
    emit currentChanged();
}

int StackBrowser::size() const
{
    return m_stack.size();
}

void StackBrowser::select(HistoryItem *item)
{
    if (!m_stack.isEmpty() && m_stack.top() == item)
        return;

    m_stack.push(item);
    emit currentChanged();
}

HistoryItem *StackBrowser::current() const
{
    return m_stack.isEmpty() ? 0 : m_stack.top();
}

void StackBrowser::goBack()
{
    if (m_stack.isEmpty())
        return;

    HistoryItem *item = m_stack.pop();
    delete item;
    emit currentChanged();
}