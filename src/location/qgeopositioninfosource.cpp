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
#include <qgeopositioninfosource.h>
#include "qgeopositioninfosource_p.h"
#include "qgeopositioninfosourcefactory.h"

#include <QFile>
#include <QPluginLoader>
#include <QStringList>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/private/qlibrary_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
        ("org.qt-project.qt.position.sourcefactory/5.0",
         QStringLiteral("/position")))
#endif

/*!
    \class QGeoPositionInfoSource
    \inmodule QtLocation
    \ingroup QtLocation-positioning
    \since Qt Location 5.0

    \brief The QGeoPositionInfoSource class is an abstract base class for the distribution of positional updates.

    The static function QGeoPositionInfoSource::createDefaultSource() creates a default
    position source that is appropriate for the platform, if one is available.
    Otherwise, QGeoPositionInfoSource will check for available plugins that
    implement the QGeoPositionInfoSourceFactory interface.

    Users of a QGeoPositionInfoSource subclass can request the current position using
    requestUpdate(), or start and stop regular position updates using
    startUpdates() and stopUpdates(). When an update is available,
    positionUpdated() is emitted. The last known position can be accessed with
    lastKnownPosition().

    If regular position updates are required, setUpdateInterval() can be used
    to specify how often these updates should be emitted. If no interval is
    specified, updates are simply provided whenever they are available.
    For example:

    \code
        // Emit updates every 10 seconds if available
        QGeoPositionInfoSource *source = QGeoPositionInfoSource::createDefaultSource(0);
        if (source)
            source->setUpdateInterval(10000);
    \endcode

    To remove an update interval that was previously set, call
    setUpdateInterval() with a value of 0.

    Note that the position source may have a minimum value requirement for
    update intervals, as returned by minimumUpdateInterval().

    \warning On Windows CE it is not possible to detect if a device is GPS enabled.
    The default position source on a Windows CE device without GPS support will never provide any position data.
*/

/*!
    \enum QGeoPositionInfoSource::PositioningMethod
    Defines the types of positioning methods.

    \value SatellitePositioningMethods Satellite-based positioning methods such as GPS or GLONASS.
    \value NonSatellitePositioningMethods Other positioning methods such as 3GPP cell identifier or WiFi based positioning.
    \value AllPositioningMethods Satellite-based positioning methods as soon as available. Otherwise non-satellite based methods.
*/

void QGeoPositionInfoSourcePrivate::loadMeta()
{
    metaData = plugins().value(providerName);
}

void QGeoPositionInfoSourcePrivate::loadPlugin()
{
    int idx = int(metaData.value(QStringLiteral("index")).toDouble());
    if (idx < 0)
        return;
    factory = qobject_cast<QGeoPositionInfoSourceFactory *>(loader()->instance(idx));
}

QHash<QString, QJsonObject> QGeoPositionInfoSourcePrivate::plugins(bool reload)
{
    static QHash<QString, QJsonObject> plugins;
    static bool alreadyDiscovered = false;

    if (reload == true)
        alreadyDiscovered = false;

    if (!alreadyDiscovered) {
        loadPluginMetadata(plugins);
        alreadyDiscovered = true;
    }
    return plugins;
}

static bool pluginComparator(const QJsonObject &p1, const QJsonObject &p2)
{
    const QString prio = QStringLiteral("Priority");
    if (p1.contains(prio) && !p2.contains(prio))
        return true;
    if (!p1.contains(prio) && p2.contains(prio))
        return false;
    if (p1.value(prio).isDouble() && !p2.value(prio).isDouble())
        return true;
    if (!p1.value(prio).isDouble() && p2.value(prio).isDouble())
        return false;
    return (p1.value(prio).toDouble() > p2.value(prio).toDouble());
}

QList<QJsonObject> QGeoPositionInfoSourcePrivate::pluginsSorted()
{
    QList<QJsonObject> list = plugins().values();
    qStableSort(list.begin(), list.end(), pluginComparator);
    return list;
}

void QGeoPositionInfoSourcePrivate::loadPluginMetadata(QHash<QString, QJsonObject> &plugins)
{
    QFactoryLoader *l = loader();
    QList<QJsonObject> meta = l->metaData();
    for (int i = 0; i < meta.size(); ++i) {
        QJsonObject obj = meta.at(i).value(QStringLiteral("MetaData")).toObject();
        obj.insert(QStringLiteral("index"), i);
        plugins.insertMulti(obj.value(QStringLiteral("Provider")).toString(), obj);
    }
}

