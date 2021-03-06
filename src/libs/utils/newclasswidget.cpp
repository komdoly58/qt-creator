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

#include "newclasswidget.h"
#include "ui_newclasswidget.h"

#include <utils/filewizardpage.h>

#include <QtGui/QFileDialog>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QRegExp>

enum { debugNewClassWidget = 0 };

/*! \class Utils::NewClassWidget

    \brief Utility widget for 'New Class' wizards

    Utility widget for 'New Class' wizards. Prompts the user
    to enter a class name (optionally derived from some base class) and file
    names for header, source and form files. Has some smart logic to derive
    the file names from the class name. */

namespace Utils {

struct NewClassWidgetPrivate {
    NewClassWidgetPrivate();

    Ui::NewClassWidget m_ui;
    QString m_headerExtension;
    QString m_sourceExtension;
    QString m_formExtension;
    bool m_valid;
    bool m_classEdited;
    // Store the "visible" values to prevent the READ accessors from being
    // fooled by a temporarily hidden widget
    bool m_baseClassInputVisible;
    bool m_formInputVisible;
    bool m_pathInputVisible;
    bool m_qobjectCheckBoxVisible;
    bool m_formInputCheckable;
};

NewClassWidgetPrivate:: NewClassWidgetPrivate() :
    m_headerExtension(QLatin1String("h")),
    m_sourceExtension(QLatin1String("cpp")),
    m_formExtension(QLatin1String("ui")),
    m_valid(false),
    m_classEdited(false),
    m_baseClassInputVisible(true),
    m_formInputVisible(true),
    m_pathInputVisible(true),
    m_qobjectCheckBoxVisible(false),
    m_formInputCheckable(false)

{
}

// --------------------- NewClassWidget
NewClassWidget::NewClassWidget(QWidget *parent) :
    QWidget(parent),
    m_d(new NewClassWidgetPrivate)
{
    m_d->m_ui.setupUi(this);

    m_d->m_ui.baseClassComboBox->setEditable(false);

    connect(m_d->m_ui.classLineEdit, SIGNAL(updateFileName(QString)),
            this, SLOT(slotUpdateFileNames(QString)));
    connect(m_d->m_ui.classLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(classNameEdited()));
    connect(m_d->m_ui.baseClassComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(suggestClassNameFromBase()));
    connect(m_d->m_ui.baseClassComboBox, SIGNAL(editTextChanged(QString)),
            this, SLOT(slotValidChanged()));
    connect(m_d->m_ui.classLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(m_d->m_ui.headerFileLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(m_d->m_ui.sourceFileLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(m_d->m_ui.formFileLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(m_d->m_ui.pathChooser, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(m_d->m_ui.generateFormCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotValidChanged()));

    connect(m_d->m_ui.classLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(m_d->m_ui.headerFileLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(m_d->m_ui.sourceFileLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(m_d->m_ui.formFileLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(m_d->m_ui.formFileLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(m_d->m_ui.pathChooser, SIGNAL(returnPressed()),
             this, SLOT(slotActivated()));

    connect(m_d->m_ui.generateFormCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(slotFormInputChecked()));
    connect(m_d->m_ui.baseClassComboBox, SIGNAL(editTextChanged(QString)),
            this, SLOT(slotBaseClassEdited(QString)));
    m_d->m_ui.generateFormCheckBox->setChecked(true);
    setFormInputCheckable(false, true);
    setClassType(NoClassType);
}

NewClassWidget::~NewClassWidget()
{
    delete m_d;
}

void NewClassWidget::classNameEdited()
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << m_d->m_headerExtension << m_d->m_sourceExtension;
    m_d->m_classEdited = true;
}

void NewClassWidget::suggestClassNameFromBase()
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << m_d->m_headerExtension << m_d->m_sourceExtension;
    if (m_d->m_classEdited)
        return;
    // Suggest a class unless edited ("QMainWindow"->"MainWindow")
    QString base = baseClassName();
    if (base.startsWith(QLatin1Char('Q'))) {
        base.remove(0, 1);
        setClassName(base);
    }
}

QStringList NewClassWidget::baseClassChoices() const
{
    QStringList rc;
    const int count = m_d->m_ui.baseClassComboBox->count();
    for (int i = 0; i <  count; i++)
        rc.push_back(m_d->m_ui.baseClassComboBox->itemText(i));
    return rc;
}

void NewClassWidget::setBaseClassChoices(const QStringList &choices)
{
    m_d->m_ui.baseClassComboBox->clear();
    m_d->m_ui.baseClassComboBox->addItems(choices);
}

void NewClassWidget::setBaseClassInputVisible(bool visible)
{
    m_d->m_baseClassInputVisible = visible;
    m_d->m_ui.baseClassLabel->setVisible(visible);
    m_d->m_ui.baseClassComboBox->setVisible(visible);
}

void NewClassWidget::setBaseClassEditable(bool editable)
{
    m_d->m_ui.baseClassComboBox->setEditable(editable);
}

bool NewClassWidget::isBaseClassInputVisible() const
{
    return  m_d->m_baseClassInputVisible;
}

bool NewClassWidget::isBaseClassEditable() const
{
    return  m_d->m_ui.baseClassComboBox->isEditable();
}

void NewClassWidget::setFormInputVisible(bool visible)
{
    m_d->m_formInputVisible = visible;
    m_d->m_ui.formLabel->setVisible(visible);
    m_d->m_ui.formFileLineEdit->setVisible(visible);
}

bool NewClassWidget::isFormInputVisible() const
{
    return m_d->m_formInputVisible;
}

void NewClassWidget::setFormInputCheckable(bool checkable)
{
    setFormInputCheckable(checkable, false);
}

void NewClassWidget::setFormInputCheckable(bool checkable, bool force)
{
    if (!force && checkable == m_d->m_formInputCheckable)
        return;
    m_d->m_formInputCheckable = checkable;
    m_d->m_ui.generateFormLabel->setVisible(checkable);
    m_d->m_ui.generateFormCheckBox->setVisible(checkable);
}

void NewClassWidget::setFormInputChecked(bool v)
{
    m_d->m_ui.generateFormCheckBox->setChecked(v);
}

bool NewClassWidget::formInputCheckable() const
{
    return m_d->m_formInputCheckable;
}

bool NewClassWidget::formInputChecked() const
{
    return m_d->m_ui.generateFormCheckBox->isChecked();
}

void NewClassWidget::slotFormInputChecked()
{
    const bool checked = formInputChecked();
    m_d->m_ui.formLabel->setEnabled(checked);
    m_d->m_ui.formFileLineEdit->setEnabled(checked);
}

void NewClassWidget::setPathInputVisible(bool visible)
{
    m_d->m_pathInputVisible = visible;
    m_d->m_ui.pathLabel->setVisible(visible);
    m_d->m_ui.pathChooser->setVisible(visible);
}

bool NewClassWidget::isPathInputVisible() const
{
    return m_d->m_pathInputVisible;
}

void NewClassWidget::setClassName(const QString &suggestedName)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << suggestedName << m_d->m_headerExtension << m_d->m_sourceExtension;
    m_d->m_ui.classLineEdit->setText(ClassNameValidatingLineEdit::createClassName(suggestedName));
}

QString NewClassWidget::className() const
{
    return m_d->m_ui.classLineEdit->text();
}

QString NewClassWidget::baseClassName() const
{
    return m_d->m_ui.baseClassComboBox->currentText();
}

void NewClassWidget::setBaseClassName(const QString &c)
{
    const int index = m_d->m_ui.baseClassComboBox->findText(c);
    if (index != -1) {
        m_d->m_ui.baseClassComboBox->setCurrentIndex(index);
        suggestClassNameFromBase();
    }
}

QString NewClassWidget::sourceFileName() const
{
    return m_d->m_ui.sourceFileLineEdit->text();
}

QString NewClassWidget::headerFileName() const
{
    return m_d->m_ui.headerFileLineEdit->text();
}

QString NewClassWidget::formFileName() const
{
    return m_d->m_ui.formFileLineEdit->text();
}

QString NewClassWidget::path() const
{
    return m_d->m_ui.pathChooser->path();
}

void NewClassWidget::setPath(const QString &path)
{
     m_d->m_ui.pathChooser->setPath(path);
}

bool NewClassWidget::namespacesEnabled() const
{
    return  m_d->m_ui.classLineEdit->namespacesEnabled();
}

void NewClassWidget::setNamespacesEnabled(bool b)
{
    m_d->m_ui.classLineEdit->setNamespacesEnabled(b);
}

QString NewClassWidget::sourceExtension() const
{
    return m_d->m_sourceExtension;
}

void NewClassWidget::setSourceExtension(const QString &e)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << e;
    m_d->m_sourceExtension = fixSuffix(e);
}

QString NewClassWidget::headerExtension() const
{
    return m_d->m_headerExtension;
}

void NewClassWidget::setHeaderExtension(const QString &e)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << e;
    m_d->m_headerExtension = fixSuffix(e);
}

QString NewClassWidget::formExtension() const
{
    return m_d->m_formExtension;
}

void NewClassWidget::setFormExtension(const QString &e)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << e;
    m_d->m_formExtension = fixSuffix(e);
}

bool NewClassWidget::allowDirectories() const
{
    return m_d->m_ui.headerFileLineEdit->allowDirectories();
}

void NewClassWidget::setAllowDirectories(bool v)
{
    // We keep all in sync
    if (allowDirectories() != v) {
        m_d->m_ui.sourceFileLineEdit->setAllowDirectories(v);
        m_d->m_ui.headerFileLineEdit->setAllowDirectories(v);
        m_d->m_ui.formFileLineEdit->setAllowDirectories(v);
    }
}

bool NewClassWidget::lowerCaseFiles() const
{
    return m_d->m_ui.classLineEdit->lowerCaseFileName();
}

void NewClassWidget::setLowerCaseFiles(bool v)
{
    m_d->m_ui.classLineEdit->setLowerCaseFileName(v);
}

NewClassWidget::ClassType NewClassWidget::classType() const
{
    return static_cast<ClassType>(m_d->m_ui.classTypeComboBox->currentIndex());
}

void NewClassWidget::setClassType(ClassType ct)
{
    m_d->m_ui.classTypeComboBox->setCurrentIndex(ct);
}

bool NewClassWidget::isClassTypeComboVisible() const
{
    return m_d->m_ui.classTypeLabel->isVisible();
}

void NewClassWidget::setClassTypeComboVisible(bool v)
{
    m_d->m_ui.classTypeLabel->setVisible(v);
    m_d->m_ui.classTypeComboBox->setVisible(v);
}

// Guess suitable type information with some smartness
static inline NewClassWidget::ClassType classTypeForBaseClass(const QString &baseClass)
{
    if (!baseClass.startsWith(QLatin1Char('Q')))
        return NewClassWidget::NoClassType;
    // QObject types: QObject as such and models.
    if (baseClass == QLatin1String("QObject") ||
        (baseClass.startsWith(QLatin1String("QAbstract")) && baseClass.endsWith(QLatin1String("Model"))))
        return NewClassWidget::ClassInheritsQObject;
    // Widgets.
    if (baseClass == QLatin1String("QWidget") || baseClass == QLatin1String("QMainWindow")
        || baseClass == QLatin1String("QDialog"))
        return NewClassWidget::ClassInheritsQWidget;
    // Declarative Items
    if (baseClass == QLatin1String("QDeclarativeItem"))
        return NewClassWidget::ClassInheritsQDeclarativeItem;
    return NewClassWidget::NoClassType;
}

void NewClassWidget::slotBaseClassEdited(const QString &baseClass)
{
    // Set type information with some smartness.
    const ClassType currentClassType = classType();
    const ClassType recommendedClassType = classTypeForBaseClass(baseClass);
    if (recommendedClassType != NoClassType && currentClassType != recommendedClassType)
        setClassType(recommendedClassType);
}

void NewClassWidget::slotValidChanged()
{
    const bool newValid = isValid();
    if (newValid != m_d->m_valid) {
        m_d->m_valid = newValid;
        emit validChanged();
    }
}

bool NewClassWidget::isValid(QString *error) const
{
    if (!m_d->m_ui.classLineEdit->isValid()) {
        if (error)
            *error = m_d->m_ui.classLineEdit->errorMessage();
        return false;
    }

    if (isBaseClassInputVisible() && isBaseClassEditable()) {
        // TODO: Should this be a ClassNameValidatingComboBox?
        QRegExp classNameValidator(QLatin1String("[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*"));
        const QString baseClass = m_d->m_ui.baseClassComboBox->currentText().trimmed();
        if (!baseClass.isEmpty() && !classNameValidator.exactMatch(baseClass)) {
            if (error)
                *error = tr("Invalid base class name");
            return false;
        }
    }

    if (!m_d->m_ui.headerFileLineEdit->isValid()) {
        if (error)
            *error = tr("Invalid header file name: '%1'").arg(m_d->m_ui.headerFileLineEdit->errorMessage());
        return false;
    }

    if (!m_d->m_ui.sourceFileLineEdit->isValid()) {
        if (error)
            *error = tr("Invalid source file name: '%1'").arg(m_d->m_ui.sourceFileLineEdit->errorMessage());
        return false;
    }

    if (isFormInputVisible() &&
        (!m_d->m_formInputCheckable || m_d->m_ui.generateFormCheckBox->isChecked())) {
        if (!m_d->m_ui.formFileLineEdit->isValid()) {
            if (error)
                *error = tr("Invalid form file name: '%1'").arg(m_d->m_ui.formFileLineEdit->errorMessage());
            return false;
        }
    }

    if (isPathInputVisible()) {
        if (!m_d->m_ui.pathChooser->isValid()) {
            if (error)
                *error =  m_d->m_ui.pathChooser->errorMessage();
            return false;
        }
    }
    return true;
}

void NewClassWidget::triggerUpdateFileNames()
{
    m_d->m_ui.classLineEdit->triggerChanged();
}

void NewClassWidget::slotUpdateFileNames(const QString &baseName)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << baseName << m_d->m_headerExtension << m_d->m_sourceExtension;
    const QChar dot = QLatin1Char('.');
    m_d->m_ui.sourceFileLineEdit->setText(baseName + dot + m_d->m_sourceExtension);
    m_d->m_ui.headerFileLineEdit->setText(baseName + dot + m_d->m_headerExtension);
    m_d->m_ui.formFileLineEdit->setText(baseName + dot + m_d->m_formExtension);
}

void NewClassWidget::slotActivated()
{
    if (m_d->m_valid)
        emit activated();
}

QString NewClassWidget::fixSuffix(const QString &suffix)
{
    QString s = suffix;
    if (s.startsWith(QLatin1Char('.')))
        s.remove(0, 1);
    return s;
}

// Utility to add a suffix to a file unless the user specified one
static QString ensureSuffix(QString f, const QString &extension)
{
    const QChar dot = QLatin1Char('.');
    if (f.contains(dot))
        return f;
    f += dot;
    f += extension;
    return f;
}

// If a non-empty name was passed, expand to directory and suffix
static QString expandFileName(const QDir &dir, const QString name, const QString &extension)
{
    if (name.isEmpty())
        return QString();
    return dir.absoluteFilePath(ensureSuffix(name, extension));
}

QStringList NewClassWidget::files() const
{
    QStringList rc;
    const QDir dir = QDir(path());
    rc.push_back(expandFileName(dir, headerFileName(), headerExtension()));
    rc.push_back(expandFileName(dir, sourceFileName(), sourceExtension()));
    if (isFormInputVisible())
        rc.push_back(expandFileName(dir, formFileName(), formExtension()));
    return rc;
}

} // namespace Utils
