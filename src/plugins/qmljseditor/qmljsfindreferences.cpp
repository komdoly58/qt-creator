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

#include "qmljsfindreferences.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/basefilefind.h>
#include <find/searchresultwindow.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/filesearch.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsscopeastpath.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/parser/qmljsast_p.h>

#include "qmljseditorconstants.h"

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QtConcurrentMap>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <qtconcurrent/runextensions.h>

#include <functional>

using namespace QmlJS;
using namespace QmlJS::Interpreter;
using namespace QmlJS::AST;
using namespace QmlJSEditor;

namespace {

// ### These visitors could be useful in general

class FindUsages: protected Visitor
{
public:
    typedef QList<AST::SourceLocation> Result;

    FindUsages(Document::Ptr doc, Context *context)
        : _doc(doc)
        , _context(context)
        , _builder(context, doc)
    {
        _builder.initializeRootScope();
    }

    Result operator()(const QString &name, const ObjectValue *scope)
    {
        _name = name;
        _scope = scope;
        _usages.clear();
        if (_doc)
            Node::accept(_doc->ast(), this);
        return _usages;
    }

protected:
    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;

    virtual bool visit(AST::UiPublicMember *node)
    {
        if (node->name
                && node->name->asString() == _name
                && _context->scopeChain().qmlScopeObjects.contains(_scope)) {
            _usages.append(node->identifierToken);
        }
        if (AST::cast<Block *>(node->statement)) {
            _builder.push(node);
            Node::accept(node->statement, this);
            _builder.pop();
            return false;
        }
        return true;
    }

    virtual bool visit(AST::UiObjectDefinition *node)
    {
        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::UiObjectBinding *node)
    {
        if (node->qualifiedId
                && !node->qualifiedId->next
                && node->qualifiedId->name->asString() == _name
                && checkQmlScope()) {
            _usages.append(node->qualifiedId->identifierToken);
        }

        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::UiScriptBinding *node)
    {
        if (node->qualifiedId
                && !node->qualifiedId->next
                && node->qualifiedId->name->asString() == _name
                && checkQmlScope()) {
            _usages.append(node->qualifiedId->identifierToken);
        }
        if (AST::cast<Block *>(node->statement)) {
            Node::accept(node->qualifiedId, this);
            _builder.push(node);
            Node::accept(node->statement, this);
            _builder.pop();
            return false;
        }
        return true;
    }

    virtual bool visit(AST::UiArrayBinding *node)
    {
        if (node->qualifiedId
                && !node->qualifiedId->next
                && node->qualifiedId->name->asString() == _name
                && checkQmlScope()) {
            _usages.append(node->qualifiedId->identifierToken);
        }
        return true;
    }

    virtual bool visit(AST::IdentifierExpression *node)
    {
        if (!node->name || node->name->asString() != _name)
            return false;

        const ObjectValue *scope;
        _context->lookup(_name, &scope);
        if (!scope)
            return false;
        if (check(scope)) {
            _usages.append(node->identifierToken);
            return false;
        }

        // the order of scopes in 'instantiatingComponents' is undefined,
        // so it might still be a use - we just found a different value in a different scope first

        // if scope is one of these, our match wasn't inside the instantiating components list
        const ScopeChain &chain = _context->scopeChain();
        if (chain.jsScopes.contains(scope)
                || chain.qmlScopeObjects.contains(scope)
                || chain.qmlTypes == scope
                || chain.globalScope == scope)
            return false;

        if (contains(chain.qmlComponentScope.data()))
            _usages.append(node->identifierToken);

        return false;
    }

    virtual bool visit(AST::FieldMemberExpression *node)
    {
        if (!node->name || node->name->asString() != _name)
            return true;

        Evaluate evaluate(_context);
        const Value *lhsValue = evaluate(node->base);
        if (!lhsValue)
            return true;

        if (check(lhsValue->asObjectValue())) // passing null is ok
            _usages.append(node->identifierToken);

        return true;
    }

    virtual bool visit(AST::FunctionDeclaration *node)
    {
        return visit(static_cast<FunctionExpression *>(node));
    }

