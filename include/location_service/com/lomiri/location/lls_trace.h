/*
 * Copyright © 2012-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef COM_LOMIRI_LOCATION_LLS_TRACE_H
#define COM_LOMIRI_LOCATION_LLS_TRACE_H

#include <cstdio>

// Set to true to enable LLS debug traces on stderr.
static constexpr bool LLS_DEBUG = false;

#define LLS_TRACE(...) do { if (LLS_DEBUG) fprintf(stderr, __VA_ARGS__); } while(0)

#endif // COM_LOMIRI_LOCATION_LLS_TRACE_H
