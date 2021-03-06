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

#include "consoleprocess_p.h"

namespace Utils {

ConsoleProcess::~ConsoleProcess()
{
    stop();
}

void ConsoleProcess::setMode(Mode m)
{
    d->m_mode = m;
}

ConsoleProcess::Mode ConsoleProcess::mode() const
{
    return d->m_mode;
}

QString ConsoleProcess::modeOption(Mode m)
{
    switch (m) {
        case Debug:
        return QLatin1String("debug");
        case Suspend:
        return QLatin1String("suspend");
        case Run:
        break;
    }
    return QLatin1String("run");
}

qint64 ConsoleProcess::applicationPID() const
{
    return d->m_appPid;
}

int ConsoleProcess::exitCode() const
{
    return d->m_appCode;
} // This will be the signal number if exitStatus == CrashExit

QProcess::ExitStatus ConsoleProcess::exitStatus() const
{
    return d->m_appStatus;
}

void ConsoleProcess::setWorkingDirectory(const QString &dir)
{
    d->m_workingDir = dir;
}

QString ConsoleProcess::workingDirectory() const
{
    return d->m_workingDir;
}

void ConsoleProcess::setEnvironment(const Environment &env)
{
    d->m_environment = env;
}

Environment ConsoleProcess::environment() const
{
    return d->m_environment;
}

QString ConsoleProcess::msgCommChannelFailed(const QString &error)
{
    return tr("Cannot set up communication channel: %1").arg(error);
}

QString ConsoleProcess::msgPromptToClose()
{
    //! Showed in a terminal which might have
    //! a different character set on Windows.
    return tr("Press <RETURN> to close this window...");
}

QString ConsoleProcess::msgCannotCreateTempFile(const QString &why)
{
    return tr("Cannot create temporary file: %1").arg(why);
}

QString ConsoleProcess::msgCannotWriteTempFile()
{
    return tr("Cannot write temporary file. Disk full?");
}

QString ConsoleProcess::msgCannotCreateTempDir(const QString & dir, const QString &why)
{
    return tr("Cannot create temporary directory '%1': %2").arg(dir, why);
}

QString ConsoleProcess::msgUnexpectedOutput(const QByteArray &what)
{
    return tr("Unexpected output from helper program (%1).").arg(QString::fromAscii(what));
}

QString ConsoleProcess::msgCannotChangeToWorkDir(const QString & dir, const QString &why)
{
    return tr("Cannot change to working directory '%1': %2").arg(dir, why);
}

QString ConsoleProcess::msgCannotExecute(const QString & p, const QString &why)
{
    return tr("Cannot execute '%1': %2").arg(p, why);
}

}
