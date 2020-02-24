#include "termwidget.h"
#include "define.h"
#include "settings.h"
#include "termproperties.h"

#include <DDesktopServices>
#include <DInputDialog>

#include <QApplication>
#include <QDesktopServices>
#include <QMenu>
#include <QVBoxLayout>
#include <QTime>
#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include <DApplicationHelper>

DWIDGET_USE_NAMESPACE
using namespace Konsole;
TermWidget::TermWidget(TermProperties properties, QWidget *parent, QWidget *grandma) : QTermWidget(0, parent)
{
    m_Page = (void *)grandma;
    setContextMenuPolicy(Qt::CustomContextMenu);

    setHistorySize(5000);

    // set shell program
    QString shell{ getenv("SHELL") };
    setShellProgram(shell.isEmpty() ? "/bin/bash" : shell);
    setTerminalOpacity(Settings::instance()->opacity());
    setScrollBarPosition(QTermWidget::ScrollBarRight);

    /******** Modify by n014361 wangpeili 2020-01-13:              ****************/
    // theme
    setColorScheme(Settings::instance()->colorScheme());
    if (properties.contains(WorkingDir)) {
        setWorkingDirectory(properties[WorkingDir].toString());
    }

    if (properties.contains(Execute)) {
        QString args = properties[Execute].toString();
        qDebug() << args;
        QStringList argList = args.split(QRegExp(QStringLiteral("\\s+")), QString::SkipEmptyParts);
        if (argList.count() > 0) {
            setShellProgram(argList.at(0));
            argList.removeAt(0);
            if (argList.count()) {
                setArgs(argList);
            }
        }
    }

    // 字体和字体大小
    QFont font = getTerminalFont();
    font.setFamily(Settings::instance()->fontName());
    font.setPointSize(Settings::instance()->fontSize());
    setTerminalFont(font);

    // 光标形状
    setKeyboardCursorShape(static_cast<QTermWidget::KeyboardCursorShape>(Settings::instance()->cursorShape()));
    // 光标闪烁
    setBlinkingCursor(Settings::instance()->cursorBlink());

    // 按键滚动
    if (Settings::instance()->PressingScroll()) {
        qDebug() << "setMotionAfterPasting(2)";
        setMotionAfterPasting(2);
    } else {
        setMotionAfterPasting(0);
        qDebug() << "setMotionAfterPasting(0)";
    }

    // 输出滚动，会在每个输出判断是否设置了滚动，即时设置
    connect(this, &QTermWidget::receivedData, this, [this](QString value) {
        setTrackOutput(Settings::instance()->OutputtingScroll());
    });

#if !(QTERMWIDGET_VERSION <= QT_VERSION_CHECK(0, 7, 1))
    setBlinkingCursor(Settings::instance()->cursorBlink());
#endif  // !(QTERMWIDGET_VERSION <= QT_VERSION_CHECK(0, 7, 1))

    connect(this, &QTermWidget::urlActivated, this, [](const QUrl & url, bool fromContextMenu) {
        if (QApplication::keyboardModifiers() & Qt::ControlModifier || fromContextMenu) {
            QDesktopServices::openUrl(url);
        }
    });

    connect(this, &QWidget::customContextMenuRequested, this, &TermWidget::customContextMenuCall);

    connect(DApplicationHelper::instance(),
            &DApplicationHelper::themeTypeChanged,
            this,
    [ = ](DGuiApplicationHelper::ColorType builtInTheme) {
        qDebug() << "themeChanged" << builtInTheme;
        // ThemePanelPlugin *plugin = qobject_cast<ThemePanelPlugin *>(getPluginByName("Theme"));
        QString theme = "GreenOnBlack";
        if (builtInTheme == DGuiApplicationHelper::LightType) {
            theme = "BlackOnWhite";
        }
        setColorScheme(theme);
        Settings::instance()->setColorScheme(theme);
    });
    /********************* Modify by n014361 wangpeili End ************************/

    startShellProgram();
}

