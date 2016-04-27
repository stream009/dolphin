/***************************************************************************
*    Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>            *
*                                                                         *
*    This program is free software; you can redistribute it and/or modify *
*    it under the terms of the GNU General Public License as published by *
*    the Free Software Foundation; either version 2 of the License, or    *
*    (at your option) any later version.                                  *
*                                                                         *
*    This program is distributed in the hope that it will be useful,      *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
*    GNU General Public License for more details.                         *
*                                                                         *
*    You should have received a copy of the GNU General Public License    *
*    along with this program; if not, write to the                        *
*    Free Software Foundation, Inc.,                                      *
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA           *
* **************************************************************************/

#include "dolphinsearchbox.h"

#include "dolphin_searchsettings.h"
#include "dolphinfacetswidget.h"

#include <QIcon>
#include <QLineEdit>
#include <KLocalizedString>
#include <KSeparator>

#include <QButtonGroup>
#include <QDir>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QUrlQuery>

#include <config-baloo.h>
#ifdef HAVE_BALOO
    #include <Baloo/Query>
    #include <Baloo/IndexerConfig>
#endif
#include <QFontDatabase>

DolphinSearchBox::DolphinSearchBox(QWidget* parent) :
    QWidget(parent),
    m_startedSearching(false),
    m_active(true),
    m_topLayout(0),
    m_searchLabel(0),
    m_searchInput(0),
    m_optionsScrollArea(0),
    m_fileNameButton(0),
    m_contentButton(0),
    m_separator(0),
    m_fromHereButton(0),
    m_everywhereButton(0),
    m_facetsToggleButton(0),
    m_facetsWidget(0),
    m_searchPath(),
    m_startSearchTimer(0)
{
}

DolphinSearchBox::~DolphinSearchBox()
{
    saveSettings();
}

void DolphinSearchBox::setText(const QString& text)
{
    m_searchInput->setText(text);
}

QString DolphinSearchBox::text() const
{
    return m_searchInput->text();
}

void DolphinSearchBox::setSearchPath(const QUrl& url)
{
    m_searchPath = url;

    QFontMetrics metrics(m_fromHereButton->font());
    const int maxWidth = metrics.height() * 8;

    QString location = url.fileName();
    if (location.isEmpty()) {
        if (url.isLocalFile()) {
            location = QStringLiteral("/");
        } else {
            location = url.scheme() + QLatin1String(" - ") + url.host();
        }
    }

    const QString elidedLocation = metrics.elidedText(location, Qt::ElideMiddle, maxWidth);
    m_fromHereButton->setText(i18nc("action:button", "From Here (%1)", elidedLocation));

    const bool showSearchFromButtons = url.isLocalFile();
    m_separator->setVisible(showSearchFromButtons);
    m_fromHereButton->setVisible(showSearchFromButtons);
    m_everywhereButton->setVisible(showSearchFromButtons);

    bool hasFacetsSupport = false;
#ifdef HAVE_BALOO
    const Baloo::IndexerConfig searchInfo;
    hasFacetsSupport = searchInfo.fileIndexingEnabled() && searchInfo.shouldBeIndexed(m_searchPath.toLocalFile());
#endif
    m_facetsWidget->setEnabled(hasFacetsSupport);
}

QUrl DolphinSearchBox::searchPath() const
{
    return m_searchPath;
}

QUrl DolphinSearchBox::urlForSearching() const
{
    QUrl url;
    bool useBalooSearch = false;
#ifdef HAVE_BALOO
    const Baloo::IndexerConfig searchInfo;
    useBalooSearch = searchInfo.fileIndexingEnabled() && searchInfo.shouldBeIndexed(m_searchPath.toLocalFile());
#endif
    if (useBalooSearch) {
        url = balooUrlForSearching();
    } else {
        url.setScheme(QStringLiteral("filenamesearch"));

        QUrlQuery query;
        query.addQueryItem(QStringLiteral("search"), m_searchInput->text());
        if (m_contentButton->isChecked()) {
            query.addQueryItem(QStringLiteral("checkContent"), QStringLiteral("yes"));
        }

        QString encodedUrl;
        if (m_everywhereButton->isChecked()) {
            // It is very unlikely, that the majority of Dolphins target users
            // mean "the whole harddisk" instead of "my home folder" when
            // selecting the "Everywhere" button.
            encodedUrl = QDir::homePath();
        } else {
            encodedUrl = m_searchPath.url();
        }
        query.addQueryItem(QStringLiteral("url"), encodedUrl);

        url.setQuery(query);
    }

    return url;
}

