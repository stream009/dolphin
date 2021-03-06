/***************************************************************************
 *  Copyright (C) 2012 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com>*
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

#ifndef FOLDERSITEMLISTWIDGET_H
#define FOLDERSITEMLISTWIDGET_H

#include <kitemviews/kfileitemlistwidget.h>

/**
 * @brief Extends KFileItemListWidget to use the right text color.
*/
class FoldersItemListWidget : public KFileItemListWidget
{
    Q_OBJECT

public:
    FoldersItemListWidget(KItemListWidgetInformant* informant, QGraphicsItem* parent);
    ~FoldersItemListWidget() override;

protected:
    QPalette::ColorRole normalTextColorRole() const override;
};

#endif


