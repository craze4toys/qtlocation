/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgeosearchmanager.h"
#include "qgeosearchmanager_p.h"
#include "qgeosearchmanagerengine.h"

#include "qgeoboundingbox.h"
#include "qgeoboundingcircle.h"

#include <QLocale>

QTM_BEGIN_NAMESPACE

/*!
    \class QGeoSearchManager

    \brief The QGeoSearchManager class provides support for searching
    operations related to geographic information.


    \inmodule QtLocation
    \since 1.1

    \ingroup maps-places

    The geocode(), reverseGeocode() and search() functions return
    QGeoSearchReply objects, which manage these operations and report on the
    result of the operations and any errors which may have occurred.

    The geocode() and reverseGeocode() functions can be used to convert
    QGeoAddress instances to QGeoCoordinate instances and vice-versa.

    The search() function allows a user to perform a free text search
    for location information.  If the string provided can be interpreted as
    an address it can be geocoded to coordinate information.

    Instances of QGeoSearchManager can be accessed with
    QGeoServiceProvider::searchManager().
*/

/*!
    Constructs a new manager with the specified \a parent and with the
    implementation provided by \a engine.

    This constructor is used interally by QGeoServiceProviderFactory. Regular
    users should acquire instances of QGeoSearchManager with
    QGeoServiceProvider::searchManager();
*/
QGeoSearchManager::QGeoSearchManager(QGeoSearchManagerEngine *engine, QObject *parent)
    : QObject(parent),
      d_ptr(new QGeoSearchManagerPrivate())
{
    d_ptr->engine = engine;
    if (d_ptr->engine) {
        d_ptr->engine->setParent(this);

        connect(d_ptr->engine,
                SIGNAL(finished(QGeoSearchReply*)),
                this,
                SIGNAL(finished(QGeoSearchReply*)));

        connect(d_ptr->engine,
                SIGNAL(error(QGeoSearchReply*, QGeoSearchReply::Error, QString)),
                this,
                SIGNAL(error(QGeoSearchReply*, QGeoSearchReply::Error, QString)));
    } else {
        qFatal("The search manager engine that was set for this search manager was NULL.");
    }
}

/*!
    Destroys this manager.
*/
QGeoSearchManager::~QGeoSearchManager()
{
    delete d_ptr;
}

/*!
    Returns the name of the engine which implements the behaviour of this
    search manager.

    The combination of managerName() and managerVersion() should be unique
    amongst the plugin implementations.
*/
QString QGeoSearchManager::managerName() const
{
//    if (!d_ptr->engine)
//        return QString();

    return d_ptr->engine->managerName();
}

/*!
    Returns the version of the engine which implements the behaviour of this
    search manager.

    The combination of managerName() and managerVersion() should be unique
    amongst the plugin implementations.
*/
int QGeoSearchManager::managerVersion() const
{
//    if (!d_ptr->engine)
//        return -1;

    return d_ptr->engine->managerVersion();
}

/*!
    Begins the geocoding of \a address. Geocoding is the process of finding a
    coordinate that corresponds to a given address.

    A QGeoSearchReply object will be returned, which can be used to manage the
    geocoding operation and to return the results of the operation.

    This manager and the returned QGeoSearchReply object will emit signals
    indicating if the operation completes or if errors occur.

    If supportsGeocoding() returns false an
    QGeoSearchReply::UnsupportedOptionError will occur.

    Once the operation has completed, QGeoSearchReply::locations() can be used to
    retrieve the results, which will consist of a list of QGeoLocation objects.
    These object represent a combination of coordinate and address data.

    The address data returned in the results may be different from \a address.
    This will usually occur if the geocoding service backend uses a different
    canonical form of addresses or if \a address was only partially filled out.

    If \a bounds is non-null and valid QGeoBoundingArea it will be used to
    limit the results to thos that are contained within \a bounds. This is
    particularly useful if \a address is only partially filled out, as the
    service will attempt to geocode all matches for the specified data.

    The user is responsible for deleting the returned reply object, although
    this can be done in the slot connected to QGeoSearchManager::finished(),
    QGeoSearchManager::error(), QGeoSearchReply::finished() or
    QGeoSearchReply::error() with deleteLater().
*/
QGeoSearchReply* QGeoSearchManager::geocode(const QGeoAddress &address, QGeoBoundingArea *bounds)
{
//    if (!d_ptr->engine)
//        return new QGeoSearchReply(QGeoSearchReply::EngineNotSetError, "The search manager was not created with a valid engine.", this);

    return d_ptr->engine->geocode(address, bounds);
}