    virtual bool visit(AST::FunctionExpression *node)
    {
        if (node->name && node->name->asString() == _name) {
            if (checkLookup())
                _usages.append(node->identifierToken);
        }
        Node::accept(node->formals, this);
        _builder.push(node);
        Node::accept(node->body, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::VariableDeclaration *node)
    {
        if (node->name && node->name->asString() == _name) {
            if (checkLookup())
                _usages.append(node->identifierToken);
        }
        return true;
    }

private:
    bool contains(const ScopeChain::QmlComponentChain *chain)
    {
        if (!chain || !chain->document)
            return false;

        if (chain->document->bind()->idEnvironment()->lookupMember(_name, _context))
            return chain->document->bind()->idEnvironment() == _scope;
        const ObjectValue *root = chain->document->bind()->rootObjectValue();
        if (root->lookupMember(_name, _context)) {
            return check(root);
        }

        foreach (const ScopeChain::QmlComponentChain *parent, chain->instantiatingComponents) {
            if (contains(parent))
                return true;
        }
        return false;
    }

    bool check(const ObjectValue *s)
    {
        if (!s)
            return false;
        const ObjectValue *definingObject;
        s->lookupMember(_name, _context, &definingObject);
        return definingObject == _scope;
    }

    bool checkQmlScope()
    {
        foreach (const ObjectValue *s, _context->scopeChain().qmlScopeObjects) {
            if (check(s))
                return true;
        }
        return false;
    }

    bool checkLookup()
    {
        const ObjectValue *scope = 0;
        _context->lookup(_name, &scope);
        return check(scope);
    }

    Result _usages;

    Document::Ptr _doc;
    Context *_context;
    ScopeBuilder _builder;

    QString _name;
    const ObjectValue *_scope;
};

class FindTypeUsages: protected Visitor
{
public:
    typedef QList<AST::SourceLocation> Result;

    FindTypeUsages(Document::Ptr doc, Context *context)
        : _doc(doc)
        , _context(context)
        , _builder(context, doc)
    {
        _builder.initializeRootScope();
    }

    Result operator()(const QString &name, const ObjectValue *typeValue)
    {
        _name = name;
        _typeValue = typeValue;
        _usages.clear();
        if (_doc)
            Node::accept(_doc->ast(), this);
        return _usages;
    }

protected:
    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;

    virtual bool visit(AST::UiPublicMember *node)
    {
        if (node->memberType && node->memberType->asString() == _name){
            const ObjectValue * tVal = _context->lookupType(_doc.data(), QStringList(_name));
            if (tVal == _typeValue)
                _usages.append(node->typeToken);
        }
        if (AST::cast<Block *>(node->statement)) {
            _builder.push(node);
            Node::accept(node->statement, this);
            _builder.pop();
            return false;
        }
        return true;
    }

    virtual bool visit(AST::UiObjectDefinition *node)
    {
        checkTypeName(node->qualifiedTypeNameId);
        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::UiObjectBinding *node)
    {
        checkTypeName(node->qualifiedTypeNameId);
        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::UiScriptBinding *node)
    {
        if (AST::cast<Block *>(node->statement)) {
            Node::accept(node->qualifiedId, this);
            _builder.push(node);
            Node::accept(node->statement, this);
            _builder.pop();
            return false;
        }
        return true;
    }

    virtual bool visit(AST::IdentifierExpression *node)
    {
        if (!node->name || node->name->asString() != _name)
            return false;

        const ObjectValue *scope;
        const Value *objV = _context->lookup(_name, &scope);
        if (objV == _typeValue)
            _usages.append(node->identifierToken);
        return false;
    }

    virtual bool visit(AST::FieldMemberExpression *node)
    {
        if (!node->name || node->name->asString() != _name)
            return true;
        Evaluate evaluate(_context);
        const Value *lhsValue = evaluate(node->base);
        if (!lhsValue)
            return true;
        const ObjectValue *lhsObj = lhsValue->asObjectValue();
        if (lhsObj && lhsObj->lookupMember(_name, _context) == _typeValue)
            _usages.append(node->identifierToken);
        return true;
    }

    virtual bool visit(AST::FunctionDeclaration *node)
    {
        return visit(static_cast<FunctionExpression *>(node));
    }

    virtual bool visit(AST::FunctionExpression *node)
    {
        Node::accept(node->formals, this);
        _builder.push(node);
        Node::accept(node->body, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::VariableDeclaration *node)
    {
        Node::accept(node->expression, this);
        return false;
    }

    virtual bool visit(UiImport *ast)
    {
        if (ast && ast->importId && ast->importId->asString() == _name) {
            const Imports *imp = _context->imports(_doc.data());
            if (!imp)
                return false;
            if (_context->lookupType(_doc.data(), QStringList(_name)) == _typeValue)
                _usages.append(ast->importIdToken);
        }
        return false;
    }


private:
    bool checkTypeName(UiQualifiedId *id)
    {
        for (UiQualifiedId *att = id; att; att = att->next){
            if (att->name && att->name->asString() == _name) {
                const ObjectValue *objectValue = _context->lookupType(_doc.data(), id, att->next);
                if (_typeValue == objectValue){
                    _usages.append(att->identifierToken);
                    return true;
                }
            }
        }
        return false;
    }

    Result _usages;

    Document::Ptr _doc;
    Context *_context;
    ScopeBuilder _builder;

    QString _name;
    const ObjectValue *_typeValue;
};

class FindTargetExpression: protected Visitor
{
public:
    enum Kind {
        ExpKind,
        TypeKind
    };

