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

#include "qmljsscopebuilder.h"

#include "qmljsbind.h"
#include "qmljsinterpreter.h"
#include "qmljsevaluate.h"
#include "parser/qmljsast_p.h"

using namespace QmlJS;
using namespace QmlJS::Interpreter;
using namespace QmlJS::AST;

ScopeBuilder::ScopeBuilder(Context *context, Document::Ptr doc)
    : _doc(doc)
    , _context(context)
{
}

ScopeBuilder::~ScopeBuilder()
{
}

void ScopeBuilder::push(AST::Node *node)
{
    _nodes += node;

    // QML scope object
    Node *qmlObject = cast<UiObjectDefinition *>(node);
    if (! qmlObject)
        qmlObject = cast<UiObjectBinding *>(node);
    if (qmlObject)
        setQmlScopeObject(qmlObject);

    // JS scopes
    switch (node->kind) {
    case Node::Kind_UiScriptBinding:
    case Node::Kind_FunctionDeclaration:
    case Node::Kind_FunctionExpression:
    case Node::Kind_UiPublicMember:
    {
        ObjectValue *scope = _doc->bind()->findAttachedJSScope(node);
        if (scope)
            _context->scopeChain().jsScopes += scope;
        break;
    }
    default:
        break;
    }

    _context->scopeChain().update();
}

void ScopeBuilder::push(const QList<AST::Node *> &nodes)
{
    foreach (Node *node, nodes)
        push(node);
}

void ScopeBuilder::pop()
{
    Node *toRemove = _nodes.last();
    _nodes.removeLast();

    // JS scopes
    switch (toRemove->kind) {
    case Node::Kind_UiScriptBinding:
    case Node::Kind_FunctionDeclaration:
    case Node::Kind_FunctionExpression:
    case Node::Kind_UiPublicMember:
    {
        ObjectValue *scope = _doc->bind()->findAttachedJSScope(toRemove);
        if (scope)
            _context->scopeChain().jsScopes.removeLast();
        break;
    }
    default:
        break;
    }

    // QML scope object
    if (! _nodes.isEmpty()
        && (cast<UiObjectDefinition *>(toRemove) || cast<UiObjectBinding *>(toRemove)))
        setQmlScopeObject(_nodes.last());

    _context->scopeChain().update();
}

void ScopeBuilder::initializeRootScope()
{
    ScopeChain &scopeChain = _context->scopeChain();
    if (scopeChain.qmlComponentScope
            && scopeChain.qmlComponentScope->document == _doc) {
        return;
    }

    scopeChain = ScopeChain(); // reset

    Interpreter::Engine *engine = _context->engine();

    // ### TODO: This object ought to contain the global namespace additions by QML.
    scopeChain.globalScope = engine->globalObject();

    if (! _doc) {
        scopeChain.update();
        return;
    }

    Bind *bind = _doc->bind();
    QHash<Document *, ScopeChain::QmlComponentChain *> componentScopes;
    const Snapshot &snapshot = _context->snapshot();

    ScopeChain::QmlComponentChain *chain = new ScopeChain::QmlComponentChain;
    scopeChain.qmlComponentScope = QSharedPointer<const ScopeChain::QmlComponentChain>(chain);
    if (_doc->qmlProgram()) {
        componentScopes.insert(_doc.data(), chain);
        makeComponentChain(_doc, snapshot, chain, &componentScopes);

        if (const Imports *imports = _context->imports(_doc.data())) {
            scopeChain.qmlTypes = imports->typeScope();
            scopeChain.jsImports = imports->jsImportScope();
        }
    } else {
        // add scope chains for all components that import this file
        foreach (Document::Ptr otherDoc, snapshot) {
            foreach (const ImportInfo &import, otherDoc->bind()->imports()) {
                if (import.type() == ImportInfo::FileImport && _doc->fileName() == import.name()) {
                    ScopeChain::QmlComponentChain *component = new ScopeChain::QmlComponentChain;
                    componentScopes.insert(otherDoc.data(), component);
                    chain->instantiatingComponents += component;
                    makeComponentChain(otherDoc, snapshot, component, &componentScopes);
                }
            }
        }

        // ### TODO: Which type environment do scripts see?

        if (bind->rootObjectValue())
            scopeChain.jsScopes += bind->rootObjectValue();
    }

    scopeChain.update();
}

void ScopeBuilder::makeComponentChain(
        Document::Ptr doc,
        const Snapshot &snapshot,
        ScopeChain::QmlComponentChain *target,
        QHash<Document *, ScopeChain::QmlComponentChain *> *components)
{
    if (!doc->qmlProgram())
        return;

    Bind *bind = doc->bind();

    // add scopes for all components instantiating this one
    foreach (Document::Ptr otherDoc, snapshot) {
        if (otherDoc == doc)
            continue;
        if (otherDoc->bind()->usesQmlPrototype(bind->rootObjectValue(), _context)) {
            if (components->contains(otherDoc.data())) {
//                target->instantiatingComponents += components->value(otherDoc.data());
            } else {
                ScopeChain::QmlComponentChain *component = new ScopeChain::QmlComponentChain;
                components->insert(otherDoc.data(), component);
                target->instantiatingComponents += component;

                makeComponentChain(otherDoc, snapshot, component, components);
            }
        }
    }

    // build this component scope
    target->document = doc;
}

