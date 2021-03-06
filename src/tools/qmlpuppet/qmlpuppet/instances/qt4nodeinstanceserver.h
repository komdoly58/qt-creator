/**************************************************************************

**

**  This  file  is  part  of  Qt  Creator

**

**  Copyright  (c)  2011  Nokia  Corporation  and/or  its  subsidiary(-ies).

**

**  Contact:  Nokia  Corporation  (qt-info@nokia.com)

**

**  No  Commercial  Usage

**

**  This  file  contains  pre-release  code  and  may  not  be  distributed.

**  You  may  use  this  file  in  accordance  with  the  terms  and  conditions

**  contained  in  the  Technology  Preview  License  Agreement  accompanying

**  this  package.

**

**  GNU  Lesser  General  Public  License  Usage

**

**  Alternatively,  this  file  may  be  used  under  the  terms  of  the  GNU  Lesser

**  General  Public  License  version  2.1  as  published  by  the  Free  Software

**  Foundation  and  appearing  in  the  file  LICENSE.LGPL  included  in  the

**  packaging  of  this  file.   Please  review  the  following  information  to

**  ensure  the  GNU  Lesser  General  Public  License  version  2.1  requirements

**  will  be  met:  http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.

**

**  In  addition,  as  a  special  exception,  Nokia  gives  you  certain  additional

**  rights.   These  rights  are  described  in  the  Nokia  Qt  LGPL  Exception

**  version  1.1,  included  in  the  file  LGPL_EXCEPTION.txt  in  this  package.

**

**  If  you  have  questions  regarding  the  use  of  this  file,  please  contact

**  Nokia  at  qt-info@nokia.com.

**

**************************************************************************/

#ifndef QT4NODEINSTANCESERVER_H
#define QT4NODEINSTANCESERVER_H

#include "nodeinstanceserver.h"

QT_BEGIN_NAMESPACE
class DesignerSupport;
QT_END_NAMESPACE

namespace QmlDesigner {

class Qt4NodeInstanceServer : public NodeInstanceServer
{
    Q_OBJECT
public:
    Qt4NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);
    ~Qt4NodeInstanceServer();

    QSGView *sgView() const;
    QDeclarativeView *declarativeView() const;
    QDeclarativeEngine *engine() const;
    void refreshBindings();

protected:
    void initializeView(const QVector<AddImportContainer> &importVector);
    void resizeCanvasSizeToRootItemSize();
    void resetAllItems();
    bool nonInstanceChildIsDirty(QGraphicsObject *graphicsObject) const;
    QList<ServerNodeInstance> setupScene(const CreateSceneCommand &command);

private:
    QWeakPointer<QDeclarativeView> m_declarativeView;
};

} // QmlDesigner
#endif // QT4NODEINSTANCESERVER_H
