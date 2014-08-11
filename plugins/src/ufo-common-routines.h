/*
* Copyright (C) 2011-2013 Karlsruhe Institute of Technology
*
* This file is part of Ufo.
*
* This library is free software: you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation, either
* version 3 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __UFO_COMMON_ROUTINES_H
#define __UFO_COMMON_ROUTINES_H

#include <glib.h>
#include <glib-object.h>
#include <ufo/ufo.h>

void warn_unimplemented (gpointer     object,
                         const gchar *func);

void error_unimplemented (gpointer     object,
                          const gchar *func);

#endif