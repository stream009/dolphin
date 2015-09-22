/***************************************************************************
 * Copyright (C) 2015 by stream <stream009@gmail.com>                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/
#ifndef DOLPHIN_PLACES_MODEL_H
#define DOLPHIN_PLACES_MODEL_H

#include <KFilePlacesModel>

class QString;
class QUrl;

/*
 * Extension of KFilePlacesModel
 * - Add function that return place text for URL.
 * - Singleton
 *
 * TODO - We should merge this with PlacesItemModel.
 * TODO - Singleton stinks. I choose singleton only because it is convenient
 *        for patch, since it minimizes changes to existing code.
 */
class DolphinPlacesModel : public KFilePlacesModel
{
public:
    static DolphinPlacesModel &instance();

    using KFilePlacesModel::text;
    /*
     * Place text for given URL
     * If the URL isn't there in places, empty string will be returned.
     */
    QString text(const QUrl&) const;

private:
    DolphinPlacesModel() = default;
};

#endif // DOLPHIN_PLACES_MODEL_H
