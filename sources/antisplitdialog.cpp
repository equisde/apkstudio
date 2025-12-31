#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include "antisplitdialog.h"

AntiSplitDialog::AntiSplitDialog(QWidget *parent)
    : QDialog(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->addLayout(buildForm());
    layout->addWidget(buildButtonBox());
    setMinimumWidth(500);
    setWindowTitle(tr("AntiSplit - Merge Split APKs"));
    updateButtons();
}

QWidget *AntiSplitDialog::buildButtonBox()
{
    m_ButtonBox = new QDialogButtonBox(this);
    m_ButtonBox->setOrientation(Qt::Horizontal);
    m_ButtonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    m_ButtonBox->button(QDialogButtonBox::Ok)->setText(tr("Merge"));
    m_ButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(m_ButtonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (m_ListFiles->count() < 1) {
            QMessageBox::warning(this, tr("Error"), tr("Please add at least one APK or XAPK file."));
            return;
        }
        if (m_EditOutput->text().isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Please specify an output file."));
            return;
        }
        // Verify all input files exist
        for (int i = 0; i < m_ListFiles->count(); ++i) {
            QString filePath = m_ListFiles->item(i)->data(Qt::UserRole).toString();
            if (!QFile::exists(filePath)) {
                QMessageBox::warning(this, tr("Error"), 
                    tr("Input file not found:\n%1").arg(filePath));
                return;
            }
        }
        accept();
    });
    connect(m_ButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    return m_ButtonBox;
}

QLayout *AntiSplitDialog::buildForm()
{
    auto layout = new QVBoxLayout();
    
    // Info label
    auto infoLabel = new QLabel(tr("Add split APKs (base.apk + config.*.apk) or XAPK files to merge them into a single universal APK."), this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    // Files group
    auto filesGroup = new QGroupBox(tr("Input Files"), this);
    auto filesLayout = new QVBoxLayout(filesGroup);
    
    m_ListFiles = new QListWidget(this);
    m_ListFiles->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_ListFiles->setMinimumHeight(150);
    connect(m_ListFiles, &QListWidget::itemSelectionChanged, this, &AntiSplitDialog::updateButtons);
    filesLayout->addWidget(m_ListFiles);
    
    auto btnLayout = new QHBoxLayout();
    m_BtnAddApk = new QPushButton(tr("Add APK(s)..."), this);
    connect(m_BtnAddApk, &QPushButton::clicked, this, &AntiSplitDialog::handleAddApk);
    btnLayout->addWidget(m_BtnAddApk);
    
    m_BtnAddXapk = new QPushButton(tr("Add XAPK..."), this);
    connect(m_BtnAddXapk, &QPushButton::clicked, this, &AntiSplitDialog::handleAddXapk);
    btnLayout->addWidget(m_BtnAddXapk);
    
    m_BtnRemove = new QPushButton(tr("Remove"), this);
    connect(m_BtnRemove, &QPushButton::clicked, this, &AntiSplitDialog::handleRemoveSelected);
    btnLayout->addWidget(m_BtnRemove);
    
    m_BtnClear = new QPushButton(tr("Clear All"), this);
    connect(m_BtnClear, &QPushButton::clicked, this, &AntiSplitDialog::handleClearAll);
    btnLayout->addWidget(m_BtnClear);
    
    btnLayout->addStretch();
    filesLayout->addLayout(btnLayout);
    layout->addWidget(filesGroup);
    
    // Output group
    auto outputGroup = new QGroupBox(tr("Output"), this);
    auto outputLayout = new QHBoxLayout(outputGroup);
    
    m_EditOutput = new QLineEdit(this);
    m_EditOutput->setPlaceholderText(tr("Output APK file path..."));
    connect(m_EditOutput, &QLineEdit::textChanged, this, &AntiSplitDialog::updateButtons);
    outputLayout->addWidget(m_EditOutput);
    
    auto btnBrowse = new QPushButton(tr("Browse..."), this);
    connect(btnBrowse, &QPushButton::clicked, this, &AntiSplitDialog::handleBrowseOutput);
    outputLayout->addWidget(btnBrowse);
    
    layout->addWidget(outputGroup);
    
    // Options
    m_CheckSign = new QCheckBox(tr("Sign output APK after merging"), this);
    m_CheckSign->setChecked(true);
    layout->addWidget(m_CheckSign);
    
    // Architecture selection
    auto archLayout = new QHBoxLayout();
    auto archLabel = new QLabel(tr("Target Architecture:"), this);
    archLayout->addWidget(archLabel);
    
    m_ComboArch = new QComboBox(this);
    m_ComboArch->addItem(tr("All (Universal)"), "all");
    m_ComboArch->addItem(tr("arm64-v8a (64-bit ARM)"), "arm64-v8a");
    m_ComboArch->addItem(tr("armeabi-v7a (32-bit ARM)"), "armeabi-v7a");
    m_ComboArch->addItem(tr("x86_64 (64-bit Intel)"), "x86_64");
    m_ComboArch->addItem(tr("x86 (32-bit Intel)"), "x86");
    m_ComboArch->setCurrentIndex(1); // Default to arm64-v8a (most common)
    m_ComboArch->setToolTip(tr("Select target architecture. Choosing a specific architecture reduces APK size and avoids split APK issues."));
    archLayout->addWidget(m_ComboArch);
    archLayout->addStretch();
    layout->addLayout(archLayout);
    
    return layout;
}

void AntiSplitDialog::handleAddApk()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
                                                       tr("Select APK files"),
                                                       QString(),
                                                       tr("Android APK Files (*.apk)"));
    for (const QString &file : files) {
        // Check for duplicates
        bool exists = false;
        for (int i = 0; i < m_ListFiles->count(); ++i) {
            if (m_ListFiles->item(i)->data(Qt::UserRole).toString() == file) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            QFileInfo info(file);
            auto item = new QListWidgetItem(info.fileName());
            item->setData(Qt::UserRole, file);
            item->setToolTip(file);
            m_ListFiles->addItem(item);
        }
    }
    updateButtons();
}