/*!
    Begins the reverse geocoding of \a coordinate. Reverse geocoding is the
    process of finding an address that corresponds to a given coordinate.

    A QGeoSearchReply object will be returned, which can be used to manage the
    reverse geocoding operation and to return the results of the operation.

    This manager and the returned QGeoSearchReply object will emit signals
    indicating if the operation completes or if errors occur.

    If supportsReverseGeocoding() returns false an
    QGeoSearchReply::UnsupportedOptionError will occur.

    At that point QGeoSearchReply::locations() can be used to retrieve the
    results, which will consist of a list of QGeoLocation objects. These object
    represent a combination of coordinate and address data.

    The coordinate data returned in the results may be different from \a
    coordinate. This will usually occur if the reverse geocoding service
    backend shifts the coordinates to be closer to the matching addresses, or
    if the backend returns results at multiple levels of detail.

    If multiple results are returned by the reverse geocoding service backend
    they will be provided in order of specificity. This normally occurs if the
    backend is configured to reverse geocode across multiple levels of detail.
    As an example, some services will return address and coordinate pairs for
    the street address, the city, the state and the country.

    If \a bounds is non-null and a valid QGeoBoundingBox it will be used to
    limit the results to thos that are contained within \a bounds.

    The user is responsible for deleting the returned reply object, although
    this can be done in the slot connected to QGeoSearchManager::finished(),
    QGeoSearchManager::error(), QGeoSearchReply::finished() or
    QGeoSearchReply::error() with deleteLater().
*/
QGeoSearchReply* QGeoSearchManager::reverseGeocode(const QGeoCoordinate &coordinate, QGeoBoundingArea *bounds)
{
//    if (!d_ptr->engine)
//        return new QGeoSearchReply(QGeoSearchReply::EngineNotSetError, "The search manager was not created with a valid engine.", this);

    return d_ptr->engine->reverseGeocode(coordinate, bounds);
}

/*!
    Begins searching for a location matching \a searchString.  The value of
    \a searchTypes will determine whether the search is for addresses only,
    for landmarks only or for both.

    A QGeoSearchReply object will be returned, which can be used to manage the
    geocoding operation and to return the results of the operation.

    This manager and the returned QGeoSearchReply object will emit signals
    indicating if the operation completes or if errors occur.

    If supportsGeocoding() returns false and \a searchTypes is
    QGeoSearchManager::SearchGeocode an
    QGeoSearchReply::UnsupportedOptionError will occur.

    Once the operation has completed, QGeoSearchReply::locations() can be used to
    retrieve the results, which will consist of a list of QGeoLocation objects.
    These object represent a combination of coordinate and address data.

    If \a limit is -1 the entire result set will be returned, otherwise at most
    \a limit results will be returned.

    The \a offset parameter is used to ask the search service to not return the
    first \a offset results.

    The \a limit and \a offset results are used together to implement paging.

    If \a bounds is non-null and a valid QGeoBoundingArea it will be used to
    limit the results to thos that are contained within \a bounds.

    The user is responsible for deleting the returned reply object, although
    this can be done in the slot connected to QGeoSearchManager::finished(),
    QGeoSearchManager::error(), QGeoSearchReply::finished() or
    QGeoSearchReply::error() with deleteLater().
*/
QGeoSearchReply* QGeoSearchManager::search(const QString &searchString,
        int limit,
        int offset,
        QGeoBoundingArea *bounds)
{
//    if (!d_ptr->engine)
//        return new QGeoSearchReply(QGeoSearchReply::EngineNotSetError, "The search manager was not created with a valid engine.", this);

    QGeoSearchReply *reply = d_ptr->engine->search(searchString,
                             limit,
                             offset,
                             bounds);
    return reply;
}

/*!
    Returns whether this manager supports geocoding.
*/
bool QGeoSearchManager::supportsGeocoding() const
{
//    if (!d_ptr->engine)
//        return false;

    return d_ptr->engine->supportsGeocoding();
}

/*!
    Returns whether this manager supports reverse geocoding.
*/
bool QGeoSearchManager::supportsReverseGeocoding() const
{
    return d_ptr->engine->supportsReverseGeocoding();
}

/*!
    Sets the locale to be used by the this manager to \a locale.

    If this search manager supports returning the results
    in different languages, they will be returned in the language of \a locale.

    The locale used defaults to the system locale if this is not set.
*/
void QGeoSearchManager::setLocale(const QLocale &locale)
{
    d_ptr->engine->setLocale(locale);
}

/*!
    Returns the locale used to hint to this search manager about what
    language to use for the results.
*/
QLocale QGeoSearchManager::locale() const
{
    return d_ptr->engine->locale();
}

/*!
\fn void QGeoSearchManager::finished(QGeoSearchReply* reply)

    This signal is emitted when \a reply has finished processing.

    If reply::error() equals QGeoSearchReply::NoError then the processing
    finished successfully.

    This signal and QGeoSearchReply::finished() will be emitted at the same
    time.

    \note Do no delete the \a reply object in the slot connected to this
    signal. Use deleteLater() instead.
*/

/*!
\fn void QGeoSearchManager::error(QGeoSearchReply* reply, QGeoSearchReply::Error error, QString errorString)

    This signal is emitted when an error has been detected in the processing of
    \a reply. The QGeoSearchManager::finished() signal will probably follow.

    The error will be described by the error code \a error. If \a errorString is
    not empty it will contain a textual description of the error.

    This signal and QGeoSearchReply::error() will be emitted at the same time.

    \note Do no delete the \a reply object in the slot connected to this
    signal. Use deleteLater() instead.
*/

/*******************************************************************************
*******************************************************************************/

QGeoSearchManagerPrivate::QGeoSearchManagerPrivate()
    : engine(0) {}

QGeoSearchManagerPrivate::~QGeoSearchManagerPrivate()
{
    if (engine)
        delete engine;
}

/*******************************************************************************
*******************************************************************************/

#include "moc_qgeosearchmanager.cpp"

QTM_END_NAMESPACE