void TermWidget::customContextMenuCall(const QPoint &pos)
{
    QMenu menu;

    QList<QAction *> termActions = filterActions(pos);
    for (QAction *&action : termActions) {
        menu.addAction(action);
    }

    if (!menu.isEmpty()) {
        menu.addSeparator();
    }

    // add other actions here.
    if (!selectedText().isEmpty()) {

        menu.addAction(tr("Copy &Selection"), this, [this] { copyClipboard(); });
    }

    menu.addAction(tr("&Paste"), this, [this] { pasteClipboard(); });

    menu.addAction(
        tr("&Open File Manager"), this, [this] { DDesktopServices::showFolder(QUrl::fromLocalFile(workingDirectory())); });

    menu.addSeparator();

    menu.addAction(tr("Split &Horizontally"), this, [this] { emit termRequestSplit(Qt::Horizontal); });

    menu.addAction(tr("Split &Vertically"), this, [this] { emit termRequestSplit(Qt::Vertical); });

    /******** Modify by n014361 wangpeili 2020-02-21: 增加关闭窗口和关闭其它窗口菜单    ****************/
    menu.addAction(tr("Close &Window"), this, [this] { ((TermWidgetPage *)m_Page)->currentTerminal()->close(); });
    if (((TermWidgetPage *)m_Page)->getTerminalCount() > 1) {
        menu.addAction(
            tr("Close &Other &Window"), this, [this] { ((TermWidgetPage *)m_Page)->closeOtherTerminal(); });
    };

    /********************* Modify by n014361 wangpeili End ************************/

    menu.addSeparator();

    bool isFullScreen = this->window()->windowState().testFlag(Qt::WindowFullScreen);
    if (isFullScreen) {
        menu.addAction(
            tr("Exit Full&screen"), this, [this] { window()->setWindowState(windowState() & ~Qt::WindowFullScreen); });
    } else {
        menu.addAction(
            tr("Full&screen"), this, [this] { window()->setWindowState(windowState() | Qt::WindowFullScreen); });
    }

    menu.addAction(tr("&Find"), this, [this] {
        // this is not a DTK look'n'feel search bar, but nah.
        toggleShowSearchBar();
    });

    menu.addSeparator();

    menu.addAction(tr("Rename Tab"), this, [this] {
        // TODO: Tab name as default text?
        bool ok;
        QString text =
        DInputDialog::getText(nullptr, tr("Rename Tab"), tr("Tab name:"), QLineEdit::Normal, QString(), &ok);
        if (ok)
        {
            emit termRequestRenameTab(text);
        }
    });

    menu.addAction(tr("Custom Command"), this, [this] {
        // TODO: Tab name as default text?
        emit termRequestOpenCustomCommand();
    });

    menu.addAction(tr("RemoteManagement"), this, [this] { emit termRequestOpenRemoteManagement(); });

    if (isInRemoteServer()) {
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("upload-file"), tr("Upload File"), this, [this] {
            emit termRequestUploadFile();
        });
        menu.addAction(QIcon::fromTheme("download-file"), tr("Download File"), this, [this] {
            emit termRequestDownloadFile();
        });
    }

    menu.addSeparator();

    menu.addAction(tr("Settings"), this, [this] { emit termRequestOpenSettings(); });

    // display the menu.
    menu.exec(mapToGlobal(pos));
}

bool TermWidget::isInRemoteServer()
{
    int pid = getForegroundProcessId();
    if (pid <= 0) {
        return false;
    }
    QString pidFilepath = "/proc/" + QString::number(pid) + "/comm";
    QFile pidFile(pidFilepath);
    if (pidFile.exists()) {
        pidFile.open(QIODevice::ReadOnly | QIODevice::Text);
        QString commString(pidFile.readLine());
        pidFile.close();
        if ("expect" == commString.trimmed()) {
            return true;
        }
    }
    return false;
}

