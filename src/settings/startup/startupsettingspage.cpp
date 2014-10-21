/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "startupsettingspage.h"

#include "dolphinmainwindow.h"
#include "dolphinviewcontainer.h"

#include "dolphin_generalsettings.h"

#include <KDialog>
#include <KFileDialog>
#include <KLocalizedString>
#include <KLineEdit>
#include <KMessageBox>
#include <KVBox>

#include <QVBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include "views/dolphinview.h"

StartupSettingsPage::StartupSettingsPage(const QUrl& url, QWidget* parent) :
    SettingsPageBase(parent),
    m_url(url),
    m_homeUrl(0),
    m_splitView(0),
    m_editableUrl(0),
    m_showFullPath(0),
    m_filterBar(0)
{
    const int spacing = KDialog::spacingHint();

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(spacing);

    // create 'Home URL' editor
    QGroupBox* homeBox = new QGroupBox(i18nc("@title:group", "Home Folder"), vBox);

    KHBox* homeUrlBox = new KHBox(homeBox);
    homeUrlBox->setSpacing(spacing);

    new QLabel(i18nc("@label:textbox", "Location:"), homeUrlBox);
    m_homeUrl = new KLineEdit(homeUrlBox);
    m_homeUrl->setClearButtonShown(true);

    QPushButton* selectHomeUrlButton = new QPushButton(QIcon::fromTheme("folder-open"), QString(), homeUrlBox);

#ifndef QT_NO_ACCESSIBILITY
    selectHomeUrlButton->setAccessibleName(i18nc("@action:button", "Select Home Location"));
#endif

    connect(selectHomeUrlButton, &QPushButton::clicked,
            this, &StartupSettingsPage::selectHomeUrl);

    KHBox* buttonBox = new KHBox(homeBox);
    buttonBox->setSpacing(spacing);

    QPushButton* useCurrentButton = new QPushButton(i18nc("@action:button", "Use Current Location"), buttonBox);
    connect(useCurrentButton, &QPushButton::clicked,
            this, &StartupSettingsPage::useCurrentLocation);
    QPushButton* useDefaultButton = new QPushButton(i18nc("@action:button", "Use Default Location"), buttonBox);
    connect(useDefaultButton, &QPushButton::clicked,
            this, &StartupSettingsPage::useDefaultLocation);

    QVBoxLayout* homeBoxLayout = new QVBoxLayout(homeBox);
    homeBoxLayout->addWidget(homeUrlBox);
    homeBoxLayout->addWidget(buttonBox);

    // create 'Split view', 'Show full path', 'Editable location' and 'Filter bar' checkboxes
    m_splitView = new QCheckBox(i18nc("@option:check Startup Settings", "Split view mode"), vBox);
    m_editableUrl = new QCheckBox(i18nc("@option:check Startup Settings", "Editable location bar"), vBox);
    m_showFullPath = new QCheckBox(i18nc("@option:check Startup Settings", "Show full path inside location bar"), vBox);
    m_filterBar = new QCheckBox(i18nc("@option:check Startup Settings", "Show filter bar"), vBox);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();

    connect(m_homeUrl, &KLineEdit::textChanged, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_splitView,    &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_editableUrl,  &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_showFullPath, &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
    connect(m_filterBar,    &QCheckBox::toggled, this, &StartupSettingsPage::slotSettingsChanged);
}

StartupSettingsPage::~StartupSettingsPage()
{
}

void StartupSettingsPage::applySettings()
{
    GeneralSettings* settings = GeneralSettings::self();

    const QUrl url(QUrl::fromLocalFile(m_homeUrl->text()));
    KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, url);
    if ((url.isValid() && fileItem.isDir()) || (url.scheme() == QLatin1String("timeline"))) {
        settings->setHomeUrl(url.toDisplayString(QUrl::PreferLocalFile));
    } else {
        KMessageBox::error(this, i18nc("@info", "The location for the home folder is invalid or does not exist, it will not be applied."));
    }

    settings->setSplitView(m_splitView->isChecked());
    settings->setEditableUrl(m_editableUrl->isChecked());
    settings->setShowFullPath(m_showFullPath->isChecked());
    settings->setFilterBar(m_filterBar->isChecked());

    settings->writeConfig();
}

void StartupSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void StartupSettingsPage::slotSettingsChanged()
{
    // Provide a hint that the startup settings have been changed. This allows the views
    // to apply the startup settings only if they have been explicitly changed by the user
    // (see bug #254947).
    GeneralSettings::setModifiedStartupSettings(true);
    emit changed();
}

void StartupSettingsPage::selectHomeUrl()
{
    const QString homeUrl = m_homeUrl->text();
    QUrl url = KFileDialog::getExistingDirectoryUrl(QUrl::fromLocalFile(homeUrl), this);
    if (!url.isEmpty()) {
        m_homeUrl->setText(url.toDisplayString(QUrl::PreferLocalFile));
        slotSettingsChanged();
    }
}

void StartupSettingsPage::useCurrentLocation()
{
    m_homeUrl->setText(m_url.toDisplayString(QUrl::PreferLocalFile));
}

void StartupSettingsPage::useDefaultLocation()
{
    m_homeUrl->setText(QDir::homePath());
}

void StartupSettingsPage::loadSettings()
{
    const QUrl url(QUrl::fromLocalFile(GeneralSettings::homeUrl()));
    m_homeUrl->setText(url.toDisplayString(QUrl::PreferLocalFile));
    m_splitView->setChecked(GeneralSettings::splitView());
    m_editableUrl->setChecked(GeneralSettings::editableUrl());
    m_showFullPath->setChecked(GeneralSettings::showFullPath());
    m_filterBar->setChecked(GeneralSettings::filterBar());
}
