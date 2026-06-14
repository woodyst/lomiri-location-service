/*
 * Copyright 2013 Canonical Ltd.
 * Copyright 2022 UBports Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Thomas Voß <thomas.voss@canonical.com>
 *         Marius Gripsgard <marius@ubports.com>
 */

#include "core_geo_position_info_source.h"

#include <cmath>
#include <chrono>

#include <QGuiApplication>
#include <QtCore>

#include "instance.h"

#include <com/lomiri/location/service/session/interface.h>
#include <com/lomiri/location/heading.h>
#include <com/lomiri/location/position.h>
#include <com/lomiri/location/velocity.h>
#include <com/lomiri/location/update.h>

namespace cll = com::lomiri::location;
namespace cllss = com::lomiri::location::service::session;

static Instance* instance()
{
    static Instance* serviceInstance = nullptr;

    if (!serviceInstance) {
        serviceInstance = new Instance();

        // Ensure we clean up the instance with valid dbus-cpp state,
        // otherwise hangs or crashes may occur stopping its worker.
        QObject::connect(qGuiApp, &QGuiApplication::aboutToQuit,
                         qGuiApp, []()
        {
            delete serviceInstance;
            serviceInstance = nullptr;
        });
    }

    return serviceInstance;
}

struct core::GeoPositionInfoSource::Private
{
    // If an application requests an individual position update with a
    // timeout value of 0, we bump the timeout to the default value of
    // 10 seconds.
    static const unsigned int default_timeout_in_ms = 10 * 1000;

    // Processes the incoming position update and translates it to Qt world.
    void handlePositionUpdate(const cll::Update<cll::Position>& position);

    // Processes the incoming heading update and translates it to Qt world.
    void handleHeadingUpdate(const cll::Update<cll::Heading>& heading);

    // Processes the incoming velocity update and translates it to Qt world.
    void handleVelocityUpdate(const cll::Update<cll::Velocity>& velocity);

    void createLocationServiceSession();
    void destroyLocationServiceSession();

    // Creates a new instance and attempts to connect to the background service.
    // Stores errors in the error member.
    Private(core::GeoPositionInfoSource* parent);
    ~Private();

    core::GeoPositionInfoSource* parent;
    cllss::Interface::Ptr session;
    QMutex lastKnownPositionGuard;
    QGeoPositionInfo lastKnownPosition;
    QTimer timer;
    QGeoPositionInfoSource::Error error;
};

core::GeoPositionInfoSource::GeoPositionInfoSource(QObject *parent)
        : QGeoPositionInfoSource(parent),
          m_applicationActive(true),
          m_lastReqTimeout(-1),
          m_state(State::stopped),
          d(new Private(this))
{
    d->timer.setSingleShot(true);
    QObject::connect(&d->timer, SIGNAL(timeout()), this, SLOT(timeout()), Qt::DirectConnection);
    // Whenever we receive an update, we stop the timeout timer immediately.
    QObject::connect(this, SIGNAL(positionUpdated(const QGeoPositionInfo&)), &d->timer, SLOT(stop()));
    QObject::connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(applicationStateChanged()));
    qRegisterMetaType<Qt::ApplicationState>("Qt::ApplicationState");
}

void core::GeoPositionInfoSource::applicationStateChanged()
{
    Qt::ApplicationState state = qApp->applicationState();
    if (state == Qt::ApplicationInactive) {
        if (m_applicationActive) {
            int state = m_state;
            stopUpdates();

            m_applicationActive = false;
            if (state == State::one_shot) {
                // Save current time out               
                if (d->timer.isActive()) {
                    m_lastReqTimeout = d->timer.interval();
                    d->timer.stop();
                }
            }
            else if (state == State::running) {
                // Stop continuous updates and suspend
                m_state = State::suspended;
            }
        }
    }
    else if (state == Qt::ApplicationActive) {
        if (!m_applicationActive) {
            m_applicationActive = true;

            // Only restart updates if active before suspending
            if (m_lastReqTimeout > -1) {
                requestUpdate(m_lastReqTimeout);
                m_lastReqTimeout = -1;
            }
            else if (m_state == State::suspended) {
                // Restart continuous updates
                startUpdates();
            }
        }
    }
}


core::GeoPositionInfoSource::~GeoPositionInfoSource()
{
}

void core::GeoPositionInfoSource::setUpdateInterval(int msec)
{
    // We emit our current error state whenever a caller tries to interact
    // with the source although we are in error state.
    if (error() != QGeoPositionInfoSource::NoError)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        Q_EMIT(QGeoPositionInfoSource::errorOccurred(d->error));
#else
        Q_EMIT(QGeoPositionInfoSource::error(d->error));
#endif
        return;
    }

    (void) msec;
}

