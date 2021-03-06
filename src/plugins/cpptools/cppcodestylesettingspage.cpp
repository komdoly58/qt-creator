#include "cppcodestylesettingspage.h"
#include "cppcodestylepreferences.h"
#include "ui_cppcodestylesettingspage.h"
#include "cpptoolsconstants.h"
#include "cpptoolssettings.h"
#include "cppqtstyleindenter.h"
#include <texteditor/snippets/isnippetprovider.h>
#include <texteditor/fontsettings.h>
#include <texteditor/displaysettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/tabpreferences.h>
#include <extensionsystem/pluginmanager.h>
#include <cppeditor/cppeditorconstants.h>
#include <coreplugin/icore.h>
#include <QtGui/QTextBlock>
#include <QtCore/QTextStream>

static const char *defaultCodeStyleSnippets[] = {
    "#include <math.h>\n"
    "\n"
    "class Complex\n"
    "    {\n"
    "public:\n"
    "    Complex(double re, double im)\n"
    "        : _re(re), _im(im)\n"
    "        {}\n"
    "    double modulus() const\n"
    "        {\n"
    "        return sqrt(_re * _re + _im * _im);\n"
    "        }\n"
    "private:\n"
    "    double _re;\n"
    "    double _im;\n"
    "    };\n"
    "\n"
    "void bar(int i)\n"
    "    {\n"
    "    static int counter = 0;\n"
    "    counter += i;\n"
    "    }\n"
    "\n"
    "namespace Foo\n"
    "    {\n"
    "    namespace Bar\n"
    "        {\n"
    "        void foo(int a, int b)\n"
    "            {\n"
    "            for (int i = 0; i < a; i++)\n"
    "                {\n"
    "                if (i < b)\n"
    "                    bar(i);\n"
    "                else\n"
    "                    {\n"
    "                    bar(i);\n"
    "                    bar(b);\n"
    "                    }\n"
    "                }\n"
    "            }\n"
    "        } // namespace Bar\n"
    "    } // namespace Foo\n"
    ,
    "#include <math.h>\n"
    "\n"
    "class Complex\n"
    "    {\n"
    "public:\n"
    "    Complex(double re, double im)\n"
    "        : _re(re), _im(im)\n"
    "        {}\n"
    "    double modulus() const\n"
    "        {\n"
    "        return sqrt(_re * _re + _im * _im);\n"
    "        }\n"
    "private:\n"
    "    double _re;\n"
    "    double _im;\n"
    "    };\n"
    "\n"
    "void bar(int i)\n"
    "    {\n"
    "    static int counter = 0;\n"
    "    counter += i;\n"
    "    }\n"
    "\n"
    "namespace Foo\n"
    "    {\n"
    "    namespace Bar\n"
    "        {\n"
    "        void foo(int a, int b)\n"
    "            {\n"
    "            for (int i = 0; i < a; i++)\n"
    "                {\n"
    "                if (i < b)\n"
    "                    bar(i);\n"
    "                else\n"
    "                    {\n"
    "                    bar(i);\n"
    "                    bar(b);\n"
    "                    }\n"
    "                }\n"
    "            }\n"
    "        } // namespace Bar\n"
    "    } // namespace Foo\n"
    ,
    "namespace Foo\n"
    "{\n"
    "namespace Bar\n"
    "{\n"
    "class FooBar\n"
    "    {\n"
    "public:\n"
    "    FooBar(int a)\n"
    "        : _a(a)\n"
    "        {}\n"
    "    int calculate() const\n"
    "        {\n"
    "        if (a > 10)\n"
    "            {\n"
    "            int b = 2 * a;\n"
    "            return a * b;\n"
    "            }\n"
    "        return -a;\n"
    "        }\n"
    "private:\n"
    "    int _a;\n"
    "    };\n"
    "}\n"
    "}\n"
    ,
    "#include \"bar.h\"\n"
    "\n"
    "int foo(int a)\n"
    "    {\n"
    "    switch (a)\n"
    "        {\n"
    "        case 1:\n"
    "            bar(1);\n"
    "            break;\n"
    "        case 2:\n"
    "            {\n"
    "            bar(2);\n"
    "            break;\n"
    "            }\n"
    "        case 3:\n"
    "        default:\n"
    "            bar(3);\n"
    "            break;\n"
    "        }\n"
    "    return 0;\n"
    "    }\n"
    ,
    "void foo() {\n"
    "    if (a &&\n"
    "        b)\n"
    "        c;\n"
    "\n"
    "    while (a ||\n"
    "           b)\n"
    "        break;\n"
    "    a = b +\n"
    "        c;\n"
    "    myInstance.longMemberName +=\n"
    "            foo;\n"
    "    myInstance.longMemberName += bar +\n"
    "                                 foo;\n"
    "}\n"
};

