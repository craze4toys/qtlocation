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

#include "qdeclarativegeoroutemodel_p.h"
#include "qdeclarativegeoroute_p.h"

#include <qgeoserviceprovider.h>
#include <qgeoroutingmanager.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass RouteModel QDeclarativeGeoRouteModel
    \inqmlmodule QtLocation 5
    \ingroup qml-QtLocation5-routing
    \since Qt Location 5.0

    \brief The RouteModel element provides access to routes.

    The RouteModel element is used as part of a model/view grouping to retrieve
    geographic routes from a backend provider. Routes include data about driving
    directions between two points, walking directions with multiple waypoints,
    and various other similar concepts. It functions much like other Model
    elements in QML (see for example \l ListModel and \l XmlListModel), and interacts
    with views such as \l MapItemView, and \l{ListView}.

    Like \l Map and \l GeocodeModel, all the data for a RouteModel to work comes
    from a services plugin. This is contained in the \l{plugin} property, and
    this must be set before the RouteModel can do any useful work.

    Once the plugin is set, create a \l RouteQuery with the appropriate
    waypoints and other settings, and set the \l{RouteModel}'s \l{query}
    property. If \l autoUpdate is enabled, the update will being automatically.
    Otherwise, the \l{update} method may be used. By default, autoUpdate is
    disabled.

    The data stored and returned in the RouteModel consists of \l Route elements,
    as a list with the role name "routeData". See the documentation for \l Route
    for further details on its structure and contents.

    \section2 Example Usage

    The following snippet is two-part, showing firstly the declaration of
    elements, and secondly a short piece of procedural code using it. We set
    the routeModel's \l{autoUpdate} property to false, and call \l{update} once
    the query is set up, to avoid useless extra requests halfway through the
    set up of the query.

    \code
    Plugin {
        id: aPlugin
    }

    RouteQuery {
        id: aQuery
    }

    RouteModel {
        id: routeModel
        plugin: aPlugin
        query: aQuery
        autoUpdate: false
    }
    \endcode

    \code
    {
        aQuery.addWaypoint(...)
        aQuery.addWaypoint(...)
        aQuery.travelModes = ...
        routeModel.update()
    }
    \endcode

*/

QDeclarativeGeoRouteModel::QDeclarativeGeoRouteModel(QObject *parent)
    : QAbstractListModel(parent),
      complete_(false),
      plugin_(0),
      routeQuery_(0),
      reply_(0),
      autoUpdate_(false),
      status_(QDeclarativeGeoRouteModel::Null),
      error_(QDeclarativeGeoRouteModel::NoError)
{
    // Establish role names so that they can be queried from this model
    QHash<int, QByteArray> roleNames;
    roleNames = QAbstractItemModel::roleNames();
    roleNames.insert(RouteRole, "routeData");
    setRoleNames(roleNames);
}

QDeclarativeGeoRouteModel::~QDeclarativeGeoRouteModel()
{
    if (!routes_.empty()) {
        qDeleteAll(routes_);
        routes_.clear();
    }
    if (reply_)
        delete reply_;
}

/*!
    \qmlproperty int QtLocation5::RouteModel::count

    This property holds how many routes the model currently has.
    Amongst other uses, you can use this value when accessing routes
    via the QtLocation5::RouteModel::get -method.
*/

int QDeclarativeGeoRouteModel::count() const
{
    return routes_.count();
}

/*!
    \qmlmethod QtLocation5::RouteModel::reset()

    Resets the model. All route data is cleared, any outstanding requests
    are aborted and possible errors are cleared. Model status will be set
    to RouteModel.Null
*/

void QDeclarativeGeoRouteModel::reset()
{
    if (!routes_.isEmpty()) {
        beginResetModel();
        qDeleteAll(routes_);
        routes_.clear();
        emit countChanged();
        emit routesChanged();
        endResetModel();
    }

    abortRequest();
    setErrorString(QString());
    setError(NoError);
    setStatus(QDeclarativeGeoRouteModel::Null);
}