    FindTargetExpression(Document::Ptr doc, Context *context)
        : _doc(doc), _context(context)
    {
    }

    void operator()(quint32 offset)
    {
        _name.clear();
        _scope = 0;
        _objectNode = 0;
        _offset = offset;
        _typeKind = ExpKind;
        if (_doc)
            Node::accept(_doc->ast(), this);
    }

    QString name() const
    { return _name; }

    const ObjectValue *scope()
    {
        if (!_scope)
            _context->lookup(_name, &_scope);
        return _scope;
    }

    Kind typeKind(){
        return _typeKind;
    }

    const Value *targetValue(){
        return _targetValue;
    }

protected:
    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;

    virtual bool preVisit(Node *node)
    {
        if (Statement *stmt = node->statementCast()) {
            return containsOffset(stmt->firstSourceLocation(), stmt->lastSourceLocation());
        } else if (ExpressionNode *exp = node->expressionCast()) {
            return containsOffset(exp->firstSourceLocation(), exp->lastSourceLocation());
        } else if (UiObjectMember *ui = node->uiObjectMemberCast()) {
            return containsOffset(ui->firstSourceLocation(), ui->lastSourceLocation());
        }
        return true;
    }

    virtual bool visit(IdentifierExpression *node)
    {
        if (containsOffset(node->identifierToken)) {
            _name = node->name->asString();
            if ((!_name.isEmpty()) && _name.at(0).isUpper()) {
                // a possible type
                _targetValue = _context->lookup(_name, &_scope);
                if (value_cast<const ObjectValue*>(_targetValue))
                    _typeKind = TypeKind;
            }
        }
        return true;
    }

    virtual bool visit(FieldMemberExpression *node)
    {
        if (containsOffset(node->identifierToken)) {
            setScope(node->base);
            _name = node->name->asString();
            if ((!_name.isEmpty()) && _name.at(0).isUpper()) {
                // a possible type
                Evaluate evaluate(_context);
                const Value *lhsValue = evaluate(node->base);
                if (!lhsValue)
                    return true;
                const ObjectValue *lhsObj = lhsValue->asObjectValue();
                if (lhsObj) {
                    _scope = lhsObj;
                    _targetValue = lhsObj->lookupMember(_name, _context);
                    _typeKind = TypeKind;
                }
            }
            return false;
        }
        return true;
    }

    virtual bool visit(UiScriptBinding *node)
    {
        return !checkBindingName(node->qualifiedId);
    }

    virtual bool visit(UiArrayBinding *node)
    {
        return !checkBindingName(node->qualifiedId);
    }

    virtual bool visit(UiObjectBinding *node)
    {
        if ((!checkTypeName(node->qualifiedTypeNameId)) &&
                (!checkBindingName(node->qualifiedId))) {
            Node *oldObjectNode = _objectNode;
            _objectNode = node;
            accept(node->initializer);
            _objectNode = oldObjectNode;
        }
        return false;
    }

    virtual bool visit(UiObjectDefinition *node)
    {
        if (!checkTypeName(node->qualifiedTypeNameId)) {
            Node *oldObjectNode = _objectNode;
            _objectNode = node;
            accept(node->initializer);
            _objectNode = oldObjectNode;
        }
        return false;
    }

    virtual bool visit(UiPublicMember *node)
    {
        if (containsOffset(node->typeToken)){
            if (node->memberType){
                _name = node->memberType->asString();
                _targetValue = _context->lookupType(_doc.data(), QStringList(_name));
                _scope = 0;
                _typeKind = TypeKind;
            }
            return false;
        } else if (containsOffset(node->identifierToken)) {
            _scope = _doc->bind()->findQmlObject(_objectNode);
            _name = node->name->asString();
            return false;
        }
        return true;
    }

    virtual bool visit(FunctionDeclaration *node)
    {
        return visit(static_cast<FunctionExpression *>(node));
    }

    virtual bool visit(FunctionExpression *node)
    {
        if (containsOffset(node->identifierToken)) {
            _name = node->name->asString();
            return false;
        }
        return true;
    }