void core::GeoPositionInfoSource::setPreferredPositioningMethods(PositioningMethods methods)
{
    // We emit our current error state whenever a caller tries to interact
    // with the source although we are in error state.
    if (error() != QGeoPositionInfoSource::NoError)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        Q_EMIT(QGeoPositionInfoSource::errorOccurred(d->error));
#else
        Q_EMIT(QGeoPositionInfoSource::error(d->error));
#endif
        return;
    }

    QGeoPositionInfoSource::setPreferredPositioningMethods(methods);
}

QGeoPositionInfo core::GeoPositionInfoSource::lastKnownPosition(bool fromSatellitePositioningMethodsOnly) const
{
    // We emit our current error state whenever a caller tries to interact
    // with the source although we are in error state.
    if (error() != QGeoPositionInfoSource::NoError)
        return QGeoPositionInfo();

    Q_UNUSED(fromSatellitePositioningMethodsOnly);
    QMutexLocker lock(&d->lastKnownPositionGuard);
    return QGeoPositionInfo(d->lastKnownPosition);
}

QGeoPositionInfoSource::PositioningMethods core::GeoPositionInfoSource::supportedPositioningMethods() const
{
    // We emit our current error state whenever a caller tries to interact
    // with the source although we are in error state.
    if (error() != QGeoPositionInfoSource::NoError)
    {
        return QGeoPositionInfoSource::NoPositioningMethods;
    }

    return AllPositioningMethods;
}

void core::GeoPositionInfoSource::startUpdates()
{
    if (d->session == nullptr) {
        d->createLocationServiceSession();
    }

    // We emit our current error state whenever a caller tries to interact
    // with the source although we are in error state.
    if (error() != QGeoPositionInfoSource::NoError)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        Q_EMIT(QGeoPositionInfoSource::errorOccurred(d->error));
#else
        Q_EMIT(QGeoPositionInfoSource::error(d->error));
#endif
        return;
    }

    d->session->updates().position_status.set(
                cllss::Interface::Updates::Status::enabled);

    d->session->updates().heading_status.set(
                cllss::Interface::Updates::Status::enabled);

    d->session->updates().velocity_status.set(
                cllss::Interface::Updates::Status::enabled);


    if (m_state != State::one_shot)
        m_state = State::running;
}


int core::GeoPositionInfoSource::minimumUpdateInterval() const {
    // We emit our current error state whenever a caller tries to interact
    // with the source although we are in error state.
    if (error() != QGeoPositionInfoSource::NoError)
    {
        return -1;
    }

    return 500;
}

void core::GeoPositionInfoSource::stopUpdates()
{
    if (error() != QGeoPositionInfoSource::NoError || !d->session)
    {
        // Don't emit an error from stopUpdates(). Applications usually call
        // stop() in response to an error, thus emitting one here could lead
        // to an infinite call cycle (and maybe stack overflow).
        return;
    }

    d->session->updates().position_status.set(
                cllss::Interface::Updates::Status::disabled);

    d->session->updates().heading_status.set(
                cllss::Interface::Updates::Status::disabled);

    d->session->updates().velocity_status.set(
                cllss::Interface::Updates::Status::disabled);

    m_state = State::stopped;
}

void core::GeoPositionInfoSource::requestUpdate(int timeout)
{
    // We emit our current error state whenever a caller tries to interact
    // with the source although we are in error state.
    if (error() != QGeoPositionInfoSource::NoError)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        Q_EMIT(QGeoPositionInfoSource::errorOccurred(d->error));
#else
        Q_EMIT(QGeoPositionInfoSource::error(d->error));
#endif
        return;
    }

    if (d->timer.isActive())
    {
        return;
    }

    // Bump the timeout if caller indicates "choose default value".
    if (timeout <= 0)
        timeout = Private::default_timeout_in_ms;

    startUpdates();
    d->timer.start(timeout);
}

void core::GeoPositionInfoSource::timeout()
{
    // Update timeout reached, clean up
    stopUpdates();

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    Q_EMIT(QGeoPositionInfoSource::errorOccurred(QGeoPositionInfoSource::UpdateTimeoutError));
#else
    Q_EMIT updateTimeout();
#endif
}

QGeoPositionInfoSource::Error core::GeoPositionInfoSource::error() const
{
    return d->error;
}

