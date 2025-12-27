#ifndef BINARYSETTINGSWIDGET_H
#define BINARYSETTINGSWIDGET_H

#include <QWidget>

class BinarySettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BinarySettingsWidget(QWidget *parent = nullptr);
    
public slots:
    void save();
    void downloadAllTools();
};

#endif // BINARYSETTINGSWIDGET_H
