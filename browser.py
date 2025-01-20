import sys
import re
from PyQt5.QtCore import QUrl, QSize, Qt, QTimer, QPropertyAnimation, QRectF
from PyQt5.QtGui import QIcon, QKeySequence, QColor, QPainter, QPixmap, QCursor
from PyQt5.QtWidgets import QApplication, QMainWindow, QTabWidget, QToolBar, QLineEdit, QAction, QVBoxLayout, QWidget, QFileDialog, QMenu, QColorDialog, QDialog, QPushButton, QLabel, QGraphicsOpacityEffect, QShortcut, QStyleOptionTab, QStyle
from PyQt5.QtWebEngineWidgets import QWebEngineView, QWebEnginePage, QWebEngineProfile, QWebEngineSettings

class SettingsDialog(QDialog):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        self.initUI()

    def initUI(self):
        layout = QVBoxLayout()
        self.setLayout(layout)

        self.search_engine_label = QLabel("Default search engine:")
        layout.addWidget(self.search_engine_label)

        self.search_engine_edit = QLineEdit(self.parent.default_search_engine)
        layout.addWidget(self.search_engine_edit)

        self.tint_color_label = QLabel("Tint color:")
        layout.addWidget(self.tint_color_label)

        self.tint_color_edit = QLineEdit()
        self.tint_color_edit.setReadOnly(True)
        self.tint_color_edit.setStyleSheet("background-color: {}".format(self.parent.default_bg_color))
        layout.addWidget(self.tint_color_edit)

        self.color_button = QPushButton("Select Color")
        self.color_button.clicked.connect(self.choose_color)
        layout.addWidget(self.color_button)

        self.save_button = QPushButton("Save")
        self.save_button.clicked.connect(self.save_settings)
        layout.addWidget(self.save_button)

    def choose_color(self):
        color = QColorDialog.getColor()
        if color.isValid():
            self.parent.default_bg_color = color.name()
            self.tint_color_edit.setStyleSheet("background-color: {}".format(self.parent.default_bg_color))

    def save_settings(self):
        self.parent.default_search_engine = self.search_engine_edit.text()
        self.accept()

class AdBlockerPage(QWebEnginePage):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.ad_patterns = [
            r"ad.doubleclick.net",
            r"ads.google.com",
            # Add more ad patterns here
        ]
        self.init_privacy_settings()

    def acceptNavigationRequest(self, url, _type, is_main_frame):
        if self.is_ad(url.toString()):
            return False
        return super().acceptNavigationRequest(url, _type, is_main_frame)

    def is_ad(self, url):
        for pattern in self.ad_patterns:
            if re.search(pattern, url):
                return True
        return False

    def init_privacy_settings(self):
        profile = self.profile()
        profile.setPersistentCookiesPolicy(QWebEngineProfile.NoPersistentCookies)
        profile.setHttpCacheType(QWebEngineProfile.MemoryHttpCache)
        profile.setPersistentStoragePath("")

        settings = self.settings()
        settings.setAttribute(QWebEngineSettings.JavascriptCanOpenWindows, False)
        settings.setAttribute(QWebEngineSettings.LocalStorageEnabled, False)
        settings.setAttribute(QWebEngineSettings.LocalContentCanAccessRemoteUrls, False)
        settings.setAttribute(QWebEngineSettings.XSSAuditingEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebGLEnabled, False)