// Processes the incoming position update and translates it to Qt world.
void core::GeoPositionInfoSource::Private::handlePositionUpdate(const cll::Update<cll::Position>& position)
{
    QGeoCoordinate coord(
        position.value.latitude.value.value(),
        position.value.longitude.value.value(),
        position.value.altitude ? position.value.altitude->value.value() : 0);

    QMutexLocker lock(&lastKnownPositionGuard);

    lastKnownPosition.setCoordinate(coord);

    if (position.value.accuracy.horizontal)
    {
        double accuracy = position.value.accuracy.horizontal->value();
        if (!std::isnan(accuracy))
            lastKnownPosition.setAttribute(QGeoPositionInfo::HorizontalAccuracy, accuracy);
    }

    if (position.value.accuracy.vertical)
    {
        double accuracy = position.value.accuracy.vertical->value();
        if (!std::isnan(accuracy))
            lastKnownPosition.setAttribute(QGeoPositionInfo::VerticalAccuracy, accuracy);
    }

    lastKnownPosition.setTimestamp(
        QDateTime::fromMSecsSinceEpoch(
            std::chrono::duration_cast<std::chrono::milliseconds>(position.when.time_since_epoch()).count()));

    QGeoPositionInfo info{lastKnownPosition};

    QMetaObject::invokeMethod(
        parent,
        "positionUpdated",
        Qt::QueuedConnection,
        Q_ARG(QGeoPositionInfo, info));

    if (timer.isActive())
        timer.stop();

    if (parent->m_state == State::one_shot)
        parent->stopUpdates();
}

// Processes the incoming heading update and translates it to Qt world.
void core::GeoPositionInfoSource::Private::handleHeadingUpdate(const cll::Update<cll::Heading>& heading)
{
    QMutexLocker lock(&lastKnownPositionGuard);

    lastKnownPosition.setAttribute(
        QGeoPositionInfo::Direction,
        heading.value.value());

    lastKnownPosition.setTimestamp(
        QDateTime::fromMSecsSinceEpoch(
            std::chrono::duration_cast<std::chrono::milliseconds>(heading.when.time_since_epoch()).count()));

    QGeoPositionInfo info{lastKnownPosition};

    QMetaObject::invokeMethod(
        parent,
        "positionUpdated",
        Qt::QueuedConnection,
        Q_ARG(QGeoPositionInfo, info));
}

// Processes the incoming velocity update and translates it to Qt world.
void core::GeoPositionInfoSource::Private::handleVelocityUpdate(const cll::Update<cll::Velocity>& velocity)
{
    QMutexLocker lock(&lastKnownPositionGuard);

    lastKnownPosition.setAttribute(
        QGeoPositionInfo::GroundSpeed,
        velocity.value.value());

    lastKnownPosition.setTimestamp(
        QDateTime::fromMSecsSinceEpoch(
            std::chrono::duration_cast<std::chrono::milliseconds>(velocity.when.time_since_epoch()).count()));

    QGeoPositionInfo info{lastKnownPosition};

    QMetaObject::invokeMethod(
        parent,
        "positionUpdated",
        Qt::QueuedConnection,
        Q_ARG(QGeoPositionInfo, info));
}

void core::GeoPositionInfoSource::Private::createLocationServiceSession()
{
    try {
        session = instance()->getService()->create_session_for_criteria(cll::Criteria{});
        error = QGeoPositionInfoSource::NoError;
    } catch(...)
    {
        error = QGeoPositionInfoSource::AccessError;
        return;
    }

    session->updates().position.changed().connect(
        [this](const cll::Update<cll::Position>& new_position)
        {
            try
            {
                this->handlePositionUpdate(new_position);
            } catch(...)
            {
                // We silently ignore the issue and keep going.
            }
        });
    session->updates().heading.changed().connect(
        [this](const cll::Update<cll::Heading>& new_heading)
        {
            try
            {
                this->handleHeadingUpdate(new_heading);
            } catch(...)
            {
                // We silently ignore the issue and keep going.
            }
        });
    session->updates().velocity.changed().connect(
        [this](const cll::Update<cll::Velocity>& new_velocity)
        {
            try
            {
                this->handleVelocityUpdate(new_velocity);
            } catch(...)
            {
                // We silently ignore the issue and keep going.
            }
        });
}

void core::GeoPositionInfoSource::Private::destroyLocationServiceSession()
{
    if (session)
        session = nullptr;
}

// Creates a new instance and attempts to connect to the background service.
// Stores errors in the error member.
core::GeoPositionInfoSource::Private::Private(core::GeoPositionInfoSource* parent)
    : parent(parent),
      session(nullptr),
      error(QGeoPositionInfoSource::NoError)
{
    qRegisterMetaType<QGeoPositionInfo>("QGeoPositionInfo");
}

core::GeoPositionInfoSource::Private::~Private()
{
    destroyLocationServiceSession();
}