TermWidgetWrapper::TermWidgetWrapper(TermProperties properties, QWidget *parent)
    : QWidget(parent), m_term(new TermWidget(properties, this, parent))
{
    initUI();

    connect(m_term, &QTermWidget::titleChanged, this, [this] { emit termTitleChanged(m_term->title()); });
    connect(m_term, &QTermWidget::finished, this, &TermWidgetWrapper::termClosed);
    // proxy signal:
    connect(m_term, &QTermWidget::termGetFocus, this, &TermWidgetWrapper::termGetFocus);
    connect(m_term, &TermWidget::termRequestSplit, this, &TermWidgetWrapper::termRequestSplit);
    connect(m_term, &TermWidget::termRequestRenameTab, this, &TermWidgetWrapper::termRequestRenameTab);
    connect(m_term, &TermWidget::termRequestOpenSettings, this, &TermWidgetWrapper::termRequestOpenSettings);
    connect(m_term, &TermWidget::termRequestOpenCustomCommand, this, &TermWidgetWrapper::termRequestOpenCustomCommand);
    connect(
        m_term, &TermWidget::termRequestOpenRemoteManagement, this, &TermWidgetWrapper::termRequestOpenRemoteManagement);
    connect(m_term, &TermWidget::copyAvailable, this, [this](bool enable) {
        if (Settings::instance()->IsPasteSelection() && enable) {
            qDebug() << "hasCopySelection";
            QApplication::clipboard()->setText(selectedText(), QClipboard::Clipboard);
        }
    });
    connect(Settings::instance(), &Settings::terminalSettingChanged, this, &TermWidgetWrapper::onSettingValueChanged);
}

QList<int> TermWidgetWrapper::getRunningSessionIdList()
{
    return m_term->getRunningSessionIdList();
}

bool TermWidgetWrapper::hasRunningProcess()
{
    return m_term->hasRunningProcess();
}

bool TermWidgetWrapper::isTitleChanged() const
{
    return m_term->isTitleChanged();
}

QString TermWidgetWrapper::title() const
{
    return m_term->title();
}

QString TermWidgetWrapper::workingDirectory()
{
    return m_term->workingDirectory();
}

void TermWidgetWrapper::sendText(const QString &text)
{
    return m_term->sendText(text);
}

void TermWidgetWrapper::setTerminalOpacity(qreal opacity)
{
    m_term->setTerminalOpacity(opacity);
    /******* Modify by n014361 wangpeili 2020-01-04: 修正实时设置透明度问题************/
    m_term->hide();
    m_term->show();
    /********************* Modify by n014361 wangpeili End ************************/
}

void TermWidgetWrapper::setColorScheme(const QString &name)
{
    m_term->setColorScheme(name);
}

/*******************************************************************************
 1. @函数:   void TermWidgetWrapper::setTerminalFont(const QString &fontName)
 2. @作者:     n014361 王培利
 3. @日期:     2020-01-10
 4. @说明:     设置字体
*******************************************************************************/
void TermWidgetWrapper::setTerminalFont(const QString &fontName)
{
    QFont font = m_term->getTerminalFont();
    // font.setFixedPitch(true);
    font.setFamily(fontName);

    m_term->setTerminalFont(font);
}

/*******************************************************************************
 1. @函数:   void TermWidgetWrapper::setTerminalFontSize(const int fontSize)
 2. @作者:     n014361 王培利
 3. @日期:     2020-01-10
 4. @说明:     设置字体大小
*******************************************************************************/
void TermWidgetWrapper::setTerminalFontSize(const int fontSize)
{
    QFont font = m_term->getTerminalFont();
    font.setFixedPitch(true);
    font.setPointSize(fontSize);
    m_term->setTerminalFont(font);
}

/*******************************************************************************
 1. @函数:   void TermWidgetWrapper::selectAll()
 2. @作者:     n014361 王培利
 3. @日期:     2020-01-10
 4. @说明:   全选
*******************************************************************************/
void TermWidgetWrapper::selectAll()
{
    m_term->setSelectionAll();
}

/*******************************************************************************
 1. @函数:   void TermWidgetWrapper::skipToNextCommand()
 2. @作者:     n014361 王培利
 3. @日期:     2020-01-10
 4. @说明:    跳转到下一个命令（这个功能没找到库的接口，现在是暂时是以虚拟键形式实现）
*******************************************************************************/
void TermWidgetWrapper::skipToNextCommand()
{
    qDebug() << "skipToNextCommand";
}

/*******************************************************************************
 1. @函数:  void TermWidgetWrapper::skipToPreCommand()
 2. @作者:     n014361 王培利
 3. @日期:     2020-01-10
 4. @说明:   跳转到前一个命令（这个功能没找到库的接口，现在是暂时是以虚拟键形式实现）
*******************************************************************************/
void TermWidgetWrapper::skipToPreCommand()
{
    qDebug() << "skipToPreCommand";
}

