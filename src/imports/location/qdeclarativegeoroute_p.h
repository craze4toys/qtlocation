/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEGEOROUTE_H
#define QDECLARATIVEGEOROUTE_H

#include "qdeclarativegeoboundingbox_p.h"
#include "qdeclarativecoordinate_p.h"
#include "qdeclarativegeoroutesegment_p.h"

#include <qgeoroute.h>

#include <QObject>

QT_BEGIN_NAMESPACE

class QDeclarativeGeoRoute : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDeclarativeGeoBoundingBox *bounds READ bounds CONSTANT)
    Q_PROPERTY(int travelTime READ travelTime CONSTANT)
    Q_PROPERTY(qreal distance READ distance CONSTANT)
    Q_PROPERTY(QQmlListProperty<QDeclarativeCoordinate> path READ path CONSTANT)
    Q_PROPERTY(QQmlListProperty<QDeclarativeGeoRouteSegment> segments READ segments CONSTANT)

public:
    explicit QDeclarativeGeoRoute(QObject *parent = 0);
    QDeclarativeGeoRoute(const QGeoRoute &route, QObject *parent = 0);
    ~QDeclarativeGeoRoute();

    QDeclarativeGeoBoundingBox *bounds() const;
    int travelTime() const;
    qreal distance() const;
    QQmlListProperty<QDeclarativeCoordinate> path();
    QQmlListProperty<QDeclarativeGeoRouteSegment> segments();

    void appendPath(QDeclarativeCoordinate *coordinate);
    void clearPath();

    void appendSegment(QDeclarativeGeoRouteSegment *segment);
    void clearSegments();

private:
    static void path_append(QQmlListProperty<QDeclarativeCoordinate> *prop, QDeclarativeCoordinate *coordinate);
    static int path_count(QQmlListProperty<QDeclarativeCoordinate> *prop);
    static QDeclarativeCoordinate *path_at(QQmlListProperty<QDeclarativeCoordinate> *prop, int index);
    static void path_clear(QQmlListProperty<QDeclarativeCoordinate> *prop);

    static void segments_append(QQmlListProperty<QDeclarativeGeoRouteSegment> *prop, QDeclarativeGeoRouteSegment *segment);
    static int segments_count(QQmlListProperty<QDeclarativeGeoRouteSegment> *prop);
    static QDeclarativeGeoRouteSegment *segments_at(QQmlListProperty<QDeclarativeGeoRouteSegment> *prop, int index);
    static void segments_clear(QQmlListProperty<QDeclarativeGeoRouteSegment> *prop);

    void init();
    QList<QGeoCoordinate> routePath();

    QGeoRoute route_;
    QDeclarativeGeoBoundingBox *bounds_;
    QList<QDeclarativeCoordinate *> path_;
    QList<QDeclarativeGeoRouteSegment *> segments_;
    friend class QDeclarativeRouteMapItem;
};

QT_END_NAMESPACE

#endif