using namespace TextEditor;

namespace CppTools {

namespace Internal {

// ------------------ CppCodeStyleSettingsWidget

CppCodeStylePreferencesWidget::CppCodeStylePreferencesWidget(QWidget *parent)
    : QWidget(parent),
      m_tabPreferences(0),
      m_cppCodeStylePreferences(0),
      m_ui(new Ui::CppCodeStyleSettingsPage)
{
    m_ui->setupUi(this);
    m_ui->categoryTab->setProperty("_q_custom_style_disabled", true);

    m_previews << m_ui->previewTextEditGeneral << m_ui->previewTextEditContent
               << m_ui->previewTextEditBraces << m_ui->previewTextEditSwitch
               << m_ui->previewTextEditPadding;
    for (int i = 0; i < m_previews.size(); ++i) {
        m_previews[i]->setPlainText(defaultCodeStyleSnippets[i]);
    }

    TextEditor::TextEditorSettings *settings = TextEditorSettings::instance();
    decorateEditors(settings->fontSettings());
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
       this, SLOT(decorateEditors(TextEditor::FontSettings)));

    setVisualizeWhitespace(true);

    connect(m_ui->indentBlockBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentBlockBody, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentClassBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentNamespaceBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentEnumBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentNamespaceBody, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentSwitchLabels, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentCaseStatements, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentCaseBlocks, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentCaseBreak, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentAccessSpecifiers, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentDeclarationsRelativeToAccessSpecifiers, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentFunctionBody, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->indentFunctionBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->extraPaddingConditions, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));
    connect(m_ui->alignAssignments, SIGNAL(toggled(bool)),
       this, SLOT(slotCppCodeStyleSettingsChanged()));

    m_ui->categoryTab->setCurrentIndex(0);

    m_ui->tabPreferencesWidget->setFlat(true);
    m_ui->fallbackWidget->setLabelText(tr("Code style settings:"));
}

CppCodeStylePreferencesWidget::~CppCodeStylePreferencesWidget()
{
    delete m_ui;
}

void CppCodeStylePreferencesWidget::setTabPreferences(TextEditor::TabPreferences *preferences)
{
    m_tabPreferences = preferences;
    m_ui->tabPreferencesWidget->setTabPreferences(preferences);
    connect(m_tabPreferences, SIGNAL(currentSettingsChanged(TextEditor::TabSettings)),
            this, SLOT(slotSettingsChanged()));
    updatePreview();
}

void CppCodeStylePreferencesWidget::setCppCodeStylePreferences(CppCodeStylePreferences *preferences)
{
    m_cppCodeStylePreferences = preferences;
    m_ui->fallbackWidget->setFallbackPreferences(preferences);
    m_ui->fallbackContainer->setVisible(!m_ui->fallbackWidget->isHidden());

    connect(m_cppCodeStylePreferences, SIGNAL(settingsChanged(CppTools::CppCodeStyleSettings)),
            this, SLOT(setCppCodeStyleSettings(CppTools::CppCodeStyleSettings)));
    connect(m_cppCodeStylePreferences, SIGNAL(currentFallbackChanged(TextEditor::IFallbackPreferences*)),
            this, SLOT(slotCurrentFallbackChanged(TextEditor::IFallbackPreferences*)));
    connect(this, SIGNAL(cppCodeStyleSettingsChanged(CppTools::CppCodeStyleSettings)),
            m_cppCodeStylePreferences, SLOT(setSettings(CppTools::CppCodeStyleSettings)));

    setCppCodeStyleSettings(m_cppCodeStylePreferences->settings());
    slotCurrentFallbackChanged(m_cppCodeStylePreferences->currentFallback());

    connect(m_cppCodeStylePreferences, SIGNAL(currentSettingsChanged(CppTools::CppCodeStyleSettings)),
            this, SLOT(slotSettingsChanged()));
    updatePreview();
}