void AntiSplitDialog::handleAddXapk()
{
    QString file = QFileDialog::getOpenFileName(this,
                                                 tr("Select XAPK file"),
                                                 QString(),
                                                 tr("XAPK Files (*.xapk *.apks *.apkm)"));
    if (!file.isEmpty()) {
        // Check for duplicates
        bool exists = false;
        for (int i = 0; i < m_ListFiles->count(); ++i) {
            if (m_ListFiles->item(i)->data(Qt::UserRole).toString() == file) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            QFileInfo info(file);
            auto item = new QListWidgetItem(info.fileName() + " [XAPK]");
            item->setData(Qt::UserRole, file);
            item->setToolTip(file);
            m_ListFiles->addItem(item);
        }
    }
    updateButtons();
}

void AntiSplitDialog::handleRemoveSelected()
{
    QList<QListWidgetItem*> selected = m_ListFiles->selectedItems();
    for (QListWidgetItem *item : selected) {
        delete item;
    }
    updateButtons();
}

void AntiSplitDialog::handleClearAll()
{
    m_ListFiles->clear();
    updateButtons();
}

void AntiSplitDialog::handleBrowseOutput()
{
    QString file = QFileDialog::getSaveFileName(this,
                                                 tr("Save merged APK as"),
                                                 QString(),
                                                 tr("Android APK File (*.apk)"));
    if (!file.isEmpty()) {
        if (!file.endsWith(".apk", Qt::CaseInsensitive)) {
            file += ".apk";
        }
        m_EditOutput->setText(file);
    }
}

void AntiSplitDialog::updateButtons()
{
    bool hasFiles = m_ListFiles->count() > 0;
    bool hasSelection = !m_ListFiles->selectedItems().isEmpty();
    bool hasOutput = !m_EditOutput->text().isEmpty();
    
    m_BtnRemove->setEnabled(hasSelection);
    m_BtnClear->setEnabled(hasFiles);
    m_ButtonBox->button(QDialogButtonBox::Ok)->setEnabled(hasFiles && hasOutput);
}

QStringList AntiSplitDialog::inputFiles() const
{
    QStringList files;
    for (int i = 0; i < m_ListFiles->count(); ++i) {
        files << m_ListFiles->item(i)->data(Qt::UserRole).toString();
    }
    return files;
}

QString AntiSplitDialog::outputFile() const
{
    return m_EditOutput->text();
}

bool AntiSplitDialog::signApk() const
{
    return m_CheckSign->isChecked();
}

QString AntiSplitDialog::targetArchitecture() const
{
    return m_ComboArch->currentData().toString();
}
