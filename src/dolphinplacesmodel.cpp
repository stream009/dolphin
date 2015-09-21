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
#include "dolphinplacesmodel.h"

#include <QModelIndex>
#include <QString>
#include <QUrl>

DolphinPlacesModel &DolphinPlacesModel::
instance()
{
    static DolphinPlacesModel theObject;

    return theObject;
}

QString DolphinPlacesModel::
text(const QUrl &url) const
{
    for (auto i = 0, cnt = this->rowCount(); i < cnt; ++i) {
        const auto &idx = this->index(i, 0);
        const auto &placeUrl = this->url(idx);

        if (placeUrl.matches(url, QUrl::StripTrailingSlash)) {
            return this->text(idx);
        }
    }

    return {};
}
