#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QToolBar>
#include <QLineEdit>
#include <QAction>
#include <QVBoxLayout>
#include <QWidget>
#include <QFileDialog>
#include <QMenu>
#include <QColorDialog>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QShortcut>
#include <QStyleOptionTab>
#include <QStyle>
#include <QUrl>
#include <QSize>
#include <QIcon>
#include <QKeySequence>
#include <QColor>
#include <QPainter>
#include <QPixmap>
#include <QCursor>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QPropertyAnimation>
#include <QRectF>
#include <QTimer>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget* parent) : QDialog(parent), parent(parent) {
        initUI();
    }

private:
    QWidget* parent;
    QLineEdit* search_engine_edit;
    QLineEdit* tint_color_edit;
    QPushButton* color_button;
    QPushButton* save_button;
    QLabel* search_engine_label;
    QLabel* tint_color_label;

    void initUI() {
        QVBoxLayout* layout = new QVBoxLayout();
        setLayout(layout);

        search_engine_label = new QLabel("Default search engine:");
        layout->addWidget(search_engine_label);

        search_engine_edit = new QLineEdit(static_cast<Browser*>(parent)->default_search_engine);
        layout->addWidget(search_engine_edit);

        tint_color_label = new QLabel("Tint color:");
        layout->addWidget(tint_color_label);

        tint_color_edit = new QLineEdit();
        tint_color_edit->setReadOnly(true);
        tint_color_edit->setStyleSheet(QString("background-color: %1").arg(static_cast<Browser*>(parent)->default_bg_color));
        layout->addWidget(tint_color_edit);

        color_button = new QPushButton("Select Color");
        connect(color_button, &QPushButton::clicked, this, &SettingsDialog::choose_color);
        layout->addWidget(color_button);

        save_button = new QPushButton("Save");
        connect(save_button, &QPushButton::clicked, this, &SettingsDialog::save_settings);
        layout->addWidget(save_button);
    }

    void choose_color() {
        QColor color = QColorDialog::getColor();
        if (color.isValid()) {
            static_cast<Browser*>(parent)->default_bg_color = color.name();
            tint_color_edit->setStyleSheet(QString("background-color: %1").arg(static_cast<Browser*>(parent)->default_bg_color));
        }
    }

    void save_settings() {
        static_cast<Browser*>(parent)->default_search_engine = search_engine_edit->text();
        accept();
    }
};

class AdBlockerPage : public QWebEnginePage {
    Q_OBJECT
public:
    AdBlockerPage(QWidget* parent = nullptr) : QWebEnginePage(parent) {
        ad_patterns = {
            QRegularExpression("ad.doubleclick.net"),
            QRegularExpression("ads.google.com"),
            // Add more ad patterns here
        };
        init_privacy_settings();
    }

protected:
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) override {
        if (is_ad(url.toString())) {
            return false;
        }
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }

private:
    QList<QRegularExpression> ad_patterns;

    bool is_ad(const QString& url) {
        for (const QRegularExpression& pattern : ad_patterns) {
            if (pattern.match(url).hasMatch()) {
                return true;
            }
        }
        return false;
    }

    void init_privacy_settings() {
        QWebEngineProfile* profile = this->profile();
        profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
        profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
        profile->setPersistentStoragePath("");

        QWebEngineSettings* settings = this->settings();
        settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
        settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, false);
        settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
        settings->setAttribute(QWebEngineSettings::XSSAuditingEnabled, true);
        settings->setAttribute(QWebEngineSettings::WebGLEnabled, false);
    }
};

