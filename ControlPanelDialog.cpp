#include "ControlPanelDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>

ControlPanelDialog::ControlPanelDialog(MainWindow *mainWindow, InkCanvas *targetCanvas, QWidget *parent)
    : QDialog(parent), canvas(targetCanvas), selectedColor(canvas->getBackgroundColor()), mainWindowRef(mainWindow) {

    setWindowTitle(tr("Canvas Control Panel"));
    resize(400, 200);

    tabWidget = new QTabWidget(this);

    // === Tabs ===
    createBackgroundTab();
    tabWidget->addTab(backgroundTab, tr("Background"));
    if (mainWindowRef) {
        createPerformanceTab();
        tabWidget->addTab(performanceTab, tr("Performance"));
        createToolbarTab();
    }
    createButtonMappingTab();
    // === Buttons ===
    applyButton = new QPushButton(tr("Apply"));
    okButton = new QPushButton(tr("OK"));
    cancelButton = new QPushButton(tr("Cancel"));

    connect(applyButton, &QPushButton::clicked, this, &ControlPanelDialog::applyChanges);
    connect(okButton, &QPushButton::clicked, this, [=]() {
        applyChanges();
        accept();
    });
    connect(cancelButton, &QPushButton::clicked, this, &ControlPanelDialog::reject);

    // === Layout ===
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(buttonLayout);

    loadFromCanvas();
}


void ControlPanelDialog::createBackgroundTab() {
    backgroundTab = new QWidget(this);

    QLabel *styleLabel = new QLabel(tr("Background Style:"));
    styleCombo = new QComboBox();
    styleCombo->addItem(tr("None"), static_cast<int>(BackgroundStyle::None));
    styleCombo->addItem(tr("Grid"), static_cast<int>(BackgroundStyle::Grid));
    styleCombo->addItem(tr("Lines"), static_cast<int>(BackgroundStyle::Lines));

    QLabel *colorLabel = new QLabel(tr("Background Color:"));
    colorButton = new QPushButton();
    connect(colorButton, &QPushButton::clicked, this, &ControlPanelDialog::chooseColor);

    QLabel *densityLabel = new QLabel(tr("Density:"));
    densitySpin = new QSpinBox();
    densitySpin->setRange(10, 200);
    densitySpin->setSuffix(" px");
    densitySpin->setSingleStep(5);

    QGridLayout *layout = new QGridLayout(backgroundTab);
    layout->addWidget(styleLabel, 0, 0);
    layout->addWidget(styleCombo, 0, 1);
    layout->addWidget(colorLabel, 1, 0);
    layout->addWidget(colorButton, 1, 1);
    layout->addWidget(densityLabel, 2, 0);
    layout->addWidget(densitySpin, 2, 1);
    // layout->setColumnStretch(1, 1); // Stretch the second column
    layout->setRowStretch(3, 1); // Stretch the last row
}

void ControlPanelDialog::chooseColor() {
    QColor chosen = QColorDialog::getColor(selectedColor, this, tr("Select Background Color"));
    if (chosen.isValid()) {
        selectedColor = chosen;
        colorButton->setStyleSheet(QString("background-color: %1").arg(selectedColor.name()));
    }
}

void ControlPanelDialog::applyChanges() {
    if (!canvas) return;

    BackgroundStyle style = static_cast<BackgroundStyle>(
        styleCombo->currentData().toInt()
    );

    canvas->setBackgroundStyle(style);
    canvas->setBackgroundColor(selectedColor);
    canvas->setBackgroundDensity(densitySpin->value());
    canvas->update();
    canvas->saveBackgroundMetadata();

    // ✅ Apply button mappings back to MainWindow
    if (mainWindowRef) {
        for (const QString &button : holdMappingCombos.keys()) {
            mainWindowRef->setHoldMapping(button, holdMappingCombos[button]->currentText());
        }
        for (const QString &button : pressMappingCombos.keys()) {
            mainWindowRef->setPressMapping(button, pressMappingCombos[button]->currentText());
        }

        // ✅ Save to persistent settings
        mainWindowRef->saveButtonMappings();
    }
}

void ControlPanelDialog::loadFromCanvas() {
    styleCombo->setCurrentIndex(static_cast<int>(canvas->getBackgroundStyle()));
    densitySpin->setValue(canvas->getBackgroundDensity());
    selectedColor = canvas->getBackgroundColor();

    colorButton->setStyleSheet(QString("background-color: %1").arg(selectedColor.name()));

    if (mainWindowRef) {
        for (const QString &button : holdMappingCombos.keys()) {
            QString mode = mainWindowRef->getHoldMapping(button);
            int index = holdMappingCombos[button]->findText(mode);
            if (index >= 0) holdMappingCombos[button]->setCurrentIndex(index);
        }

        for (const QString &button : pressMappingCombos.keys()) {
            QString action = mainWindowRef->getPressMapping(button);
            int index = pressMappingCombos[button]->findText(action);
            if (index >= 0) pressMappingCombos[button]->setCurrentIndex(index);
        }
    }
}