CppCodeStyleSettings CppCodeStylePreferencesWidget::cppCodeStyleSettings() const
{
    CppCodeStyleSettings set;

    set.indentBlockBraces = m_ui->indentBlockBraces->isChecked();
    set.indentBlockBody = m_ui->indentBlockBody->isChecked();
    set.indentClassBraces = m_ui->indentClassBraces->isChecked();
    set.indentEnumBraces = m_ui->indentEnumBraces->isChecked();
    set.indentNamespaceBraces = m_ui->indentNamespaceBraces->isChecked();
    set.indentNamespaceBody = m_ui->indentNamespaceBody->isChecked();
    set.indentAccessSpecifiers = m_ui->indentAccessSpecifiers->isChecked();
    set.indentDeclarationsRelativeToAccessSpecifiers = m_ui->indentDeclarationsRelativeToAccessSpecifiers->isChecked();
    set.indentFunctionBody = m_ui->indentFunctionBody->isChecked();
    set.indentFunctionBraces = m_ui->indentFunctionBraces->isChecked();
    set.indentSwitchLabels = m_ui->indentSwitchLabels->isChecked();
    set.indentStatementsRelativeToSwitchLabels = m_ui->indentCaseStatements->isChecked();
    set.indentBlocksRelativeToSwitchLabels = m_ui->indentCaseBlocks->isChecked();
    set.indentControlFlowRelativeToSwitchLabels = m_ui->indentCaseBreak->isChecked();
    set.extraPaddingForConditionsIfConfusingAlign = m_ui->extraPaddingConditions->isChecked();
    set.alignAssignments = m_ui->alignAssignments->isChecked();

    return set;
}

void CppCodeStylePreferencesWidget::setCppCodeStyleSettings(const CppCodeStyleSettings &s)
{
    const bool wasBlocked = blockSignals(true);
    m_ui->indentBlockBraces->setChecked(s.indentBlockBraces);
    m_ui->indentBlockBody->setChecked(s.indentBlockBody);
    m_ui->indentClassBraces->setChecked(s.indentClassBraces);
    m_ui->indentEnumBraces->setChecked(s.indentEnumBraces);
    m_ui->indentNamespaceBraces->setChecked(s.indentNamespaceBraces);
    m_ui->indentNamespaceBody->setChecked(s.indentNamespaceBody);
    m_ui->indentAccessSpecifiers->setChecked(s.indentAccessSpecifiers);
    m_ui->indentDeclarationsRelativeToAccessSpecifiers->setChecked(s.indentDeclarationsRelativeToAccessSpecifiers);
    m_ui->indentFunctionBody->setChecked(s.indentFunctionBody);
    m_ui->indentFunctionBraces->setChecked(s.indentFunctionBraces);
    m_ui->indentSwitchLabels->setChecked(s.indentSwitchLabels);
    m_ui->indentCaseStatements->setChecked(s.indentStatementsRelativeToSwitchLabels);
    m_ui->indentCaseBlocks->setChecked(s.indentBlocksRelativeToSwitchLabels);
    m_ui->indentCaseBreak->setChecked(s.indentControlFlowRelativeToSwitchLabels);
    m_ui->extraPaddingConditions->setChecked(s.extraPaddingForConditionsIfConfusingAlign);
    m_ui->alignAssignments->setChecked(s.alignAssignments);
    blockSignals(wasBlocked);

    updatePreview();
}

void CppCodeStylePreferencesWidget::slotCurrentFallbackChanged(TextEditor::IFallbackPreferences *fallback)
{
    m_ui->contentGroupBox->setEnabled(!fallback);
    m_ui->bracesGroupBox->setEnabled(!fallback);
    m_ui->switchGroupBox->setEnabled(!fallback);
    m_ui->alignmentGroupBox->setEnabled(!fallback);

    updatePreview();
}

QString CppCodeStylePreferencesWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
       << sep << m_ui->tabPreferencesWidget->searchKeywords()
       << sep << m_ui->fallbackWidget->searchKeywords()
       << sep << m_ui->indentBlockBraces->text()
       << sep << m_ui->indentBlockBody->text()
       << sep << m_ui->indentClassBraces->text()
       << sep << m_ui->indentEnumBraces->text()
       << sep << m_ui->indentNamespaceBraces->text()
       << sep << m_ui->indentNamespaceBody->text()
       << sep << m_ui->indentAccessSpecifiers->text()
       << sep << m_ui->indentDeclarationsRelativeToAccessSpecifiers->text()
       << sep << m_ui->indentFunctionBody->text()
       << sep << m_ui->indentFunctionBraces->text()
       << sep << m_ui->indentSwitchLabels->text()
       << sep << m_ui->indentCaseStatements->text()
       << sep << m_ui->indentCaseBlocks->text()
       << sep << m_ui->indentCaseBreak->text()
       << sep << m_ui->contentGroupBox->title()
       << sep << m_ui->bracesGroupBox->title()
       << sep << m_ui->switchGroupBox->title()
       << sep << m_ui->alignmentGroupBox->title()
       << sep << m_ui->extraPaddingConditions->text()
       << sep << m_ui->alignAssignments->text()
          ;
    for (int i = 0; i < m_ui->categoryTab->count(); i++)
        QTextStream(&rc) << sep << m_ui->categoryTab->tabText(i);
    rc.remove(QLatin1Char('&'));
    return rc;
}

void CppCodeStylePreferencesWidget::slotCppCodeStyleSettingsChanged()
{
    emit cppCodeStyleSettingsChanged(cppCodeStyleSettings());
    updatePreview();
}

void CppCodeStylePreferencesWidget::slotSettingsChanged()
{
    updatePreview();
}

void CppCodeStylePreferencesWidget::updatePreview()
{
    foreach (TextEditor::SnippetEditorWidget *preview, m_previews) {
        QTextDocument *doc = preview->document();

        const TextEditor::TabSettings ts = m_tabPreferences
                ? m_tabPreferences->currentSettings()
                : CppToolsSettings::instance()->tabPreferences()->settings();
        CppCodeStylePreferences *cppCodeStylePreferences = m_cppCodeStylePreferences
                ? m_cppCodeStylePreferences
                : CppToolsSettings::instance()->cppCodeStylePreferences();
        const CppCodeStyleSettings ccss = cppCodeStylePreferences->currentSettings();
        preview->setTabSettings(ts);
        preview->setCodeStylePreferences(cppCodeStylePreferences);
        QtStyleCodeFormatter formatter(ts, ccss);
        formatter.invalidateCache(doc);

        QTextBlock block = doc->firstBlock();
        QTextCursor tc = preview->textCursor();
        tc.beginEditBlock();
        while (block.isValid()) {
            int indent;
            int padding;
            formatter.indentFor(block, &indent, &padding);
            ts.indentLine(block, indent + padding, padding);
            formatter.updateLineStateChange(block);

            block = block.next();
        }
        tc.endEditBlock();
    }
}

void CppCodeStylePreferencesWidget::decorateEditors(const TextEditor::FontSettings &fontSettings)
{
    const ISnippetProvider *provider = 0;
    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::instance()->getObjects<ISnippetProvider>();
    foreach (const ISnippetProvider *current, providers) {
        if (current->groupId() == QLatin1String(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID)) {
            provider = current;
            break;
        }
    }

    foreach (TextEditor::SnippetEditorWidget *editor, m_previews) {
        editor->setFontSettings(fontSettings);
        if (provider)
            provider->decorateEditor(editor);
    }
}

void CppCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    foreach (TextEditor::SnippetEditorWidget *editor, m_previews) {
        DisplaySettings displaySettings = editor->displaySettings();
        displaySettings.m_visualizeWhitespace = on;
        editor->setDisplaySettings(displaySettings);
    }
}


