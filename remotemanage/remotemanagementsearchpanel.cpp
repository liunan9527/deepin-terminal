#include "remotemanagementsearchpanel.h"
#include "serverconfigmanager.h"

RemoteManagementSearchPanel::RemoteManagementSearchPanel(QWidget *parent) : CommonPanel(parent)
{
    initUI();
}

void RemoteManagementSearchPanel::initUI()
{
    this->setBackgroundRole(QPalette::Base);
    this->setAutoFillBackground(true);

    m_backButton = new DIconButton(this);
    m_backButton->setIcon(DStyle::StandardPixmap::SP_ArrowLeave);
    m_backButton->setFixedSize(QSize(40, 40));

    m_listWidget = new ServerConfigList(this);
    m_label = new DLabel(this);
    m_label->setAlignment(Qt::AlignCenter);

    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerItem);
    m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addWidget(m_backButton);
    hlayout->addWidget(m_label);
    hlayout->setSpacing(0);
    hlayout->setMargin(0);

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addLayout(hlayout);
    vlayout->addWidget(m_listWidget);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);
    setLayout(vlayout);

    connect(m_backButton, &DIconButton::clicked, this, &RemoteManagementSearchPanel::showPreviousPanel);  //
    connect(m_listWidget, &ServerConfigList::itemClicked, this, &RemoteManagementSearchPanel::listItemClicked);
}

void RemoteManagementSearchPanel::refreshDataByGroupAndFilter(const QString &strGroup, const QString &strFilter)
{
    setSearchFilter(strFilter);
    m_strGroupName = strGroup;
    m_strFilter = strFilter;
    m_listWidget->clearData();
    m_listWidget->refreshDataByGroupAndFilter(strGroup, strFilter);
}

void RemoteManagementSearchPanel::refreshDataByFilter(const QString &strFilter)
{
    setSearchFilter(strFilter);
    m_listWidget->clearData();
    m_listWidget->refreshDataByFilter(strFilter);
}
void RemoteManagementSearchPanel::showPreviousPanel()
{
    if (m_previousPanel == REMOTE_MANAGEMENT_PANEL) {
        emit showRemoteManagementPanel();
    }
    if (m_previousPanel == REMOTE_MANAGEMENT_GROUP) {
        emit showServerConfigGroupPanel(m_strGroupName);
    }
}

void RemoteManagementSearchPanel::setPreviousPanelType(RemoteManagementPanelType type)
{
    m_previousPanel = type;
}

void RemoteManagementSearchPanel::listItemClicked(ServerConfig *curItemServer)
{
    if (nullptr != curItemServer) {
        QString group = curItemServer->m_group;
        if (!group.isNull() && !group.isEmpty()) {
            emit showServerConfigGroupPanel(group);
        } else {
            emit doConnectServer(curItemServer);
        }
    }
}

void RemoteManagementSearchPanel::setSearchFilter(const QString &filter)
{
    m_strFilter = filter;
    m_label->setText(QString("搜索：%1").arg(filter));
}