/*******************************************************************************
 1. @函数:  void TermWidgetWrapper::setCursorShape(int shape)
 2. @作者:     n014361 王培利
 3. @日期:     2020-01-10
 4. @说明:     设置光标形状
*******************************************************************************/
void TermWidgetWrapper::setCursorShape(int shape)
{
    Konsole::Emulation::KeyboardCursorShape cursorShape = Konsole::Emulation::KeyboardCursorShape(shape);
    m_term->setKeyboardCursorShape(cursorShape);
}

/*******************************************************************************
 1. @函数:   setCursorBlinking(bool enable)
 2. @作者:   王培利n014361
 3. @日期:   2020-01-10
 4. @说明:   设置光标是否闪烁
*******************************************************************************/
void TermWidgetWrapper::setCursorBlinking(bool enable)
{
    m_term->setBlinkingCursor(enable);
}

void TermWidgetWrapper::setPressingScroll(bool enable)
{
    if (enable) {
        m_term->setMotionAfterPasting(2);
    } else {
        m_term->setMotionAfterPasting(0);
    }
}

void TermWidgetWrapper::zoomIn()
{
    m_term->zoomIn();
}

void TermWidgetWrapper::zoomOut()
{
    m_term->zoomOut();
}

void TermWidgetWrapper::copyClipboard()
{
    m_term->copyClipboard();
}

void TermWidgetWrapper::pasteClipboard()
{
    m_term->pasteClipboard();
}

/*******************************************************************************
 1. @函数:   void TermWidgetWrapper::pasteSelection()
 2. @作者:     n014361 王培利
 3. @日期:     2020-01-10
 4. @说明:     粘贴选择内容
*******************************************************************************/
void TermWidgetWrapper::pasteSelection()
{

    int x1, y1, x2, y2;
    m_term->getSelectionStart(x1, y1);
    qDebug() << x1 << y1;
    m_term->getSelectionEnd(x2, y2);
    qDebug() << x2 << y2;

    m_term->pasteSelection();
}

void TermWidgetWrapper::toggleShowSearchBar()
{
    m_term->toggleShowSearchBar();
}

QString TermWidgetWrapper::selectedText(bool preserveLineBreaks)
{
    return m_term->selectedText(true);
}
/*******************************************************************************
 1. @函数:    onSettingValueChanged
 2. @作者:    n014361 王培利
 3. @日期:    2020-02-20
 4. @说明:    Terminal的各项设置生效
*******************************************************************************/
void TermWidgetWrapper::onSettingValueChanged(const QString &keyName)
{
    qDebug() << "onSettingValueChanged:" << keyName;
    if (keyName == "basic.interface.opacity") {
        setTerminalOpacity(Settings::instance()->opacity());
        return;
    }

    if (keyName == "basic.interface.font") {
        setTerminalFont(Settings::instance()->fontName());
        return;
    }

    if (keyName == "basic.interface.font_size") {
        setTerminalFontSize(Settings::instance()->fontSize());
        return;
    }

    if (keyName == "advanced.cursor.cursor_shape") {
        setCursorShape(Settings::instance()->cursorShape());
        return;
    }

    if (keyName == "advanced.cursor.cursor_blink") {
        setCursorBlinking(Settings::instance()->cursorBlink());
        return;
    }

    if (keyName == "advanced.scroll.scroll_on_key") {
        setPressingScroll(Settings::instance()->PressingScroll());
        return;
    }

    if (keyName == "basic.interface.theme") {
        return;
    }

    if ((keyName == "advanced.cursor.auto_copy_selection") || (keyName == "advanced.scroll.scroll_on_output")) {
        qDebug() << "settingValue[" << keyName << "] changed to " << Settings::instance()->OutputtingScroll()
                 << ", auto effective when happen";
        return;
    }

    qDebug() << "settingValue[" << keyName << "] changed is not effective";
}

void TermWidgetWrapper::initUI()
{
    setFocusProxy(m_term);

    m_layout = new QVBoxLayout;
    setLayout(m_layout);

    m_layout->addWidget(m_term);
    m_layout->setContentsMargins(0, 0, 0, 0);
}

void TermWidgetWrapper::setTextCodec(QTextCodec *codec)
{
    m_term->setTextCodec(codec);
}