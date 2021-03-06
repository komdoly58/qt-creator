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

#ifndef QMLJSLINK_H
#define QMLJSLINK_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <languageutils/componentversion.h>

#include <QtCore/QCoreApplication>

namespace QmlJS {

class NameId;
class LinkPrivate;

/*
    Helper for building a context.
*/
class QMLJS_EXPORT Link
{
    Q_DECLARE_PRIVATE(Link)
    Q_DECLARE_TR_FUNCTIONS(QmlJS::Link)

public:
    Link(Interpreter::Context *context, const Snapshot &snapshot,
         const QStringList &importPaths);

    // Link all documents in snapshot, collecting all diagnostic messages (if messages != 0)
    void operator()(QHash<QString, QList<DiagnosticMessage> > *messages = 0);

    // Link all documents in snapshot, appending the diagnostic messages
    // for 'doc' in 'messages'
    void operator()(const Document::Ptr &doc, QList<DiagnosticMessage> *messages);

    ~Link();

private:
    Interpreter::Engine *engine();

    static AST::UiQualifiedId *qualifiedTypeNameId(AST::Node *node);

    void linkImports();

    void populateImportedTypes(Interpreter::Imports *imports, Document::Ptr doc);
    Interpreter::Import importFileOrDirectory(
        Document::Ptr doc,
        const Interpreter::ImportInfo &importInfo);
    Interpreter::Import importNonFile(
        Document::Ptr doc,
        const Interpreter::ImportInfo &importInfo);
    void importObject(Bind *bind, const QString &name, Interpreter::ObjectValue *object, NameId *targetNamespace);

    bool importLibrary(Document::Ptr doc,
                       const QString &libraryPath,
                       Interpreter::Import *import,
                       const QString &importPath = QString());
    void loadQmldirComponents(Interpreter::ObjectValue *import,
                              LanguageUtils::ComponentVersion version,
                              const LibraryInfo &libraryInfo,
                              const QString &libraryPath);
    void loadImplicitDirectoryImports(Interpreter::Imports *imports, Document::Ptr doc);
    void loadImplicitDefaultImports(Interpreter::Imports *imports);

    void error(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message);
    void warning(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message);
    void appendDiagnostic(const Document::Ptr &doc, const DiagnosticMessage &message);

private:
    QScopedPointer<LinkPrivate> d_ptr;
};

} // namespace QmlJS

#endif // QMLJSLINK_H