/*!
    Creates a position source with the specified \a parent.
*/

QGeoPositionInfoSource::QGeoPositionInfoSource(QObject *parent)
        : QObject(parent),
        d(new QGeoPositionInfoSourcePrivate)
{
    d->interval = 0;
    d->methods = 0;
}

/*!
    Destroys the position source.
*/
QGeoPositionInfoSource::~QGeoPositionInfoSource()
{
    delete d;
}

/*!
    \property QGeoPositionInfoSource::sourceName
    \brief This property holds the unique name of the position source
           implementation in use.

    This is the same name that can be passed to createSource() in order to
    create a new instance of a particular position source implementation.
*/
QString QGeoPositionInfoSource::sourceName() const
{
    return d->metaData.value(QStringLiteral("Provider")).toString();
}

/*!
    \property QGeoPositionInfoSource::updateInterval
    \brief This property holds the requested interval in milliseconds between each update.

    If the update interval is not set (or is set to 0) the
    source will provide updates as often as necessary.

    If the update interval is set, the source will provide updates at an
    interval as close to the requested interval as possible. If the requested
    interval is less than the minimumUpdateInterval(),
    the minimum interval is used instead.

    Changes to the update interval will happen as soon as is practical, however the
    time the change takes may vary between implementations.  Whether or not the elapsed
    time from the previous interval is counted as part of the new interval is also
    implementation dependent.

    The default value for this property is 0.

    Note: Subclass implementations must call the base implementation of
    setUpdateInterval() so that updateInterval() returns the correct value.
*/
void QGeoPositionInfoSource::setUpdateInterval(int msec)
{
    d->interval = msec;
}

int QGeoPositionInfoSource::updateInterval() const
{
    return d->interval;
}

/*!
    Sets the preferred positioning methods for this source to \a methods.

    If \a methods includes a method that is not supported by the source, the
    unsupported method will be ignored.

    If \a methods does not include any methods supported by the source, the
    preferred methods will be set to the set of methods which the source supports.

    \b {Note:} When reimplementing this method, subclasses must call the
    base method implementation to ensure preferredPositioningMethods() returns the correct value.

    \sa supportedPositioningMethods()
*/
void QGeoPositionInfoSource::setPreferredPositioningMethods(PositioningMethods methods)
{
    d->methods = methods & supportedPositioningMethods();
    if (d->methods == 0) {
        d->methods = supportedPositioningMethods();
    }
}

/*!
    Returns the positioning methods set by setPreferredPositioningMethods().
*/
QGeoPositionInfoSource::PositioningMethods QGeoPositionInfoSource::preferredPositioningMethods() const
{
    return d->methods;
}

/*!
    Creates and returns a position source with the given \a parent that
    reads from the system's default sources of location data, or the plugin
    with the highest available priority.

    Returns 0 if the system has no default position source, no valid plugins
    could be found or the user does not have the permission to access the current position.
*/
QGeoPositionInfoSource *QGeoPositionInfoSource::createDefaultSource(QObject *parent)
{
    QGeoPositionInfoSourcePrivate *d = new QGeoPositionInfoSourcePrivate;

    QList<QJsonObject> plugins = QGeoPositionInfoSourcePrivate::pluginsSorted();
    foreach (const QJsonObject &obj, plugins) {
        if (obj.value(QStringLiteral("Position")).isBool()
                && obj.value(QStringLiteral("Position")).toBool()) {
            d->metaData = obj;
            d->loadPlugin();
            QGeoPositionInfoSource *s = d->factory->positionInfoSource(parent);
            if (s) {
                s->d->metaData = d->metaData;
                delete d;
                return s;
            }
        }
    }

    delete d;
    return 0;
}


/*!
    Creates and returns a position source with the given \a parent,
    by loading the plugin named \a sourceName.

    Returns 0 if the plugin cannot be found.
*/
QGeoPositionInfoSource *QGeoPositionInfoSource::createSource(const QString &sourceName, QObject *parent)
{
    QGeoPositionInfoSourcePrivate *d = new QGeoPositionInfoSourcePrivate;
    QHash<QString, QJsonObject> plugins = QGeoPositionInfoSourcePrivate::plugins();
    if (plugins.contains(sourceName)) {
        d->metaData = plugins.value(sourceName);
        d->loadPlugin();
        QGeoPositionInfoSource *src = d->factory->positionInfoSource(parent);
        if (src) {
            src->d->metaData = d->metaData;
            delete d;
            return src;
        }
    }

    delete d;
    return 0;
}