class Browser : public QMainWindow {
    Q_OBJECT
public:
    Browser() {
        default_search_engine = "https://www.google.com/search?q=";
        default_bg_color = "#202020";

        tabs = new QTabWidget();
        tabs->setTabsClosable(true);
        connect(tabs, &QTabWidget::tabCloseRequested, this, &Browser::close_current_tab);
        connect(tabs, &QTabWidget::currentChanged, this, &Browser::current_tab_changed);
        setCentralWidget(tabs);

        QToolBar* navbar = new QToolBar("Navigation");
        navbar->setIconSize(QSize(24, 24));
        navbar->setMovable(false);
        navbar->setStyleSheet("background-color: #202020; border: none;");
        addToolBar(navbar);

        QAction* back_btn = new QAction(QIcon("icons/back.png"), "Back", this);
        connect(back_btn, &QAction::triggered, this, &Browser::navigate_back);
        navbar->addAction(back_btn);

        QAction* forward_btn = new QAction(QIcon("icons/forward.png"), "Forward", this);
        connect(forward_btn, &QAction::triggered, this, &Browser::navigate_forward);
        navbar->addAction(forward_btn);

        QAction* reload_btn = new QAction(QIcon("icons/reload.png"), "Reload", this);
        connect(reload_btn, &QAction::triggered, this, &Browser::reload_page);
        navbar->addAction(reload_btn);

        QAction* home_btn = new QAction(QIcon("icons/home.png"), "Home", this);
        connect(home_btn, &QAction::triggered, this, &Browser::navigate_home);
        navbar->addAction(home_btn);

        url_bar = new QLineEdit();
        connect(url_bar, &QLineEdit::returnPressed, this, &Browser::navigate_to_url_or_search);
        url_bar->setStyleSheet(R"(
            QLineEdit {
                padding: 5px;
                border-radius: 15px;
                border: 1px solid #ccc;
                font-size: 16px;
                background-color: #1e1e1e;
                color: white;
            }
        )");
        navbar->addWidget(url_bar);

        browser_view = add_new_tab(QUrl("http://www.google.com"), "New Tab");
        browser_view->setAttribute(Qt::WA_AcceptTouchEvents);
        browser_view->setStyleSheet("background-color: white;");
        add_fade_in_effect(browser_view);

        QAction* bookmarks_btn = new QAction(QIcon("icons/bookmark.png"), "Bookmarks", this);
        connect(bookmarks_btn, &QAction::triggered, this, &Browser::show_bookmarks_menu);
        navbar->addAction(bookmarks_btn);

        QAction* reading_mode_btn = new QAction(QIcon("icons/read.png"), "Reading Mode", this);
        connect(reading_mode_btn, &QAction::triggered, this, &Browser::toggle_reading_mode);
        navbar->addAction(reading_mode_btn);

        QAction* settings_btn = new QAction(QIcon("icons/settings.png"), "Settings", this);
        connect(settings_btn, &QAction::triggered, this, &Browser::open_settings);
        navbar->addAction(settings_btn);

        QAction* incognito_btn = new QAction(QIcon("icons/incognito.png"), "Incognito", this);
        connect(incognito_btn, &QAction::triggered, this, &Browser::toggle_incognito_mode);
        navbar->addAction(incognito_btn);

        QAction* extensions_btn = new QAction(QIcon("icons/extensions.png"), "Extensions", this);
        connect(extensions_btn, &QAction::triggered, this, &Browser::open_extensions);
        navbar->addAction(extensions_btn);

        QShortcut* new_tab_shortcut = new QShortcut(QKeySequence("Ctrl+T"), this);
        connect(new_tab_shortcut, &QShortcut::activated, this, &Browser::add_new_tab);

        QShortcut* close_tab_shortcut = new QShortcut(QKeySequence("Ctrl+W"), this);
        connect(close_tab_shortcut, &QShortcut::activated, this, &Browser::close_current_tab);

        setWindowTitle("WebKit Browser");
        showMaximized();
    }

private:
    QString default_search_engine;
    QString default_bg_color;
    QTabWidget* tabs;
    QLineEdit* url_bar;
    QWebEngineView* browser_view;

    QWebEngineView* add_new_tab(const QUrl& url, const QString& label) {
        QWebEngineView* browser_view = new QWebEngineView();
        browser_view->setPage(new AdBlockerPage(this));
        browser_view->setUrl(url);
        int index = tabs->addTab(browser_view, label);
        tabs->setCurrentIndex(index);
        connect(browser_view, &QWebEngineView::urlChanged, this, [=](const QUrl& q) { update_url(q, index); });

        add_fade_in_effect(browser_view);
        return browser_view;
    }

    void add_fade_in_effect(QWebEngineView* view) {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(view);
        view->setGraphicsEffect(effect);
        effect->setOpacity(0);
        QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
        animation->setDuration(500);
        animation->setStartValue(0);
        animation->setEndValue(1);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void current_tab_changed(int index) {
        QWebEngineView* current_browser = qobject_cast<QWebEngineView*>(tabs->widget(index));
        if (current_browser) {
            update_url(current_browser->url(), index);
        }
    }

    void update_url(const QUrl& q, int index) {
        url_bar->setText(q.toString());
        tabs->setTabText(index, q.toString());
    }

    void navigate_home() {
        tabs->currentWidget()->setUrl(QUrl("http://www.google.com"));
    }

    void navigate_to_url_or_search() {
        QString url = url_bar->text();
        if (!url.contains('.')) {
            url = default_search_engine + url;
        } else if (!url.startsWith("http://") && !url.startsWith("https://")) {
            url = "http://" + url;
        }
        tabs->currentWidget()->setUrl(QUrl(url));
    }

    void navigate_back() {
        qobject_cast<QWebEngineView*>(tabs->currentWidget())->back();
    }

    void navigate_forward() {
        qobject_cast<QWebEngineView*>(tabs->currentWidget())->forward();
    }

    void reload_page() {
        qobject_cast<QWebEngineView*>(tabs->currentWidget())->reload();
    }

    void close_current_tab(int index = -1) {
        if (index == -1) {
            index = tabs->currentIndex();
        }
        if (tabs->count() > 1) {
            tabs->removeTab(index);
        }
    }

    void show_bookmarks_menu() {
        QMenu menu(this);
        menu.addAction("Placeholder Bookmark 1");
        menu.addAction("Placeholder Bookmark 2");
        menu.exec(QCursor::pos());
    }

    void open_settings() {
        SettingsDialog settings_dialog(this);
        settings_dialog.exec();
    }

    void toggle_incognito_mode() {
        // Toggle incognito mode
    }

    void open_extensions() {
        // Open extensions menu
    }

    void toggle_reading_mode() {
        // Toggle reading mode
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyleSheet(R"(
        QTabWidget::pane { border: 0; }
        QTabBar::tab { height: 30px; padding: 10px; margin: 2px; border-radius: 10px; background: #3c3c3c; color: white; }
        QTabBar::tab:selected { background: #1e1e1e; }
    )");
    Browser window;
    window.show();
    return app.exec();
}

#include "main.moc"
