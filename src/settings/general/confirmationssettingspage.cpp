/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "confirmationssettingspage.h"

#include <dolphin_generalsettings.h>

#include <KLocalizedString>

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

namespace {
    const bool ConfirmTrash = false;
    const bool ConfirmDelete = true;
    const bool ConfirmScriptExecution = true;
}

ConfirmationsSettingsPage::ConfirmationsSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_confirmMoveToTrash(0),
    m_confirmDelete(0),
    m_confirmClosingMultipleTabs(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    QLabel* confirmLabelKde = new QLabel(i18nc("@title:group", "Ask for confirmation in all KDE applications when:"), this);
    confirmLabelKde->setWordWrap(true);

    m_confirmMoveToTrash = new QCheckBox(i18nc("@option:check Ask for confirmation when",
                                               "Moving files or folders to trash"), this);
    m_confirmDelete = new QCheckBox(i18nc("@option:check Ask for confirmation when",
                                          "Deleting files or folders"), this);
    m_confirmScriptExecution = new QCheckBox(i18nc("@option:check Ask for confirmation when",
                                                   "Executing scripts or desktop files"), this);

    QLabel* confirmLabelDolphin = new QLabel(i18nc("@title:group", "Ask for confirmation when:"), this);
    confirmLabelDolphin->setWordWrap(true);

    m_confirmClosingMultipleTabs = new QCheckBox(i18nc("@option:check Ask for confirmation when",
                                                       "Closing Dolphin windows with multiple tabs"), this);

    topLayout->addWidget(confirmLabelKde);
    topLayout->addWidget(m_confirmMoveToTrash);
    topLayout->addWidget(m_confirmDelete);
    topLayout->addWidget(m_confirmScriptExecution);
    topLayout->addWidget(confirmLabelDolphin);
    topLayout->addWidget(m_confirmClosingMultipleTabs);
    topLayout->addStretch();

    loadSettings();

    connect(m_confirmMoveToTrash, &QCheckBox::toggled, this, &ConfirmationsSettingsPage::changed);
    connect(m_confirmDelete, &QCheckBox::toggled, this, &ConfirmationsSettingsPage::changed);
    connect(m_confirmScriptExecution, &QCheckBox::toggled, this, &ConfirmationsSettingsPage::changed);
    connect(m_confirmClosingMultipleTabs, &QCheckBox::toggled, this, &ConfirmationsSettingsPage::changed);
}

ConfirmationsSettingsPage::~ConfirmationsSettingsPage()
{
}

void ConfirmationsSettingsPage::applySettings()
{
    KSharedConfig::Ptr kioConfig = KSharedConfig::openConfig(QStringLiteral("kiorc"), KConfig::NoGlobals);
    KConfigGroup confirmationGroup(kioConfig, "Confirmations");
    confirmationGroup.writeEntry("ConfirmTrash", m_confirmMoveToTrash->isChecked());
    confirmationGroup.writeEntry("ConfirmDelete", m_confirmDelete->isChecked());
    confirmationGroup.sync();

    if (m_confirmScriptExecution->isChecked()) {
        KConfigGroup scriptExecutionGroup(kioConfig, "Executable scripts");
        scriptExecutionGroup.writeEntry("behaviourOnLaunch", "alwaysAsk");
        scriptExecutionGroup.sync();
    }

    GeneralSettings* settings = GeneralSettings::self();
    settings->setConfirmClosingMultipleTabs(m_confirmClosingMultipleTabs->isChecked());
    settings->save();
}

void ConfirmationsSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);

    m_confirmMoveToTrash->setChecked(ConfirmTrash);
    m_confirmDelete->setChecked(ConfirmDelete);
    m_confirmScriptExecution->setChecked(ConfirmScriptExecution);
}

void ConfirmationsSettingsPage::loadSettings()
{
    KSharedConfig::Ptr kioConfig = KSharedConfig::openConfig(QStringLiteral("kiorc"), KConfig::IncludeGlobals);
    const KConfigGroup confirmationGroup(kioConfig, "Confirmations");
    m_confirmMoveToTrash->setChecked(confirmationGroup.readEntry("ConfirmTrash", ConfirmTrash));
    m_confirmDelete->setChecked(confirmationGroup.readEntry("ConfirmDelete", ConfirmDelete));

    const KConfigGroup scriptExecutionGroup(KSharedConfig::openConfig(QStringLiteral("kiorc")), "Executable scripts");
    const QString value = scriptExecutionGroup.readEntry("behaviourOnLaunch", "alwaysAsk");
    m_confirmScriptExecution->setChecked(value == QLatin1String("alwaysAsk"));

    m_confirmClosingMultipleTabs->setChecked(GeneralSettings::confirmClosingMultipleTabs());
}