void ControlPanelDialog::createPerformanceTab() {
    performanceTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(performanceTab);

    QCheckBox *previewToggle = new QCheckBox(tr("Enable Low-Resolution PDF Previews"));
    previewToggle->setChecked(mainWindowRef->isLowResPreviewEnabled());

    connect(previewToggle, &QCheckBox::toggled, mainWindowRef, &MainWindow::setLowResPreviewEnabled);

    QLabel *note = new QLabel(tr("Disabling this may improve dial smoothness on low-end devices."));
    note->setWordWrap(true);
    note->setStyleSheet("color: gray; font-size: 10px;");

    layout->addWidget(previewToggle);
    layout->addWidget(note);
    layout->addStretch();

    // return performanceTab;
}

void ControlPanelDialog::createToolbarTab(){
    toolbarTab = new QWidget(this);
    QVBoxLayout *toolbarLayout = new QVBoxLayout(toolbarTab);

    // ✅ Checkbox to show/hide benchmark controls
    QCheckBox *benchmarkVisibilityCheckbox = new QCheckBox(tr("Show Benchmark Controls"), toolbarTab);
    benchmarkVisibilityCheckbox->setChecked(mainWindowRef->areBenchmarkControlsVisible());
    toolbarLayout->addWidget(benchmarkVisibilityCheckbox);
    QLabel *benchmarkNote = new QLabel(tr("This will show/hide the benchmark controls on the toolbar. Press the clock button to start/stop the benchmark."));
    benchmarkNote->setWordWrap(true);
    benchmarkNote->setStyleSheet("color: gray; font-size: 10px;");
    toolbarLayout->addWidget(benchmarkNote);

    // ✅ Checkbox to show/hide color buttons
    QCheckBox *colorButtonsVisibilityCheckbox = new QCheckBox(tr("Show Color Buttons"), toolbarTab);
    colorButtonsVisibilityCheckbox->setChecked(mainWindowRef->areColorButtonsVisible());
    toolbarLayout->addWidget(colorButtonsVisibilityCheckbox);
    QLabel *colorButtonsNote = new QLabel(tr("This will show/hide the color buttons on the toolbar"));
    colorButtonsNote->setWordWrap(true);
    colorButtonsNote->setStyleSheet("color: gray; font-size: 10px;");
    toolbarLayout->addWidget(colorButtonsNote);

    QCheckBox *scrollOnTopCheckBox = new QCheckBox(tr("Scroll on Top after Page Switching"), toolbarTab);
    scrollOnTopCheckBox->setChecked(mainWindowRef->isScrollOnTopEnabled());
    toolbarLayout->addWidget(scrollOnTopCheckBox);
    QLabel *scrollNote = new QLabel(tr("Enabling this will make the page scroll to the top after switching to a new page."));
    scrollNote->setWordWrap(true);
    scrollNote->setStyleSheet("color: gray; font-size: 10px;");
    toolbarLayout->addWidget(scrollNote);
    toolbarLayout->addStretch();
    toolbarTab->setLayout(toolbarLayout);
    tabWidget->addTab(toolbarTab, tr("Features"));


    // Connect the checkbox
    connect(benchmarkVisibilityCheckbox, &QCheckBox::toggled, mainWindowRef, &MainWindow::setBenchmarkControlsVisible);
    connect(colorButtonsVisibilityCheckbox, &QCheckBox::toggled, mainWindowRef, &MainWindow::setColorButtonsVisible);
    connect(scrollOnTopCheckBox, &QCheckBox::toggled, mainWindowRef, &MainWindow::setScrollOnTopEnabled);

    
}


void ControlPanelDialog::createButtonMappingTab() {
    QWidget *buttonTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(buttonTab);

    QStringList buttons = {"LEFTSHOULDER", "RIGHTSHOULDER", "PADDLE2", "PADDLE4", "Y", "A", "B", "X", "LEFTSTICK", "START", "GUIDE"};
    QStringList dialModes = {"None", "PageSwitching", "ZoomControl", "ThicknessControl", "ColorAdjustment", "ToolSwitching", "PresetSelection", "PanAndPageScroll"};
    QStringList actions = {
        "None", "Toggle Fullscreen", "Toggle Dial", "Zoom 50%", "Zoom Out", "Zoom 200%",
        "Add Preset", "Delete Page", "Fast Forward", "Open Control Panel",
        "Red", "Blue", "Yellow", "Green", "Black", "White", "Custom Color"
    };

    for (const QString &button : buttons) {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(button));

        QComboBox *holdCombo = new QComboBox();
        holdCombo->addItems(dialModes);
        holdMappingCombos[button] = holdCombo;
        h->addWidget(new QLabel(tr("Hold:")));
        h->addWidget(holdCombo);

        QComboBox *pressCombo = new QComboBox();
        pressCombo->addItems(actions);
        pressMappingCombos[button] = pressCombo;
        h->addWidget(new QLabel(tr("Press:")));
        h->addWidget(pressCombo);

        layout->addLayout(h);
    }

    layout->addStretch();
    buttonTab->setLayout(layout);
    tabWidget->addTab(buttonTab, tr("Button Mapping"));
}