    virtual bool visit(VariableDeclaration *node)
    {
        if (containsOffset(node->identifierToken)) {
            _name = node->name->asString();
            return false;
        }
        return true;
    }

private:
    bool containsOffset(SourceLocation start, SourceLocation end)
    {
        return _offset >= start.begin() && _offset <= end.end();
    }

    bool containsOffset(SourceLocation loc)
    {
        return _offset >= loc.begin() && _offset <= loc.end();
    }

    bool checkBindingName(UiQualifiedId *id)
    {
        if (id && id->name && !id->next && containsOffset(id->identifierToken)) {
            _scope = _doc->bind()->findQmlObject(_objectNode);
            _name = id->name->asString();
            return true;
        }
        return false;
    }

    bool checkTypeName(UiQualifiedId *id)
    {
        for (UiQualifiedId *att = id; att; att = att->next) {
            if (att->name && containsOffset(att->identifierToken)) {
                _targetValue = _context->lookupType(_doc.data(), id, att->next);
                _scope = 0;
                _name = att->name->asString();
                _typeKind = TypeKind;
                return true;
            }
        }
        return false;
    }

    void setScope(Node *node)
    {
        Evaluate evaluate(_context);
        const Value *v = evaluate(node);
        if (v)
            _scope = v->asObjectValue();
    }

    QString _name;
    const ObjectValue *_scope;
    const Value *_targetValue;
    Node *_objectNode;
    Document::Ptr _doc;
    Context *_context;
    quint32 _offset;
    Kind _typeKind;
};

static QString matchingLine(unsigned position, const QString &source)
{
    int start = source.lastIndexOf(QLatin1Char('\n'), position);
    start += 1;
    int end = source.indexOf(QLatin1Char('\n'), position);

    return source.mid(start, end - start);
}

class ProcessFile: public std::unary_function<QString, QList<FindReferences::Usage> >
{
    const Context &context;
    typedef FindReferences::Usage Usage;
    QString name;
    const ObjectValue *scope;

public:
    ProcessFile(const Context &context,
                QString name,
                const ObjectValue *scope)
        : context(context), name(name), scope(scope)
    { }

    QList<Usage> operator()(const QString &fileName)
    {
        QList<Usage> usages;

        Document::Ptr doc = context.snapshot().document(fileName);
        if (!doc)
            return usages;

        Context contextCopy(context);

        // find all idenfifier expressions, try to resolve them and check if the result is in scope
        FindUsages findUsages(doc, &contextCopy);
        FindUsages::Result results = findUsages(name, scope);
        foreach (const AST::SourceLocation &loc, results)
            usages.append(Usage(fileName, matchingLine(loc.offset, doc->source()), loc.startLine, loc.startColumn - 1, loc.length));

        return usages;
    }
};

class SearchFileForType: public std::unary_function<QString, QList<FindReferences::Usage> >
{
    const Context &context;
    typedef FindReferences::Usage Usage;
    QString name;
    const ObjectValue *scope;

public:
    SearchFileForType(const Context &context,
                QString name,
                const ObjectValue *scope)
        : context(context), name(name), scope(scope)
    { }

    QList<Usage> operator()(const QString &fileName)
    {
        QList<Usage> usages;

        Document::Ptr doc = context.snapshot().document(fileName);
        if (!doc)
            return usages;

        Context contextCopy(context);

        // find all idenfifier expressions, try to resolve them and check if the result is in scope
        FindTypeUsages findUsages(doc, &contextCopy);
        FindTypeUsages::Result results = findUsages(name, scope);
        foreach (const AST::SourceLocation &loc, results)
            usages.append(Usage(fileName, matchingLine(loc.offset, doc->source()), loc.startLine, loc.startColumn - 1, loc.length));

        return usages;
    }
};

class UpdateUI: public std::binary_function<QList<FindReferences::Usage> &, QList<FindReferences::Usage>, void>
{
    typedef FindReferences::Usage Usage;
    QFutureInterface<Usage> *future;

public:
    UpdateUI(QFutureInterface<Usage> *future): future(future) {}

