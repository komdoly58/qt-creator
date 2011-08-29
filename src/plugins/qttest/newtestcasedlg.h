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

#ifndef NEWTESTCASEDLG_H
#define NEWTESTCASEDLG_H

#include "testgenerator.h"
#include <QDialog>

namespace Ui {
    class NewTestCaseDlg;
}

class NewTestCaseDlg : public QDialog
{
    Q_OBJECT

public:
    explicit NewTestCaseDlg(const QString &path, QWidget *parent = 0);
    ~NewTestCaseDlg();

    TestGenerator::GenMode mode();
    QString testCaseName();
    QString location();
    QString testedClassName();
    QString testedComponent();
    QString actualPath();
    bool componentInName();

private slots:
    void onTextChanged(const QString &txt);
    void onLocationChanged(const QString &txt);
    void onChanged();
    void onLocationBtnClicked();
    void onComponentChkBoxChanged(int state);

private:
    Ui::NewTestCaseDlg *ui;
    bool m_pathIsEdited;
    QPalette m_originalPalette;
    TestGenerator m_testCaseGenerator;
};

#endif // NEWTESTCASEDLG_H