/*
 * Copyright 2013 Canonical Ltd.
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
 */

#include "core_geo_position_info_source_factory.h"
#include "core_geo_position_info_source.h"

QGeoPositionInfoSource *core::GeoPositionInfoSourceFactory::positionInfoSource(QObject *parent
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    , const QVariantMap &parameters
#endif
)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    Q_UNUSED(parameters);
#endif
    core::GeoPositionInfoSource *src = new core::GeoPositionInfoSource(parent);
    return src;
}

QGeoSatelliteInfoSource *core::GeoPositionInfoSourceFactory::satelliteInfoSource(QObject *parent
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    , const QVariantMap &parameters
#endif
)
{
    Q_UNUSED(parent);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    Q_UNUSED(parameters);
#endif
    return 0;
}

QGeoAreaMonitorSource *core::GeoPositionInfoSourceFactory::areaMonitor(QObject *parent
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    , const QVariantMap &parameters
#endif
)
{
    Q_UNUSED(parent);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    Q_UNUSED(parameters);
#endif
    return 0;
}

