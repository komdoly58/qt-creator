/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef MAEMORUNCONFIGURATIONWIDGET_H
#define MAEMORUNCONFIGURATIONWIDGET_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QModelIndex;
class QPushButton;
class QRadioButton;
class QTableView;
class QToolButton;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
class EnvironmentItem;
}

namespace ProjectExplorer {
class EnvironmentWidget;
}

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
}

namespace RemoteLinux {
class RemoteLinuxRunConfiguration;

namespace Internal {
class MaemoDeviceEnvReader;

class MaemoRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MaemoRunConfigurationWidget(RemoteLinuxRunConfiguration *runConfiguration,
                                         QWidget *parent = 0);

private slots:
    void runConfigurationEnabledChange(bool enabled);
    void argumentsEdited(const QString &args);
    void showDeviceConfigurationsDialog(const QString &link);
    void updateTargetInformation();
    void handleCurrentDeviceConfigChanged();
    void addMount();
    void removeMount();
    void changeLocalMountDir(const QModelIndex &index);
    void enableOrDisableRemoveMountSpecButton();
    void fetchEnvironment();
    void fetchEnvironmentFinished();
    void fetchEnvironmentError(const QString &error);
    void stopFetchEnvironment();
    void userChangesEdited();
    void baseEnvironmentSelected(int index);
    void baseEnvironmentChanged();
    void systemEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges);
    void handleRemoteMountsChanged();
    void handleDebuggingTypeChanged();
    void handleDeploySpecsChanged();

private:
    void addDisabledLabel(QVBoxLayout *topLayout);
    void addGenericWidgets(QVBoxLayout *mainLayout);
    void addMountWidgets(QVBoxLayout *mainLayout);
    void addEnvironmentWidgets(QVBoxLayout *mainLayout);
    void updateMountWarning();

    QWidget *topWidget;
    QLabel *m_disabledIcon;
    QLabel *m_disabledReason;
    QLineEdit *m_argsLineEdit;
    QLabel *m_localExecutableLabel;
    QLabel *m_remoteExecutableLabel;
    QLabel *m_devConfLabel;
    QLabel *m_debuggingLanguagesLabel;
    QRadioButton *m_debugCppOnlyButton;
    QRadioButton *m_debugQmlOnlyButton;
    QRadioButton *m_debugCppAndQmlButton;
    QLabel *m_mountWarningLabel;
    QTableView *m_mountView;
    QToolButton *m_removeMountButton;
    Utils::DetailsWidget *m_mountDetailsContainer;
    RemoteLinuxRunConfiguration *m_runConfiguration;

    bool m_ignoreChange;
    QPushButton *m_fetchEnv;
    QComboBox *m_baseEnvironmentComboBox;
    MaemoDeviceEnvReader *m_deviceEnvReader;
    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
    bool m_deployablesConnected;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMORUNCONFIGURATIONWIDGET_H
