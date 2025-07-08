#ifndef FILE_MANAGER_APP_H
#define FILE_MANAGER_APP_H

#include "../core/apps/app_base.h"
#include <vector>
#include <functional>

/**
 * @brief File Manager Application for SD Card Operations
 * 
 * Provides comprehensive file system management including browsing,
 * copying, moving, deleting, and viewing files on the SD card.
 */
class FileManagerApp : public AppBase {
public:
    enum class FileType {
        DIRECTORY,
        TEXT_FILE,
        IMAGE_FILE,
        AUDIO_FILE,
        VIDEO_FILE,
        ARCHIVE_FILE,
        EXECUTABLE_FILE,
        CONFIG_FILE,
        LOG_FILE,
        UNKNOWN
    };

    enum class SortMode {
        NAME_ASC,
        NAME_DESC,
        SIZE_ASC,
        SIZE_DESC,
        DATE_ASC,
        DATE_DESC,
        TYPE_ASC,
        TYPE_DESC
    };

    enum class ViewMode {
        LIST,
        GRID,
        DETAILS
    };

    struct FileInfo {
        String name;
        String fullPath;
        FileType type;
        size_t size;
        uint32_t lastModified;
        bool isHidden;
        bool isReadOnly;
        bool isSelected;
    };

    struct DirectoryInfo {
        String path;
        std::vector<FileInfo> files;
        size_t totalSize;
        uint32_t fileCount;
        uint32_t dirCount;
    };

    struct ClipboardItem {
        String sourcePath;
        bool isCut; // true for cut, false for copy
        uint32_t timestamp;
    };

    FileManagerApp(const AppInfo& info);
    virtual ~FileManagerApp();

    // AppBase implementation
    bool initialize() override;
    bool start() override;
    bool pause() override;
    bool resume() override;
    bool stop() override;
    void cleanup() override;

    // Event handling
    void onKeyPress(uint8_t key) override;
    void onTouch(lv_event_t* e) override;

    // UI management
    lv_obj_t* createUI(lv_obj_t* parent) override;
    void updateUI() override;

    // Static app info
    static AppInfo getAppInfo();

    // File operations
    bool navigateToDirectory(const String& path);
    bool navigateUp();
    bool navigateHome();
    bool refreshCurrentDirectory();

    // File manipulation
    bool createDirectory(const String& name);
    bool createFile(const String& name);
    bool deleteFile(const String& path);
    bool deleteDirectory(const String& path);
    bool renameFile(const String& oldPath, const String& newName);
    bool copyFile(const String& sourcePath, const String& destPath);
    bool moveFile(const String& sourcePath, const String& destPath);

    // Clipboard operations
    void copyToClipboard(const String& path);
    void cutToClipboard(const String& path);
    bool pasteFromClipboard();
    void clearClipboard();
    bool hasClipboardContent() const;

    // Selection operations
    void selectFile(const String& path);
    void deselectFile(const String& path);
    void selectAll();
    void deselectAll();
    std::vector<String> getSelectedFiles() const;
    bool hasSelection() const;

    // View operations
    void setViewMode(ViewMode mode);
    void setSortMode(SortMode mode);
    void toggleShowHidden();
    void setFilter(const String& filter);

    // File information
    FileInfo getFileInfo(const String& path);
    DirectoryInfo getCurrentDirectoryInfo();
    size_t getDirectorySize(const String& path);
    bool fileExists(const String& path);
    bool isDirectory(const String& path);

    // Configuration
    bool saveConfig() override;
    bool loadConfig() override;
    void resetConfig() override;

private:
    // UI components
    lv_obj_t* mainContainer;
    lv_obj_t* toolbarPanel;
    lv_obj_t* pathPanel;
    lv_obj_t* contentPanel;
    lv_obj_t* statusPanel;
    lv_obj_t* sidePanel;

    // Toolbar elements
    lv_obj_t* backButton;
    lv_obj_t* upButton;
    lv_obj_t* homeButton;
    lv_obj_t* refreshButton;
    lv_obj_t* newFolderButton;
    lv_obj_t* deleteButton;
    lv_obj_t* copyButton;
    lv_obj_t* cutButton;
    lv_obj_t* pasteButton;
    lv_obj_t* viewModeButton;
    lv_obj_t* sortButton;