// ------------------ CppCodeStyleSettingsPage

CppCodeStyleSettingsPage::CppCodeStyleSettingsPage(
        QWidget *parent) :
    Core::IOptionsPage(parent),
    m_pageTabPreferences(0)
{
}

CppCodeStyleSettingsPage::~CppCodeStyleSettingsPage()
{
}

QString CppCodeStyleSettingsPage::id() const
{
    return QLatin1String(Constants::CPP_CODE_STYLE_SETTINGS_ID);
}

QString CppCodeStyleSettingsPage::displayName() const
{
    return QCoreApplication::translate("CppTools", Constants::CPP_CODE_STYLE_SETTINGS_NAME);
}

QString CppCodeStyleSettingsPage::category() const
{
    return QLatin1String(Constants::CPP_SETTINGS_CATEGORY);
}

QString CppCodeStyleSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("CppTools", Constants::CPP_SETTINGS_TR_CATEGORY);
}

QIcon CppCodeStyleSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::SETTINGS_CATEGORY_CPP_ICON));
}

QWidget *CppCodeStyleSettingsPage::createPage(QWidget *parent)
{
    m_widget = new CppCodeStylePreferencesWidget(parent);

    TextEditor::TabPreferences *originalTabPreferences
            = CppToolsSettings::instance()->tabPreferences();
    m_pageTabPreferences = new TextEditor::TabPreferences(originalTabPreferences->fallbacks(), m_widget);
    m_pageTabPreferences->setSettings(originalTabPreferences->settings());
    m_pageTabPreferences->setCurrentFallback(originalTabPreferences->currentFallback());
    m_widget->setTabPreferences(m_pageTabPreferences);

    CppCodeStylePreferences *originalCodeStylePreferences
            = CppToolsSettings::instance()->cppCodeStylePreferences();
    m_pageCppCodeStylePreferences = new CppCodeStylePreferences(originalCodeStylePreferences->fallbacks(), m_widget);
    m_pageCppCodeStylePreferences->setSettings(originalCodeStylePreferences->settings());
    m_pageCppCodeStylePreferences->setCurrentFallback(originalCodeStylePreferences->currentFallback());
    m_widget->setCppCodeStylePreferences(m_pageCppCodeStylePreferences);

    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void CppCodeStyleSettingsPage::apply()
{
    if (m_widget) {
        QSettings *s = Core::ICore::instance()->settings();

        TextEditor::TabPreferences *originalTabPreferences = CppToolsSettings::instance()->tabPreferences();
        if (originalTabPreferences->settings() != m_pageTabPreferences->settings()) {
            originalTabPreferences->setSettings(m_pageTabPreferences->settings());
            if (s)
                originalTabPreferences->toSettings(CppTools::Constants::CPP_SETTINGS_ID, s);
        }
        if (originalTabPreferences->currentFallback() != m_pageTabPreferences->currentFallback()) {
            originalTabPreferences->setCurrentFallback(m_pageTabPreferences->currentFallback());
            if (s)
                originalTabPreferences->toSettings(CppTools::Constants::CPP_SETTINGS_ID, s);
        }

        CppCodeStylePreferences *originalCppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStylePreferences();
        if (originalCppCodeStylePreferences->settings() != m_pageCppCodeStylePreferences->settings()) {
            originalCppCodeStylePreferences->setSettings(m_pageCppCodeStylePreferences->settings());
            if (s)
                originalCppCodeStylePreferences->toSettings(CppTools::Constants::CPP_SETTINGS_ID, s);
        }
        if (originalCppCodeStylePreferences->currentFallback() != m_pageCppCodeStylePreferences->currentFallback()) {
            originalCppCodeStylePreferences->setCurrentFallback(m_pageCppCodeStylePreferences->currentFallback());
            if (s)
                originalCppCodeStylePreferences->toSettings(CppTools::Constants::CPP_SETTINGS_ID, s);
        }
    }
}

bool CppCodeStyleSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace CppTools