/*!
    Returns a list of available source plugins. This includes any default backend
    plugin for the current platform.
*/
QStringList QGeoPositionInfoSource::availableSources()
{
    QStringList plugins;
    QHash<QString, QJsonObject> meta = QGeoPositionInfoSourcePrivate::plugins();
    foreach (const QString &name, meta.keys()) {
        if (meta.value(name).value(QStringLiteral("Position")).isBool()
                && meta.value(name).value(QStringLiteral("Position")).toBool()) {
            plugins << name;
        }
    }

    return plugins;
}

/*!
    \fn QGeoPositionInfo QGeoPositionInfoSource::lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const = 0;

    Returns an update containing the last known position, or a null update
    if none is available.

    If \a fromSatellitePositioningMethodsOnly is true, this returns the last
    known position received from a satellite positioning method; if none
    is available, a null update is returned.
*/

/*!
    \fn virtual PositioningMethods QGeoPositionInfoSource::supportedPositioningMethods() const = 0;

    Returns the positioning methods available to this source.

    \sa setPreferredPositioningMethods()
*/


/*!
    \property QGeoPositionInfoSource::minimumUpdateInterval
    \brief This property holds the minimum time (in milliseconds) required to retrieve a position update.

    This is the minimum value accepted by setUpdateInterval() and
    requestUpdate().
*/


/*!
    \fn virtual void QGeoPositionInfoSource::startUpdates() = 0;

    Starts emitting updates at regular intervals as specified by setUpdateInterval().

    If setUpdateInterval() has not been called, the source will emit updates
    as soon as they become available.

    An updateTimout() signal will be emitted if this QGeoPositionInfoSource subclass determines
    that it will not be able to provide regular updates.  This could happen if a satellite fix is
    lost or if a hardware error is detected.  Position updates will recommence if the data becomes
    available later on.  The updateTimout() signal will not be emitted again until after the
    periodic updates resume.
*/

/*!
    \fn virtual void QGeoPositionInfoSource::stopUpdates() = 0;

    Stops emitting updates at regular intervals.
*/

/*!
    \fn virtual void QGeoPositionInfoSource::requestUpdate(int timeout = 0);

    Attempts to get the current position and emit positionUpdated() with
    this information. If the current position cannot be found within the given \a timeout
    (in milliseconds) or if \a timeout is less than the value returned by
    minimumUpdateInterval(), updateTimeout() is emitted.

    If the timeout is zero, the timeout defaults to a reasonable timeout
    period as appropriate for the source.

    This does nothing if another update request is in progress. However
    it can be called even if startUpdates() has already been called and
    regular updates are in progress.

    If the source uses multiple positioning methods, it tries to gets the
    current position from the most accurate positioning method within the
    given timeout.
*/

/*!
     \fn virtual QGeoPositionInfoSource::Error QGeoPositionInfoSource::error() const;

     Returns the type of error that last occurred.

*/

/*!
    \fn void QGeoPositionInfoSource::positionUpdated(const QGeoPositionInfo &update);

    If startUpdates() or requestUpdate() is called, this signal is emitted
    when an update becomes available.

    The \a update value holds the value of the new update.
*/

/*!
    \fn void QGeoPositionInfoSource::updateTimeout();

    If requestUpdate() was called, this signal will be emitted if the current position could not
    be retrieved within the specified timeout.

    If startUpdates() has been called, this signal will be emitted if this QGeoPositionInfoSource
    subclass determines that it will not be able to provide further regular updates.  This signal
    will not be emitted again until after the regular updates resume.
*/

/*!
    \fn void QGeoPositionInfoSource::error(QGeoPositionInfoSource::Error positioningError)

    This signal is emitted after an error occurred. The \a positioningError
    parameter describes the type of error that occurred.

*/

/*!
    \enum QGeoPositionInfoSource::Error

    The Error enumeration represents the errors which can occur.

    \value AccessError The connection setup to the remote positioning backend failed because the
        application lacked the required privileges.
    \value ClosedError  The remote positioning backend closed the connection, which happens for example in case
        the user is switching location services to off. This object becomes invalid and should be deleted.
        A call to createDefaultSource() can be used to create a new source later on.
    \value UnknownSourceError An unidentified error occurred.
 */

#include "moc_qgeopositioninfosource.cpp"

QT_END_NAMESPACE