/*!
    \qmlmethod QtLocation5::RouteModel::cancel()

    Cancels any outstanding requests and clears errors.  Model status will be set to either
    RouteModel.Null or RouteModel.Ready.
*/
void QDeclarativeGeoRouteModel::cancel()
{
    abortRequest();
    setErrorString(QString());
    setError(NoError);
    setStatus(routes_.isEmpty() ? Null : Ready);
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::abortRequest()
{
    if (reply_) {
        reply_->abort();
        reply_->deleteLater();
        reply_ = 0;
    }
}


/*!
    \qmlmethod QtLocation5::RouteModel::get(int)

    Returns the Route at given index. Use \l count property to check the
    amount of routes available. The routes are indexed from zero, so the accessible range
    is 0...(count - 1).

    If you access out of bounds, a zero (null object) is returned and a warning is issued.
*/

QDeclarativeGeoRoute* QDeclarativeGeoRouteModel::get(int index)
{
    if (index < 0 || index >= routes_.count()) {
        qmlInfo(this) << tr("Error, invalid index for get(): ") << index;
        return 0;
    }
    return routes_.at(index);
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::componentComplete()
{
    complete_ = true;
    if (autoUpdate_) {
        update();
    }
}

/*!
    \internal
*/
int QDeclarativeGeoRouteModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return routes_.count();
}

/*!
    \internal
*/
QVariant QDeclarativeGeoRouteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qmlInfo(this) << tr("Error in indexing route model's data (invalid index).");
        return QVariant();
    }

    if (index.row() >= routes_.count()) {
        qmlInfo(this) << tr("Fatal error in indexing route model's data (index overflow).");
        return QVariant();
    }

    if (role == RouteRole) {
        QObject* route = routes_.at(index.row());
        return QVariant::fromValue(route);
    }
    return QVariant();
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::setPlugin(QDeclarativeGeoServiceProvider *plugin)
{
    if (plugin_ == plugin)
        return;

    reset(); // reset the model
    plugin_ = plugin;
    if (complete_)
        emit pluginChanged();

    if (!plugin)
        return;

    if (plugin_->isAttached()) {
        pluginReady();
    } else {
        connect(plugin_, SIGNAL(attached()),
                this, SLOT(pluginReady()));
    }
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::pluginReady()
{
    QGeoServiceProvider *serviceProvider = plugin_->sharedGeoServiceProvider();
    QGeoRoutingManager *routingManager = serviceProvider->routingManager();
    if (!routingManager || serviceProvider->error() != QGeoServiceProvider::NoError) {
        qmlInfo(this) << tr("Warning: Plugin does not support routing. Error message: %1")
                         .arg(serviceProvider->errorString());
        return;
    }
    connect(routingManager, SIGNAL(finished(QGeoRouteReply*)),
            this, SLOT(routingFinished(QGeoRouteReply*)));
    connect(routingManager, SIGNAL(error(QGeoRouteReply*,QGeoRouteReply::Error,QString)),
            this, SLOT(routingError(QGeoRouteReply*,QGeoRouteReply::Error,QString)));
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::queryDetailsChanged()
{
    if (autoUpdate_ && complete_)
        update();
}

/*!
    \qmlproperty Plugin QtLocation5::RouteModel::plugin

    This property holds the plugin that providers the actual
    routing service. Note that all plugins do not necessarily
    provide routing (could for example provide only geocoding or maps).

    A valid plugin must be set before the RouteModel can perform any useful
    operations.

    \sa Plugin
*/

QDeclarativeGeoServiceProvider* QDeclarativeGeoRouteModel::plugin() const
{
    return plugin_;
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::setQuery(QDeclarativeGeoRouteQuery* query)
{
    if (!query || query == routeQuery_)
        return;
    if (routeQuery_)
        routeQuery_->disconnect(this);
    routeQuery_ = query;
    connect(query, SIGNAL(queryDetailsChanged()), this, SLOT(queryDetailsChanged()));
    if (complete_) {
        emit queryChanged();
        if (autoUpdate_)
            update();
    }
}

/*!
    \qmlproperty RouteQuery QtLocation5::RouteModel::query

    This property holds the data of the route request.
    The primary data are the waypoint coordinates and possible further
    preferences (means of traveling, things to avoid on route etc).
*/

QDeclarativeGeoRouteQuery* QDeclarativeGeoRouteModel::query() const
{
    return routeQuery_;
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::setAutoUpdate(bool autoUpdate)
{
    if (autoUpdate_ == autoUpdate)
        return;
    autoUpdate_ = autoUpdate;
    if (complete_)
        emit autoUpdateChanged();
}

/*!
    \qmlproperty bool QtLocation5::RouteModel::autoUpdate

    This property controls whether the Model automatically updates in response
    to changes in its attached RouteQuery. The default value of this property
    is false.

    If setting this value to 'true', note that any change at all in
    the RouteQuery object set in the \l{query} property will trigger a new
    request to be sent. If you are adjusting many properties of the RouteQuery
    with autoUpdate enabled, this can generate large numbers of useless (and
    later discarded) requests.
*/

bool QDeclarativeGeoRouteModel::autoUpdate() const
{
    return autoUpdate_;
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::setStatus(QDeclarativeGeoRouteModel::Status status)
{
    if (status_ == status)
        return;

    status_ = status;

    if (complete_)
        emit statusChanged();
}

/*!
    \qmlproperty enumeration QtLocation5::RouteModel::status

    This read-only property holds the current status of the model.

    \list
    \li RouteModel.Null - No route requests have been issued or \l reset has been called.
    \li RouteModel.Ready - Route request(s) have finished successfully.
    \li RouteModel.Loading - Route request has been issued but not yet finished
    \li RouteModel.Error - Routing error has occurred, details are in \l error and \l errorString
    \endlist
*/

QDeclarativeGeoRouteModel::Status QDeclarativeGeoRouteModel::status() const
{
    return status_;
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::setErrorString(const QString &error)
{
    if (errorString_ == error)
        return;

    errorString_ = error;

    if (complete_)
        emit errorStringChanged();
}

/*!
    \qmlproperty string QtLocation5::RouteModel::errorString

    This read-only property holds the textual presentation of latest routing error.
    If no error has occurred or the model has been reset, an empty string is returned.

    An empty string may also be returned if an error occurred which has no associated
    textual representation.
*/

QString QDeclarativeGeoRouteModel::errorString() const
{
    return errorString_;
}

/*!
    \qmlproperty enumeration QtLocation5::RouteModel::error

    This read-only property holds the latest error value of the routing request.

    \list
    \li RouteModel.NoError - No error has occurred
    \li RouteModel.EngineNotSetError - The plugin/service provider used does not support routing
    \li RouteModel.CommunicationError - An error occurred while communicating with the service provider
    \li RouteModel.ParseError - The response from the service provider was in an unrecognizable format
    \li RouteModel.UnsupportedOptionError - The requested operation or one of the options for the operation are not supported by the service provider.
    \li RouteModel.UnknownError - An error occurred which does not fit into any of the other categories
    \endlist
*/

QDeclarativeGeoRouteModel::RouteError QDeclarativeGeoRouteModel::error() const
{
    return error_;
}

void QDeclarativeGeoRouteModel::setError(RouteError error)
{
    if (error_ == error)
        return;
    error_ = error;
    emit errorChanged();
}

/*!
    \qmlmethod QtLocation5::RouteModel::update()

    Instructs the RouteModel to update its data. This is most useful
    when \l autoUpdate is disabled, to force a refresh when the query
    has been changed.
*/
void QDeclarativeGeoRouteModel::update()
{
    if (!complete_)
        return;

    if (!plugin_) {
        qmlInfo(this) << tr("Plugin not set, cannot route.");
        return;
    }

    QGeoServiceProvider *serviceProvider = plugin_->sharedGeoServiceProvider();
    if (!serviceProvider)
        return;

    QGeoRoutingManager *routingManager = serviceProvider->routingManager();
    if (!routingManager) {
        qmlInfo(this) << tr("No routing manager available, cannot route.");
        return;
    }
    if (!routeQuery_) {
        qmlInfo(this) << tr("Route query not set, cannot route.");
        return;
    }
    abortRequest(); // Clear previus requests
    QGeoRouteRequest request = routeQuery_->routeRequest();
    if (request.waypoints().count() < 2) {
        qmlInfo(this) << tr("Not enough waypoints for routing.");
        return;
    }

    setErrorString("");   // clear previous error string
    setError(NoError);

    reply_ = routingManager->calculateRoute(request);
    setStatus(QDeclarativeGeoRouteModel::Loading);
    if (reply_->isFinished()) {
        if (reply_->error() == QGeoRouteReply::NoError) {
            routingFinished(reply_);
        } else {
            routingError(reply_, reply_->error(), reply_->errorString());
        }
    }
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::routingFinished(QGeoRouteReply *reply)
{
    if (reply->error() != QGeoRouteReply::NoError) {
        return;
    }
    beginResetModel();
    int oldCount = routes_.count();
    qDeleteAll(routes_);
    // Convert routes to declarative
    routes_.clear();
    for (int i = 0; i < reply->routes().size(); ++i)
        routes_.append(new QDeclarativeGeoRoute(reply->routes().at(i), this));
    endResetModel();

    setErrorString("");
    setError(NoError);
    setStatus(QDeclarativeGeoRouteModel::Ready);

    reply->deleteLater();
    reply_ = 0;

    if (oldCount != 0 || routes_.count() != 0)
        emit routesChanged();
    if (oldCount != routes_.count())
        emit countChanged();
}

/*!
    \internal
*/
void QDeclarativeGeoRouteModel::routingError(QGeoRouteReply *reply,
                                               QGeoRouteReply::Error error,
                                               const QString &errorString)
{
    setErrorString(errorString);
    setError(static_cast<QDeclarativeGeoRouteModel::RouteError>(error));
    setStatus(QDeclarativeGeoRouteModel::Error);
    reply->deleteLater();
    reply_ = 0;
}


/*!
    \qmlclass RouteQuery QDeclarativeGeoRouteQuery
    \inqmlmodule QtLocation 5
    \ingroup qml-QtLocation5-routing
    \since Qt Location 5.0

    \brief The RouteQuery element is used to provide query parameters to a
           RouteModel.

    A RouteQuery contains all the parameters necessary to make a request
    to a routing service, which can then populate the contents of a RouteModel.

    These parameters describe key details of the route, such as \l waypoints to
    pass through, \l excludedAreas to avoid, the \l travelModes in use, as well
    as detailed preferences on how to optimize the route and what features
    to prefer or avoid along the path (such as toll roads, highways, etc).

    RouteQuery elements are used exclusively to fill out the value of a
    RouteModel's \l{RouteModel::query}{query} property, which can then begin
    the retrieval process to populate the model.

    \section2 Example Usage

    The following snipped shows an incomplete example of creating a RouteQuery
    element and setting it as the value of a RouteModel's \l{RouteModel::query}{query}
    property.

    \code
    RouteQuery {
        id: aQuery
    }

    RouteModel {
        query: aQuery
        autoUpdate: false
    }
    \endcode

    For a more complete example, see the documentation for the \l{RouteModel}
    element, and the Mapviewer example.

    \sa RouteModel

*/

QDeclarativeGeoRouteQuery::QDeclarativeGeoRouteQuery(QObject *parent)
    : QObject(parent),
      complete_(false)
{
}

QDeclarativeGeoRouteQuery::~QDeclarativeGeoRouteQuery()
{
    if (!waypoints_.isEmpty())
        waypoints_.clear();
    if (!exclusions_.isEmpty())
        exclusions_.clear();
}

/*!
    \internal
*/
void QDeclarativeGeoRouteQuery::componentComplete()
{
    complete_ = true;
}

/*!
    \qmlproperty QList<FeatureType> RouteQuery::featureTypes

    List of features that will be considered when planning the
    route. Features with a weight of NeutralFeatureWeight will not be returned.

    \list
    \li RouteQuery.NoFeature - No features will be taken into account when planning the route
    \li RouteQuery.TollFeature - Consider tollways when planning the route
    \li RouteQuery.HighwayFeature - Consider highways when planning the route
    \li RouteQuery.PublicTransitFeature - Consider public transit when planning the route
    \li RouteQuery.FerryFeature - Consider ferries when planning the route
    \li RouteQuery.TunnelFeature - Consider tunnels when planning the route
    \li RouteQuery.DirtRoadFeature - Consider dirt roads when planning the route
    \li RouteQuery.ParksFeature - Consider parks when planning the route
    \li RouteQuery.MotorPoolLaneFeature - Consider motor pool lanes when planning the route
    \endlist

    \sa setFeatureWeight, featureWeight
*/

QList<int> QDeclarativeGeoRouteQuery::featureTypes()
{
    QList<int> list;

    for (int i = 0; i < request_.featureTypes().count(); i++) {
        list.append(static_cast<int>(request_.featureTypes().at(i)));
    }
    return list;
}

/*!
    \qmlproperty int RouteQuery::numberAlternativeRoutes

    The number of alternative routes requested when requesting routes.
    The default value is 0.
*/


int QDeclarativeGeoRouteQuery::numberAlternativeRoutes() const
{
    return request_.numberAlternativeRoutes();
}

void QDeclarativeGeoRouteQuery::setNumberAlternativeRoutes(int numberAlternativeRoutes)
{
    if (numberAlternativeRoutes == request_.numberAlternativeRoutes())
        return;

    request_.setNumberAlternativeRoutes(numberAlternativeRoutes);

    if (complete_) {
        emit numberAlternativeRoutesChanged();
        emit queryDetailsChanged();
    }
}

/*!
    \qmlproperty QQmlListProperty<Coordinate> RouteQuery::waypoints


    The waypoint coordinates of the desired route.
    The waypoints should be given in order from origin to destination.
    Two or more coordinates are needed.

    Waypoints can be set as part of the RouteQuery element declaration or
    dynamically with the functions provided.

    \sa addWaypoint, removeWaypoint, clearWaypoints
*/

QQmlListProperty<QDeclarativeCoordinate> QDeclarativeGeoRouteQuery::waypoints()
{
    return QQmlListProperty<QDeclarativeCoordinate>(this, 0, waypoints_append, waypoints_count,
                                                    waypoints_at, waypoints_clear);
}

/*!
    \internal
*/
void QDeclarativeGeoRouteQuery::waypoints_append(QQmlListProperty<QDeclarativeCoordinate> *prop,
                                                 QDeclarativeCoordinate *waypoint)
{
    QDeclarativeGeoRouteQuery* model = static_cast<QDeclarativeGeoRouteQuery*>(prop->object);
    model->addWaypoint(waypoint);
}

/*!
    \internal
*/
int QDeclarativeGeoRouteQuery::waypoints_count(QQmlListProperty<QDeclarativeCoordinate> *prop)
{
    QDeclarativeGeoRouteQuery* model = static_cast<QDeclarativeGeoRouteQuery*>(prop->object);
    return model->waypoints_.count();
}

/*!
    \internal
*/
QDeclarativeCoordinate* QDeclarativeGeoRouteQuery::waypoints_at(QQmlListProperty<QDeclarativeCoordinate> *prop, int index)
{
    QDeclarativeGeoRouteQuery* model = static_cast<QDeclarativeGeoRouteQuery*>(prop->object);
    Q_ASSERT(index < model->waypoints_.count());
    return model->waypoints_.at(index);
}

/*!
    \internal
*/
void QDeclarativeGeoRouteQuery::waypoints_clear(QQmlListProperty<QDeclarativeCoordinate> *prop)
{
    QDeclarativeGeoRouteQuery* model = static_cast<QDeclarativeGeoRouteQuery*>(prop->object);
    model->clearWaypoints();
}

/*!
    \qmlproperty QQmlListProperty<BoundingBox> RouteQuery::excludedAreas

    Areas that the route must not cross.

    Excluded areas can be set as part of the RouteQuery element declaration or
    dynamically with the functions provided.

    \sa addExcludedArea, removeExcludedArea, clearExcludedAreas
*/

QQmlListProperty<QDeclarativeGeoBoundingBox> QDeclarativeGeoRouteQuery::excludedAreas()
{
    return QQmlListProperty<QDeclarativeGeoBoundingBox>(this, 0, exclusions_append,
                                                        exclusions_count, exclusions_at,
                                                        exclusions_clear);
}

/*!
    \internal
*/
void QDeclarativeGeoRouteQuery::exclusions_append(QQmlListProperty<QDeclarativeGeoBoundingBox> *prop, QDeclarativeGeoBoundingBox *area)
{
    QDeclarativeGeoRouteQuery* model = static_cast<QDeclarativeGeoRouteQuery*>(prop->object);
    model->addExcludedArea(area);
}

/*!
    \internal
*/
int QDeclarativeGeoRouteQuery::exclusions_count(QQmlListProperty<QDeclarativeGeoBoundingBox> *prop)
{
    QDeclarativeGeoRouteQuery* model = static_cast<QDeclarativeGeoRouteQuery*>(prop->object);
    return model->exclusions_.count();
}

/*!
    \internal
*/
QDeclarativeGeoBoundingBox* QDeclarativeGeoRouteQuery::exclusions_at(QQmlListProperty<QDeclarativeGeoBoundingBox> *prop, int index)
{
    QDeclarativeGeoRouteQuery* model = static_cast<QDeclarativeGeoRouteQuery*>(prop->object);
    Q_ASSERT(index < model->exclusions_.count());
    return model->exclusions_.at(index);
}

/*!
    \internal
*/
void QDeclarativeGeoRouteQuery::exclusions_clear(QQmlListProperty<QDeclarativeGeoBoundingBox> *prop)
{
    QDeclarativeGeoRouteQuery* model = static_cast<QDeclarativeGeoRouteQuery*>(prop->object);
    model->clearExcludedAreas();
}

/*!
    \qmlmethod QtLocation5::RouteQuery::addExcludedArea(BoundingBox)

    Adds the given area to excluded areas (areas that the route must not cross).
    Same area can only be added once.

    \sa removeExcludedArea, clearExcludedAreas
*/


void QDeclarativeGeoRouteQuery::addExcludedArea(QDeclarativeGeoBoundingBox* area)
{
    if (!area)
        return;
    if (exclusions_.contains(area))
        return;
    connect(area, SIGNAL(bottomLeftChanged()), this, SIGNAL(queryDetailsChanged()));
    connect(area, SIGNAL(bottomRightChanged()), this, SIGNAL(queryDetailsChanged()));
    connect(area, SIGNAL(topLeftChanged()), this, SIGNAL(queryDetailsChanged()));
    connect(area, SIGNAL(topRightChanged()), this, SIGNAL(queryDetailsChanged()));
    connect(area, SIGNAL(centerChanged()), this, SIGNAL(queryDetailsChanged()));
    connect(area, SIGNAL(widthChanged()), this, SIGNAL(queryDetailsChanged()));
    connect(area, SIGNAL(heightChanged()), this, SIGNAL(queryDetailsChanged()));
    exclusions_.append(area);
    if (complete_) {
        emit excludedAreasChanged();
        emit queryDetailsChanged();
    }
}

/*!
    \qmlmethod QtLocation5::RouteQuery::removeExcludedArea(BoundingBox)

    Removes the given area to excluded areas (areas that the route must not cross).

    \sa addExcludedArea, clearExcludedAreas
*/

void QDeclarativeGeoRouteQuery::removeExcludedArea(QDeclarativeGeoBoundingBox* area)
{
    if (!area)
        return;

    int index = exclusions_.lastIndexOf(area);
    if (index == -1) {
        qmlInfo(this) << tr("Cannot remove nonexistent area.");
        return;
    }
    exclusions_.removeAt(index);
    area->disconnect(this);
    emit excludedAreasChanged();
    emit queryDetailsChanged();
}

/*!
    \qmlmethod QtLocation5::RouteQuery::clearExcludedAreas()

    Clears all excluded areas (areas that the route must not cross).

    \sa addExcludedArea, removeExcludedAreas
*/

void QDeclarativeGeoRouteQuery::clearExcludedAreas()
{
    if (!exclusions_.count())
        return;
    for (int i = 0; i < exclusions_.count(); i++)
        exclusions_.at(i)->disconnect(this);
    exclusions_.clear();
    emit excludedAreasChanged();
    emit queryDetailsChanged();
}

/*!
    \qmlmethod QtLocation5::RouteQuery::addWaypoint(Coordinate)

    Appends a coordinate to the list of waypoints. Same coordinate
    can be set multiple times.

    \sa removeWaypoint, clearWaypoints
*/

void QDeclarativeGeoRouteQuery::addWaypoint(QDeclarativeCoordinate* waypoint)
{
    if (!waypoint) {
        qmlInfo(this) << tr("Not adding null waypoint.");
        return;
    }

    if (!waypoint->isValid()) {
        qmlInfo(this) << tr("Not adding invalid waypoint.");
        return;
    }

    if (!waypoints_.contains(waypoint)) {
        connect (waypoint, SIGNAL(latitudeChanged(double)), this, SIGNAL(queryDetailsChanged()));
        connect (waypoint, SIGNAL(longitudeChanged(double)), this, SIGNAL(queryDetailsChanged()));
        connect (waypoint, SIGNAL(altitudeChanged(double)), this, SIGNAL(queryDetailsChanged()));
        connect (waypoint, SIGNAL(destroyed(QObject*)), this, SLOT(waypointDestroyed(QObject*)));
    }
    waypoints_.append(waypoint);
    if (complete_) {
        emit waypointsChanged();
        emit queryDetailsChanged();
    }
}

/*!
    \qmlmethod QtLocation5::RouteQuery::removeWaypoint(Coordinate)

    Removes the given from the list of waypoints. In case same coordinate
    appears multiple times, the most recently added coordinate instance is
    removed.

    \sa addWaypoint, clearWaypoints
*/

void QDeclarativeGeoRouteQuery::removeWaypoint(QDeclarativeCoordinate* waypoint)
{
    if (!waypoint)
        return;

    int index = waypoints_.lastIndexOf(waypoint);
    if (index == -1) {
        qmlInfo(this) << tr("Cannot remove nonexistent waypoint.");
        return;
    }
    waypoints_.removeAt(index);
    if (!waypoints_.contains(waypoint)) {
        waypoint->disconnect(this);
    }
    emit waypointsChanged();
    emit queryDetailsChanged();
}

/*!
    \qmlmethod QtLocation5::RouteQuery::clearWaypoints()

    Clears all waypoints.

    \sa removeWaypoint, addWaypoints
*/

void QDeclarativeGeoRouteQuery::clearWaypoints()
{
    if (!waypoints_.count())
        return;
    for (int i = 0; i < waypoints_.count(); i++)
        waypoints_.at(i)->disconnect(this);
    waypoints_.clear();
    emit waypointsChanged();
    emit queryDetailsChanged();
}

/*!
    \qmlmethod QtLocation5::RouteQuery::setFeatureWeight(FeatureType, FeatureWeight)

    Defines the weight to associate with a feature during the planning of a
    route.

    Following lists the possible feature weights:

    \list
    \li RouteQuery.NeutralFeatureWeight - The presence or absence of the feature will not affect the planning of the route
    \li RouteQuery.PreferFeatureWeight - Routes which contain the feature will be preferred over those that do not
    \li RouteQuery.RequireFeatureWeight - Only routes which contain the feature will be considered, otherwise no route will be returned
    \li RouteQuery.AvoidFeatureWeight - Routes which do not contain the feature will be preferred over those that do
    \li RouteQuery.DisallowFeatureWeight - Only routes which do not contain the feature will be considered, otherwise no route will be returned
    \endlist

    \sa featureTypes, resetFeatureWeights, featureWeight

*/

void QDeclarativeGeoRouteQuery::setFeatureWeight(FeatureType featureType, FeatureWeight featureWeight)
{
    if (featureType == NoFeature && !request_.featureTypes().isEmpty()) {
        resetFeatureWeights();
        return;
    }

    // Check if the weight changes, as we need to signal it
    FeatureWeight originalWeight = static_cast<FeatureWeight>(request_.featureWeight(static_cast<QGeoRouteRequest::FeatureType>(featureType)));
    if (featureWeight == originalWeight)
        return;

    request_.setFeatureWeight(static_cast<QGeoRouteRequest::FeatureType>(featureType),
                              static_cast<QGeoRouteRequest::FeatureWeight>(featureWeight));
    if (complete_ && ((originalWeight == NeutralFeatureWeight) || (featureWeight == NeutralFeatureWeight))) {
        // featureTypes should now give a different list, because the original and new weight
        // were not same, and other one was neutral weight
        emit featureTypesChanged();
        emit queryDetailsChanged();
    }
}

/*!
    \qmlmethod QtLocation5::RouteQuery::resetFeatureWeights()

    Resets all feature weights to their default state (NeutralFeatureWeight).

    \sa featureTypes, setFeatureWeights, featureWeight
*/
void QDeclarativeGeoRouteQuery::resetFeatureWeights()
{
    // reset all feature types.
    QList<QGeoRouteRequest::FeatureType> featureTypes = request_.featureTypes();
    for (int i = 0; i < featureTypes.count(); ++i) {
        request_.setFeatureWeight(featureTypes.at(i), QGeoRouteRequest::NeutralFeatureWeight);
    }
    if (complete_) {
        emit featureTypesChanged();
        emit queryDetailsChanged();
    }
}

/*!
    \qmlmethod FeatureWeight QtLocation5::RouteQuery::featureWeight(FeatureType featureType)

    Gets the weight for the \a featureType.

    \sa featureTypes, setFeatureWeight, resetFeatureWeights
*/

int QDeclarativeGeoRouteQuery::featureWeight(FeatureType featureType)
{
    return request_.featureWeight(static_cast<QGeoRouteRequest::FeatureType>(featureType));
}

/*!
    \internal
*/
void QDeclarativeGeoRouteQuery::setTravelModes(QDeclarativeGeoRouteQuery::TravelModes travelModes)
{
    QGeoRouteRequest::TravelModes reqTravelModes;

    if (travelModes & QDeclarativeGeoRouteQuery::CarTravel)
        reqTravelModes |= QGeoRouteRequest::CarTravel;
    if (travelModes & QDeclarativeGeoRouteQuery::PedestrianTravel)
        reqTravelModes |= QGeoRouteRequest::PedestrianTravel;
    if (travelModes & QDeclarativeGeoRouteQuery::BicycleTravel)
        reqTravelModes |= QGeoRouteRequest::BicycleTravel;
    if (travelModes & QDeclarativeGeoRouteQuery::PublicTransitTravel)
        reqTravelModes |= QGeoRouteRequest::PublicTransitTravel;
    if (travelModes & QDeclarativeGeoRouteQuery::TruckTravel)
        reqTravelModes |= QGeoRouteRequest::TruckTravel;

    if (reqTravelModes == request_.travelModes())
        return;

    request_.setTravelModes(reqTravelModes);

    if (complete_) {
        emit travelModesChanged();
        emit queryDetailsChanged();
    }
}


/*!
    \qmlproperty SegmentDetail RouteQuery::segmentDetail

    The level of detail which will be used in the representation of routing segments.
    Values can be combined with OR ('|') -operator.

    \list
    \li RouteQuery.NoSegmentData - No segment data should be included with the route
    \li RouteQuery.BasicSegmentData - Basic segment data will be included with the route
    \endlist

    The default value is RouteQuery.BasicSegmentData
*/

void QDeclarativeGeoRouteQuery::setSegmentDetail(SegmentDetail segmentDetail)
{
    if (static_cast<QGeoRouteRequest::SegmentDetail>(segmentDetail) == request_.segmentDetail())
        return;
    request_.setSegmentDetail(static_cast<QGeoRouteRequest::SegmentDetail>(segmentDetail));
    if (complete_) {
        emit segmentDetailChanged();
        emit queryDetailsChanged();
    }
}

QDeclarativeGeoRouteQuery::SegmentDetail QDeclarativeGeoRouteQuery::segmentDetail() const
{
    return static_cast<QDeclarativeGeoRouteQuery::SegmentDetail>(request_.segmentDetail());
}

/*!
    \qmlproperty ManeuverDetail RouteQuery::maneuverDetail

    The level of detail which will be used in the representation of routing maneuvers.
    Values can be combined with OR ('|') -operator.

    \list
    \li RouteQuery.NoManeuvers - No maneuvers should be included with the route
    \li RouteQuery.BasicManeuvers - Basic maneuvers will be included with the route
    \endlist

    The default value is RouteQuery.BasicManeuvers
*/

void QDeclarativeGeoRouteQuery::setManeuverDetail(ManeuverDetail maneuverDetail)
{
    if (static_cast<QGeoRouteRequest::ManeuverDetail>(maneuverDetail) == request_.maneuverDetail())
        return;
    request_.setManeuverDetail(static_cast<QGeoRouteRequest::ManeuverDetail>(maneuverDetail));
    if (complete_) {
        emit maneuverDetailChanged();
        emit queryDetailsChanged();
    }
}

QDeclarativeGeoRouteQuery::ManeuverDetail QDeclarativeGeoRouteQuery::maneuverDetail() const
{
    return static_cast<QDeclarativeGeoRouteQuery::ManeuverDetail>(request_.maneuverDetail());
}

/*!
    \qmlproperty enumeration RouteQuery::travelModes

    The travel modes which should be considered during the planning of the route.
    Values can be combined with OR ('|') -operator.

    \list
    \li RouteQuery.CarTravel - The route will be optimized for someone who is driving a car
    \li RouteQuery.PedestrianTravel - The route will be optimized for someone who is walking
    \li RouteQuery.BicycleTravel - The route will be optimized for someone who is riding a bicycle
    \li RouteQuery.PublicTransitTravel - The route will be optimized for someone who is making use of public transit
    \li RouteQuery.TruckTravel - The route will be optimized for someone who is driving a truck
    \endlist

    The default value is RouteQuery.CarTravel
*/

QDeclarativeGeoRouteQuery::TravelModes QDeclarativeGeoRouteQuery::travelModes() const
{
    QGeoRouteRequest::TravelModes reqTravelModes = request_.travelModes();
    QDeclarativeGeoRouteQuery::TravelModes travelModes;

    if (reqTravelModes & QGeoRouteRequest::CarTravel)
        travelModes |= QDeclarativeGeoRouteQuery::CarTravel;
    if (reqTravelModes & QGeoRouteRequest::PedestrianTravel)
        travelModes |= QDeclarativeGeoRouteQuery::PedestrianTravel;
    if (reqTravelModes & QGeoRouteRequest::BicycleTravel)
        travelModes |= QDeclarativeGeoRouteQuery::BicycleTravel;
    if (reqTravelModes & QGeoRouteRequest::PublicTransitTravel)
        travelModes |= QDeclarativeGeoRouteQuery::PublicTransitTravel;
    if (reqTravelModes & QGeoRouteRequest::TruckTravel)
        travelModes |= QDeclarativeGeoRouteQuery::TruckTravel;

    return travelModes;
}

/*!
    \qmlproperty RouteOptimizations RouteQuery::routeOptimizations

    The route optimizations which should be considered during the planning of the route.
    Values can be combined with OR ('|') -operator.

    \list
    \li RouteQuery.ShortestRoute - Minimize the length of the journey
    \li RouteQuery.FastestRoute - Minimize the traveling time for the journey
    \li RouteQuery.MostEconomicRoute - Minimize the cost of the journey
    \li RouteQuery.MostScenicRoute - Maximize the scenic potential of the journey
    \endlist

    The default value is RouteQuery.FastestRoute
*/

QDeclarativeGeoRouteQuery::RouteOptimizations QDeclarativeGeoRouteQuery::routeOptimizations() const
{
    QGeoRouteRequest::RouteOptimizations reqOptimizations = request_.routeOptimization();
    QDeclarativeGeoRouteQuery::RouteOptimizations optimization;

    if (reqOptimizations & QGeoRouteRequest::ShortestRoute)
        optimization |= QDeclarativeGeoRouteQuery::ShortestRoute;
    if (reqOptimizations & QGeoRouteRequest::FastestRoute)
        optimization |= QDeclarativeGeoRouteQuery::FastestRoute;
    if (reqOptimizations & QGeoRouteRequest::MostEconomicRoute)
        optimization |= QDeclarativeGeoRouteQuery::MostEconomicRoute;
    if (reqOptimizations & QGeoRouteRequest::MostScenicRoute)
        optimization |= QDeclarativeGeoRouteQuery::MostScenicRoute;

    return optimization;
}

void QDeclarativeGeoRouteQuery::setRouteOptimizations(QDeclarativeGeoRouteQuery::RouteOptimizations optimization)
{
    QGeoRouteRequest::RouteOptimizations reqOptimizations;

    if (optimization & QDeclarativeGeoRouteQuery::ShortestRoute)
        reqOptimizations |= QGeoRouteRequest::ShortestRoute;
    if (optimization & QDeclarativeGeoRouteQuery::FastestRoute)
        reqOptimizations |= QGeoRouteRequest::FastestRoute;
    if (optimization & QDeclarativeGeoRouteQuery::MostEconomicRoute)
        reqOptimizations |= QGeoRouteRequest::MostEconomicRoute;
    if (optimization & QDeclarativeGeoRouteQuery::MostScenicRoute)
        reqOptimizations |= QGeoRouteRequest::MostScenicRoute;

    if (reqOptimizations == request_.routeOptimization())
        return;

    request_.setRouteOptimization(reqOptimizations);

    if (complete_) {
        emit routeOptimizationsChanged();
        emit queryDetailsChanged();
    }
}

/*!
    \internal
*/
QGeoRouteRequest& QDeclarativeGeoRouteQuery::routeRequest()
{
    // Bit inefficient, but waypoint and excludearea count is not big
    QList<QGeoCoordinate> waypoints;
    for (int i = 0; i < waypoints_.count(); i++)
        waypoints.append(waypoints_.at(i)->coordinate());
    QList<QGeoBoundingBox> exclusions;
    for (int i = 0; i < exclusions_.count(); i++)
        exclusions.append(exclusions_.at(i)->box());

    request_.setWaypoints(waypoints);
    request_.setExcludeAreas(exclusions);
    return request_;
}

/*!
    \internal
*/
void QDeclarativeGeoRouteQuery::waypointDestroyed(QObject* object)
{
    const int index = waypoints_.indexOf(static_cast<QDeclarativeCoordinate*>(object));

    if (index >= 0) {
        waypoints_.removeAt(index);
        if (complete_) {
            emit waypointsChanged();
            emit queryDetailsChanged();
        }
    }
}

#include "moc_qdeclarativegeoroutemodel_p.cpp"

QT_END_NAMESPACE