    // Path navigation
    lv_obj_t* pathLabel;
    lv_obj_t* pathInput;
    lv_obj_t* breadcrumbContainer;

    // Content area
    lv_obj_t* fileList;
    lv_obj_t* fileGrid;
    lv_obj_t* detailsTable;
    lv_obj_t* scrollContainer;

    // Status bar
    lv_obj_t* statusLabel;
    lv_obj_t* selectionLabel;
    lv_obj_t* progressBar;

    // Side panel
    lv_obj_t* bookmarksList;
    lv_obj_t* recentList;
    lv_obj_t* propertiesPanel;

    // Data
    String currentPath;
    DirectoryInfo currentDirectory;
    std::vector<String> navigationHistory;
    int32_t historyIndex;
    std::vector<ClipboardItem> clipboard;
    ViewMode currentViewMode;
    SortMode currentSortMode;
    String currentFilter;
    bool showHidden;

    // Settings
    struct Settings {
        ViewMode defaultViewMode;
        SortMode defaultSortMode;
        bool showHiddenFiles;
        bool confirmDelete;
        bool showThumbnails;
        uint32_t maxHistorySize;
        std::vector<String> bookmarks;
        std::vector<String> recentPaths;
    } settings;

    // UI creation methods
    void createToolbar();
    void createPathPanel();
    void createContentPanel();
    void createStatusPanel();
    void createSidePanel();
    void createContextMenu();

    // UI update methods
    void updateToolbar();
    void updatePathPanel();
    void updateContentPanel();
    void updateStatusPanel();
    void updateFileList();
    void updateFileGrid();
    void updateDetailsTable();
    void updateBreadcrumbs();

    // Event handlers
    static void onBackClicked(lv_event_t* e);
    static void onUpClicked(lv_event_t* e);
    static void onHomeClicked(lv_event_t* e);
    static void onRefreshClicked(lv_event_t* e);
    static void onNewFolderClicked(lv_event_t* e);
    static void onDeleteClicked(lv_event_t* e);
    static void onCopyClicked(lv_event_t* e);
    static void onCutClicked(lv_event_t* e);
    static void onPasteClicked(lv_event_t* e);
    static void onFileSelected(lv_event_t* e);
    static void onFileDoubleClicked(lv_event_t* e);
    static void onViewModeChanged(lv_event_t* e);
    static void onSortModeChanged(lv_event_t* e);

    // File operations helpers
    bool scanDirectory(const String& path, DirectoryInfo& dirInfo);
    void sortFiles(std::vector<FileInfo>& files, SortMode mode);
    void filterFiles(std::vector<FileInfo>& files, const String& filter);
    FileType detectFileType(const String& filename);
    String getFileIcon(FileType type);
    String formatFileSize(size_t size);
    String formatTimestamp(uint32_t timestamp);

    // Navigation helpers
    void addToHistory(const String& path);
    bool canNavigateBack() const;
    bool canNavigateForward() const;
    void navigateBack();
    void navigateForward();

    // Clipboard helpers
    void cleanupClipboard();
    bool validateClipboardOperation(const String& destPath);

    // Bookmark helpers
    void addBookmark(const String& path);
    void removeBookmark(const String& path);
    bool isBookmarked(const String& path) const;

    // Recent files helpers
    void addToRecent(const String& path);
    void cleanupRecent();

    // Dialog helpers
    void showCreateDirectoryDialog();
    void showCreateFileDialog();
    void showDeleteConfirmDialog();
    void showRenameDialog(const String& currentName);
    void showPropertiesDialog(const String& path);
    void showErrorDialog(const String& message);

    // Configuration helpers
    String getConfigPath() const;
    bool saveSettings();
    bool loadSettings();
    void resetSettings();

    // Constants
    static const uint32_t MAX_HISTORY_SIZE = 50;
    static const uint32_t MAX_RECENT_SIZE = 20;
    static const uint32_t CLIPBOARD_TIMEOUT = 3600000; // 1 hour
    static const size_t MAX_FILENAME_LENGTH = 255;
    static const size_t SCAN_BATCH_SIZE = 100;
};

#endif // FILE_MANAGER_APP_H