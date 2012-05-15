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

#ifndef QGEOROUTINGMANAGERENGINE_H
#define QGEOROUTINGMANAGERENGINE_H

#include "qgeorouterequest.h"
#include "qgeoroutereply.h"

#include <QObject>
#include <QMap>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE



class QLocale;

class QGeoRoutingManagerEnginePrivate;

class Q_LOCATION_EXPORT QGeoRoutingManagerEngine : public QObject
{
    Q_OBJECT
public:
    QGeoRoutingManagerEngine(const QMap<QString, QVariant> &parameters, QObject *parent = 0);
    virtual ~QGeoRoutingManagerEngine();

    QString managerName() const;
    int managerVersion() const;

    virtual QGeoRouteReply *calculateRoute(const QGeoRouteRequest &request) = 0;
    virtual QGeoRouteReply *updateRoute(const QGeoRoute &route, const QGeoCoordinate &position);

    QGeoRouteRequest::TravelModes supportedTravelModes() const;
    QGeoRouteRequest::FeatureTypes supportedFeatureTypes() const;
    QGeoRouteRequest::FeatureWeights supportedFeatureWeights() const;
    QGeoRouteRequest::RouteOptimizations supportedRouteOptimizations() const;
    QGeoRouteRequest::SegmentDetails supportedSegmentDetails() const;
    QGeoRouteRequest::ManeuverDetails supportedManeuverDetails() const;

    void setLocale(const QLocale &locale);
    QLocale locale() const;

Q_SIGNALS:
    void finished(QGeoRouteReply *reply);
    void error(QGeoRouteReply *reply, QGeoRouteReply::Error error, QString errorString = QString());

protected:
    void setSupportedTravelModes(QGeoRouteRequest::TravelModes travelModes);
    void setSupportedFeatureTypes(QGeoRouteRequest::FeatureTypes featureTypes);
    void setSupportedFeatureWeights(QGeoRouteRequest::FeatureWeights featureWeights);
    void setSupportedRouteOptimizations(QGeoRouteRequest::RouteOptimizations optimizations);
    void setSupportedSegmentDetails(QGeoRouteRequest::SegmentDetails segmentDetails);
    void setSupportedManeuverDetails(QGeoRouteRequest::ManeuverDetails maneuverDetails);

private:
    void setManagerName(const QString &managerName);
    void setManagerVersion(int managerVersion);

    QGeoRoutingManagerEnginePrivate *d_ptr;
    Q_DISABLE_COPY(QGeoRoutingManagerEngine)

    friend class QGeoServiceProvider;
    friend class QGeoServiceProviderPrivate;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