void ScopeBuilder::setQmlScopeObject(Node *node)
{
    ScopeChain &scopeChain = _context->scopeChain();

    if (_doc->bind()->isGroupedPropertyBinding(node)) {
        UiObjectDefinition *definition = cast<UiObjectDefinition *>(node);
        if (!definition)
            return;
        const Value *v = scopeObjectLookup(definition->qualifiedTypeNameId);
        if (!v)
            return;
        const ObjectValue *object = v->asObjectValue();
        if (!object)
            return;

        scopeChain.qmlScopeObjects.clear();
        scopeChain.qmlScopeObjects += object;
    }

    const ObjectValue *scopeObject = _doc->bind()->findQmlObject(node);
    if (scopeObject) {
        scopeChain.qmlScopeObjects.clear();
        scopeChain.qmlScopeObjects += scopeObject;
    } else {
        return; // Probably syntax errors, where we're working with a "recovered" AST.
    }

    // check if the object has a Qt.ListElement or Qt.Connections ancestor
    // ### allow only signal bindings for Connections
    PrototypeIterator iter(scopeObject, _context);
    iter.next();
    while (iter.hasNext()) {
        const ObjectValue *prototype = iter.next();
        if (const QmlObjectValue *qmlMetaObject = dynamic_cast<const QmlObjectValue *>(prototype)) {
            if ((qmlMetaObject->className() == QLatin1String("ListElement")
                    || qmlMetaObject->className() == QLatin1String("Connections")
                    ) && (qmlMetaObject->packageName() == QLatin1String("Qt")
                          || qmlMetaObject->packageName() == QLatin1String("QtQuick"))) {
                scopeChain.qmlScopeObjects.clear();
                break;
            }
        }
    }

    // check if the object has a Qt.PropertyChanges ancestor
    const ObjectValue *prototype = scopeObject->prototype(_context);
    prototype = isPropertyChangesObject(_context, prototype);
    // find the target script binding
    if (prototype) {
        UiObjectInitializer *initializer = 0;
        if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(node))
            initializer = definition->initializer;
        if (UiObjectBinding *binding = cast<UiObjectBinding *>(node))
            initializer = binding->initializer;
        if (initializer) {
            for (UiObjectMemberList *m = initializer->members; m; m = m->next) {
                if (UiScriptBinding *scriptBinding = cast<UiScriptBinding *>(m->member)) {
                    if (scriptBinding->qualifiedId && scriptBinding->qualifiedId->name
                            && scriptBinding->qualifiedId->name->asString() == QLatin1String("target")
                            && ! scriptBinding->qualifiedId->next) {
                        Evaluate evaluator(_context);
                        const Value *targetValue = evaluator(scriptBinding->statement);

                        if (const ObjectValue *target = value_cast<const ObjectValue *>(targetValue)) {
                            scopeChain.qmlScopeObjects.prepend(target);
                        } else {
                            scopeChain.qmlScopeObjects.clear();
                        }
                    }
                }
            }
        }
    }
}

const Value *ScopeBuilder::scopeObjectLookup(AST::UiQualifiedId *id)
{
    // do a name lookup on the scope objects
    const Value *result = 0;
    foreach (const ObjectValue *scopeObject, _context->scopeChain().qmlScopeObjects) {
        const ObjectValue *object = scopeObject;
        for (UiQualifiedId *it = id; it; it = it->next) {
            if (!it->name)
                return 0;
            result = object->lookupMember(it->name->asString(), _context);
            if (!result)
                break;
            if (it->next) {
                object = result->asObjectValue();
                if (!object) {
                    result = 0;
                    break;
                }
            }
        }
        if (result)
            break;
    }

    return result;
}


const ObjectValue *ScopeBuilder::isPropertyChangesObject(const Context *context,
                                                   const ObjectValue *object)
{
    PrototypeIterator iter(object, context);
    while (iter.hasNext()) {
        const ObjectValue *prototype = iter.next();
        if (const QmlObjectValue *qmlMetaObject = dynamic_cast<const QmlObjectValue *>(prototype)) {
            if (qmlMetaObject->className() == QLatin1String("PropertyChanges")
                    && (qmlMetaObject->packageName() == QLatin1String("Qt")
                        || qmlMetaObject->packageName() == QLatin1String("QtQuick")))
                return prototype;
        }
    }
    return 0;
}