void DolphinSearchBox::fromSearchUrl(const QUrl& url)
{
    if (url.scheme() == QLatin1String("baloosearch")) {
        fromBalooSearchUrl(url);
    } else if (url.scheme() == QLatin1String("filenamesearch")) {
        const QUrlQuery query(url);
        setText(query.queryItemValue(QStringLiteral("search")));
        setSearchPath(QUrl::fromUserInput(query.queryItemValue(QStringLiteral("url")), QString(), QUrl::AssumeLocalFile));
        m_contentButton->setChecked(query.queryItemValue(QStringLiteral("checkContent")) == QLatin1String("yes"));
    } else {
        setText(QString());
        setSearchPath(url);
    }
}

void DolphinSearchBox::selectAll()
{
    m_searchInput->selectAll();
}

void DolphinSearchBox::setActive(bool active)
{
    if (active != m_active) {
        m_active = active;

        if (active) {
            emit activated();
        }
    }
}

bool DolphinSearchBox::isActive() const
{
    return m_active;
}

bool DolphinSearchBox::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        init();
    }
    return QWidget::event(event);
}

void DolphinSearchBox::showEvent(QShowEvent* event)
{
    if (!event->spontaneous()) {
        m_searchInput->setFocus();
        m_startedSearching = false;
    }
}

void DolphinSearchBox::keyReleaseEvent(QKeyEvent* event)
{
    QWidget::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Escape) {
        if (m_searchInput->text().isEmpty()) {
            emit closeRequest();
        } else {
            m_searchInput->clear();
        }
    }
}