    void operator()(QList<Usage> &, const QList<Usage> &usages)
    {
        foreach (const Usage &u, usages)
            future->reportResult(u);

        future->setProgressValue(future->progressValue() + 1);
    }
};

} // end of anonymous namespace

FindReferences::FindReferences(QObject *parent)
    : QObject(parent)
    , _resultWindow(Find::SearchResultWindow::instance())
{
    m_watcher.setPendingResultsLimit(1);
    connect(&m_watcher, SIGNAL(resultsReadyAt(int,int)), this, SLOT(displayResults(int,int)));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
}

FindReferences::~FindReferences()
{
}

static void find_helper(QFutureInterface<FindReferences::Usage> &future,
                        const ModelManagerInterface::WorkingCopy workingCopy,
                        Snapshot snapshot,
                        const QString fileName,
                        quint32 offset)
{
    // update snapshot from workingCopy to make sure it's up to date
    // ### remove?
    // ### this is a great candidate for map-reduce
    QHashIterator< QString, QPair<QString, int> > it(workingCopy.all());
    while (it.hasNext()) {
        it.next();
        Document::Ptr oldDoc = snapshot.document(it.key());
        if (oldDoc && oldDoc->editorRevision() == it.value().second)
            continue;

        Document::Ptr newDoc = snapshot.documentFromSource(it.key(), it.value().first);
        newDoc->parse();
        snapshot.insert(newDoc);
    }

    // find the scope for the name we're searching
    Context context(snapshot);
    Document::Ptr doc = snapshot.document(fileName);
    if (!doc)
        return;

    Link link(&context, snapshot, ModelManagerInterface::instance()->importPaths());
    link();

    ScopeBuilder builder(&context, doc);
    builder.initializeRootScope();
    ScopeAstPath astPath(doc);
    builder.push(astPath(offset));

    FindTargetExpression findTarget(doc, &context);
    findTarget(offset);
    const QString &name = findTarget.name();
    if (name.isEmpty())
        return;

    QStringList files;
    foreach (const Document::Ptr &doc, snapshot) {
        // ### skip files that don't contain the name token
        files.append(doc->fileName());
    }

    future.setProgressRange(0, files.size());

    if (findTarget.typeKind() == findTarget.TypeKind){
        const ObjectValue *typeValue = value_cast<const ObjectValue*>(findTarget.targetValue());
        if (!typeValue)
            return;
        // report a dummy usage to indicate the search is starting
        future.reportResult(FindReferences::Usage());

        SearchFileForType process(context, name, typeValue);
        UpdateUI reduce(&future);

        QtConcurrent::blockingMappedReduced<QList<FindReferences::Usage> > (files, process, reduce);
    } else {
        const ObjectValue *scope = findTarget.scope();
        if (!scope)
            return;
        scope->lookupMember(name, &context, &scope);
        if (!scope)
            return;
        // report a dummy usage to indicate the search is starting
         future.reportResult(FindReferences::Usage());

        ProcessFile process(context, name, scope);
        UpdateUI reduce(&future);

        QtConcurrent::blockingMappedReduced<QList<FindReferences::Usage> > (files, process, reduce);
    }
    future.setProgressValue(files.size());
}

void FindReferences::findUsages(const QString &fileName, quint32 offset)
{
    findAll_helper(fileName, offset);
}

void FindReferences::findAll_helper(const QString &fileName, quint32 offset)
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();


    QFuture<Usage> result = QtConcurrent::run(
                &find_helper, modelManager->workingCopy(),
                modelManager->snapshot(), fileName, offset);
    m_watcher.setFuture(result);
}

void FindReferences::displayResults(int first, int last)
{
    // the first usage is always a dummy to indicate we now start searching
    if (first == 0) {
        Find::SearchResult *search = _resultWindow->startNewSearch(Find::SearchResultWindow::SearchOnly);
        connect(search, SIGNAL(activated(Find::SearchResultItem)),
                this, SLOT(openEditor(Find::SearchResultItem)));
        _resultWindow->popup(true);

        Core::ProgressManager *progressManager = Core::ICore::instance()->progressManager();
        Core::FutureProgress *progress = progressManager->addTask(
                    m_watcher.future(), tr("Searching"),
                    QmlJSEditor::Constants::TASK_SEARCH);
        connect(progress, SIGNAL(clicked()), _resultWindow, SLOT(popup()));

        ++first;
    }

    for (int index = first; index != last; ++index) {
        Usage result = m_watcher.future().resultAt(index);
        _resultWindow->addResult(result.path,
                                 result.line,
                                 result.lineText,
                                 result.col,
                                 result.len);
    }
}

void FindReferences::searchFinished()
{
    _resultWindow->finishSearch();
    emit changed();
}

void FindReferences::openEditor(const Find::SearchResultItem &item)
{
    if (item.path.size() > 0) {
        TextEditor::BaseTextEditorWidget::openEditorAt(item.path.first(), item.lineNumber, item.textMarkPos,
                                                 QString(),
                                                 Core::EditorManager::ModeSwitch);
    } else {
        Core::EditorManager::instance()->openEditor(item.text, QString(), Core::EditorManager::ModeSwitch);
    }
}