class Browser(QMainWindow):
    def __init__(self):
        super().__init__()

        self.default_search_engine = "https://www.google.com/search?q="
        self.default_bg_color = "#202020"

        # Create a Tab widget
        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(True)
        self.tabs.tabCloseRequested.connect(self.close_current_tab)
        self.tabs.currentChanged.connect(self.current_tab_changed)
        self.setCentralWidget(self.tabs)

        # Setup the navigation bar
        navbar = QToolBar("Navigation")
        navbar.setIconSize(QSize(24, 24))  # Set icon size
        navbar.setMovable(False)
        navbar.setStyleSheet("background-color: #202020; border: none;")
        self.addToolBar(navbar)

        # Adding navigation actions
        back_btn = QAction(QIcon("icons/back.png"), 'Back', self)
        back_btn.triggered.connect(self.navigate_back)
        navbar.addAction(back_btn)

        forward_btn = QAction(QIcon("icons/forward.png"), 'Forward', self)
        forward_btn.triggered.connect(self.navigate_forward)
        navbar.addAction(forward_btn)

        reload_btn = QAction(QIcon("icons/reload.png"), 'Reload', self)
        reload_btn.triggered.connect(self.reload_page)
        navbar.addAction(reload_btn)

        home_btn = QAction(QIcon("icons/home.png"), 'Home', self)
        home_btn.triggered.connect(self.navigate_home)
        navbar.addAction(home_btn)

        # URL/Search bar
        self.url_bar = QLineEdit()
        self.url_bar.returnPressed.connect(self.navigate_to_url_or_search)
        self.url_bar.setStyleSheet(f"""
            QLineEdit {{
                padding: 5px;
                border-radius: 15px;
                border: 1px solid #ccc;
                font-size: 16px;
                background-color: #1e1e1e;
                color: white;
            }}
        """)
        navbar.addWidget(self.url_bar)

        # Gesture recognition and animations
        self.browser_view = self.add_new_tab(QUrl('http://www.google.com'), 'New Tab')
        self.browser_view.setAttribute(Qt.WA_AcceptTouchEvents)
        self.browser_view.setStyleSheet("background-color: white;")

        # Add a fade-in effect for newly opened tabs
        self.add_fade_in_effect(self.browser_view)

        # Bookmarks menu
        bookmarks_btn = QAction(QIcon("icons/bookmark.png"), 'Bookmarks', self)
        bookmarks_btn.triggered.connect(self.show_bookmarks_menu)
        navbar.addAction(bookmarks_btn)

        # Reading mode
        reading_mode_btn = QAction(QIcon("icons/read.png"), 'Reading Mode', self)
        reading_mode_btn.triggered.connect(self.toggle_reading_mode)
        navbar.addAction(reading_mode_btn)

        # Settings
        settings_btn = QAction(QIcon("icons/settings.png"), 'Settings', self)
        settings_btn.triggered.connect(self.open_settings)
        navbar.addAction(settings_btn)

        # Incognito mode
        incognito_btn = QAction(QIcon("icons/incognito.png"), 'Incognito', self)
        incognito_btn.triggered.connect(self.toggle_incognito_mode)
        navbar.addAction(incognito_btn)

        # Extensions
        extensions_btn = QAction(QIcon("icons/extensions.png"), 'Extensions', self)
        extensions_btn.triggered.connect(self.open_extensions)
        navbar.addAction(extensions_btn)

        # Shortcuts
        new_tab_shortcut = QShortcut(QKeySequence('Ctrl+T'), self)
        new_tab_shortcut.activated.connect(self.add_new_tab)

        close_tab_shortcut = QShortcut(QKeySequence('Ctrl+W'), self)
        close_tab_shortcut.activated.connect(self.close_current_tab)

        self.setWindowTitle("WebKit Browser")
        self.showMaximized()

    def add_new_tab(self, url=QUrl(''), label="Blank"):
        browser_view = QWebEngineView()
        browser_view.setPage(AdBlockerPage(self))
        browser_view.setUrl(url)
        index = self.tabs.addTab(browser_view, label)
        self.tabs.setCurrentIndex(index)
        browser_view.urlChanged.connect(lambda q: self.update_url(q, index))

        # Add a fade-in effect for the new tab
        self.add_fade_in_effect(browser_view)
        return browser_view

    def add_fade_in_effect(self, view):
        effect = QGraphicsOpacityEffect(view)
        view.setGraphicsEffect(effect)
        effect.setOpacity(0)
        animation = QPropertyAnimation(effect, b"opacity")
        animation.setDuration(500)  # Duration in milliseconds
        animation.setStartValue(0)
        animation.setEndValue(1)
        animation.start()

    def current_tab_changed(self, index):
        current_browser = self.tabs.widget(index)
        if current_browser:
            self.update_url(current_browser.url(), index)

    def update_url(self, q, index):
        self.url_bar.setText(q.toString())
        self.tabs.setTabText(index, q.toString())

    def navigate_home(self):
        self.tabs.currentWidget().setUrl(QUrl("http://www.google.com"))

    def navigate_to_url_or_search(self):
        url = self.url_bar.text()
        if "." not in url:
            url = self.default_search_engine + url
        elif not url.startswith('http://') and not url.startswith('https://'):
            url = 'http://' + url
        self.tabs.currentWidget().setUrl(QUrl(url))

    def navigate_back(self):
        self.tabs.currentWidget().back()

    def navigate_forward(self):
        self.tabs.currentWidget().forward()

    def reload_page(self):
        self.tabs.currentWidget().reload()

    def close_current_tab(self, index=None):
        if index is None:
            index = self.tabs.currentIndex()
        if self.tabs.count() > 1:
            self.tabs.removeTab(index)

    def show_bookmarks_menu(self):
        menu = QMenu(self)
        menu.addAction("Placeholder Bookmark 1")
        menu.addAction("Placeholder Bookmark 2")
        menu.exec_(QCursor.pos())

    def open_settings(self):
        settings_dialog = SettingsDialog(self)
        settings_dialog.exec_()

    def toggle_incognito_mode(self):
        # Toggle incognito mode
        pass

    def open_extensions(self):
        # Open extensions menu
        pass

    def toggle_reading_mode(self):
        # Toggle reading mode
        pass

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyleSheet("QTabWidget::pane { border: 0; } QTabBar::tab { height: 30px; padding: 10px; margin: 2px; border-radius: 10px; background: #3c3c3c; color: white; } QTabBar::tab:selected { background: #1e1e1e; }")
    window = Browser()
    window.show()
    sys.exit(app.exec_())