bool DolphinSearchBox::eventFilter(QObject* obj, QEvent* event)
{
    switch (event->type()) {
    case QEvent::FocusIn:
        setActive(true);
        setFocus();
        break;

    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}

void DolphinSearchBox::emitSearchRequest()
{
    m_startSearchTimer->stop();
    m_startedSearching = true;
    emit searchRequest();
}

void DolphinSearchBox::emitCloseRequest()
{
    m_startSearchTimer->stop();
    m_startedSearching = false;
    emit closeRequest();
}

void DolphinSearchBox::slotConfigurationChanged()
{
    saveSettings();
    if (m_startedSearching) {
        emitSearchRequest();
    }
}

void DolphinSearchBox::slotSearchTextChanged(const QString& text)
{

    if (text.isEmpty()) {
        m_startSearchTimer->stop();
    } else {
        m_startSearchTimer->start();
    }
    emit searchTextChanged(text);
}

void DolphinSearchBox::slotReturnPressed()
{
    emitSearchRequest();
    emit returnPressed();
}

void DolphinSearchBox::slotFacetsButtonToggled()
{
    const bool facetsIsVisible = !m_facetsWidget->isVisible();
    m_facetsWidget->setVisible(facetsIsVisible);
    updateFacetsToggleButton();
}

void DolphinSearchBox::slotFacetChanged()
{
    m_startedSearching = true;
    m_startSearchTimer->stop();
    emit searchRequest();
}

void DolphinSearchBox::initButton(QToolButton* button)
{
    button->installEventFilter(this);
    button->setAutoExclusive(true);
    button->setAutoRaise(true);
    button->setCheckable(true);
    connect(button, &QToolButton::clicked, this, &DolphinSearchBox::slotConfigurationChanged);
}

void DolphinSearchBox::loadSettings()
{
    if (SearchSettings::location() == QLatin1String("Everywhere")) {
        m_everywhereButton->setChecked(true);
    } else {
        m_fromHereButton->setChecked(true);
    }

    if (SearchSettings::what() == QLatin1String("Content")) {
        m_contentButton->setChecked(true);
    } else {
        m_fileNameButton->setChecked(true);
    }

    m_facetsWidget->setVisible(SearchSettings::showFacetsWidget());
}

void DolphinSearchBox::saveSettings()
{
    SearchSettings::setLocation(m_fromHereButton->isChecked() ? QStringLiteral("FromHere") : QStringLiteral("Everywhere"));
    SearchSettings::setWhat(m_fileNameButton->isChecked() ? QStringLiteral("FileName") : QStringLiteral("Content"));
    SearchSettings::setShowFacetsWidget(m_facetsToggleButton->isChecked());
    SearchSettings::self()->save();
}

void DolphinSearchBox::init()
{
    // Create close button
    QToolButton* closeButton = new QToolButton(this);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    closeButton->setToolTip(i18nc("@info:tooltip", "Quit searching"));
    connect(closeButton, &QToolButton::clicked, this, &DolphinSearchBox::emitCloseRequest);

    // Create search label
    m_searchLabel = new QLabel(this);

    // Create search box
    m_searchInput = new QLineEdit(this);
    m_searchInput->installEventFilter(this);
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    connect(m_searchInput, &QLineEdit::returnPressed,
            this, &DolphinSearchBox::slotReturnPressed);
    connect(m_searchInput, &QLineEdit::textChanged,
            this, &DolphinSearchBox::slotSearchTextChanged);
    setFocusProxy(m_searchInput);

    // Apply layout for the search input
    QHBoxLayout* searchInputLayout = new QHBoxLayout();
    searchInputLayout->setMargin(0);
    searchInputLayout->addWidget(closeButton);
    searchInputLayout->addWidget(m_searchLabel);
    searchInputLayout->addWidget(m_searchInput);

    // Create "Filename" and "Content" button
    m_fileNameButton = new QToolButton(this);
    m_fileNameButton->setText(i18nc("action:button", "Filename"));
    initButton(m_fileNameButton);

    m_contentButton = new QToolButton();
    m_contentButton->setText(i18nc("action:button", "Content"));
    initButton(m_contentButton);

    QButtonGroup* searchWhatGroup = new QButtonGroup(this);
    searchWhatGroup->addButton(m_fileNameButton);
    searchWhatGroup->addButton(m_contentButton);

    m_separator = new KSeparator(Qt::Vertical, this);

    // Create "From Here" and "Everywhere"button
    m_fromHereButton = new QToolButton(this);
    m_fromHereButton->setText(i18nc("action:button", "From Here"));
    initButton(m_fromHereButton);

    m_everywhereButton = new QToolButton(this);
    m_everywhereButton->setText(i18nc("action:button", "Everywhere"));
    initButton(m_everywhereButton);

    QButtonGroup* searchLocationGroup = new QButtonGroup(this);
    searchLocationGroup->addButton(m_fromHereButton);
    searchLocationGroup->addButton(m_everywhereButton);

    // Create "Facets" widgets
    m_facetsToggleButton = new QToolButton(this);
    m_facetsToggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    initButton(m_facetsToggleButton);
    connect(m_facetsToggleButton, &QToolButton::clicked, this, &DolphinSearchBox::slotFacetsButtonToggled);

    m_facetsWidget = new DolphinFacetsWidget(this);
    m_facetsWidget->installEventFilter(this);
    m_facetsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    connect(m_facetsWidget, &DolphinFacetsWidget::facetChanged, this, &DolphinSearchBox::slotFacetChanged);

    // Apply layout for the options
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    optionsLayout->setMargin(0);
    optionsLayout->addWidget(m_fileNameButton);
    optionsLayout->addWidget(m_contentButton);
    optionsLayout->addWidget(m_separator);
    optionsLayout->addWidget(m_fromHereButton);
    optionsLayout->addWidget(m_everywhereButton);
    optionsLayout->addStretch(1);
    optionsLayout->addWidget(m_facetsToggleButton);

    // Put the options into a QScrollArea. This prevents increasing the view width
    // in case that not enough width for the options is available.
    QWidget* optionsContainer = new QWidget(this);
    optionsContainer->setLayout(optionsLayout);

    m_optionsScrollArea = new QScrollArea(this);
    m_optionsScrollArea->setFrameShape(QFrame::NoFrame);
    m_optionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_optionsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_optionsScrollArea->setMaximumHeight(optionsContainer->sizeHint().height());
    m_optionsScrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_optionsScrollArea->setWidget(optionsContainer);
    m_optionsScrollArea->setWidgetResizable(true);

    m_topLayout = new QVBoxLayout(this);
    m_topLayout->setMargin(0);
    m_topLayout->addLayout(searchInputLayout);
    m_topLayout->addWidget(m_optionsScrollArea);
    m_topLayout->addWidget(m_facetsWidget);

    loadSettings();

    // The searching should be started automatically after the user did not change
    // the text within one second
    m_startSearchTimer = new QTimer(this);
    m_startSearchTimer->setSingleShot(true);
    m_startSearchTimer->setInterval(1000);
    connect(m_startSearchTimer, &QTimer::timeout, this, &DolphinSearchBox::emitSearchRequest);

    updateFacetsToggleButton();
}

QUrl DolphinSearchBox::balooUrlForSearching() const
{
#ifdef HAVE_BALOO
    const QString text = m_searchInput->text();

    Baloo::Query query;
    query.addType(m_facetsWidget->facetType());

    QStringList queryStrings;
    QString ratingQuery = m_facetsWidget->ratingTerm();
    if (!ratingQuery.isEmpty()) {
        queryStrings << ratingQuery;
    }

    if (m_contentButton->isChecked()) {
        queryStrings << text;
    } else if (!text.isEmpty()) {
        queryStrings << QStringLiteral("filename:\"%1\"").arg(text);
    }

    if (m_fromHereButton->isChecked()) {
        query.setIncludeFolder(m_searchPath.toLocalFile());
    }

    query.setSearchString(queryStrings.join(QStringLiteral(" ")));

    return query.toSearchUrl(i18nc("@title UDS_DISPLAY_NAME for a KIO directory listing. %1 is the query the user entered.",
                                   "Query Results from '%1'", text));
#else
    return QUrl();
#endif
}

void DolphinSearchBox::fromBalooSearchUrl(const QUrl& url)
{
#ifdef HAVE_BALOO
    const Baloo::Query query = Baloo::Query::fromSearchUrl(url);

    // Block all signals to avoid unnecessary "searchRequest" signals
    // while we adjust the search text and the facet widget.
    blockSignals(true);

    const QString customDir = query.includeFolder();
    if (!customDir.isEmpty()) {
        setSearchPath(QUrl::fromLocalFile(customDir));
    } else {
        setSearchPath(QUrl::fromLocalFile(QDir::homePath()));
    }

    setText(query.searchString());

    QStringList types = query.types();
    if (!types.isEmpty()) {
        m_facetsWidget->setFacetType(types.first());
    }

    const QStringList subTerms = query.searchString().split(' ', QString::SkipEmptyParts);
    foreach (const QString& subTerm, subTerms) {
        if (subTerm.startsWith(QLatin1String("filename:"))) {
            const QString value = subTerm.mid(9);
            setText(value);
        } else if (m_facetsWidget->isRatingTerm(subTerm)) {
            m_facetsWidget->setRatingTerm(subTerm);
        }
    }

    m_startSearchTimer->stop();
    blockSignals(false);
#else
    Q_UNUSED(url);
#endif
}

void DolphinSearchBox::updateFacetsToggleButton()
{
    const bool facetsIsVisible = SearchSettings::showFacetsWidget();
    m_facetsToggleButton->setChecked(facetsIsVisible ? true : false);
    m_facetsToggleButton->setIcon(QIcon::fromTheme(facetsIsVisible ? QStringLiteral("arrow-up-double") : QStringLiteral("arrow-down-double")));
    m_facetsToggleButton->setText(facetsIsVisible ? i18nc("action:button", "Fewer Options") : i18nc("action:button", "More Options"));
}

