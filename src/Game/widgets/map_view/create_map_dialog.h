#pragma once
#include <QDialog>
#include <memory>

class QLineEdit;
class QLabel;
class QPushButton;
class QSpinBox;

class CreateMapDialog : public QDialog {
    Q_OBJECT
public:
    explicit CreateMapDialog(QWidget* parent = nullptr);
    ~CreateMapDialog() override;

    // Получить данные для создания карты
    QString getMapName() const;
    int getChunkWidth() const;
    int getChunkHeight() const;
    int getTileSize() const;

private:
    class Private;
    std::unique_ptr<Private> d;

    void setupUI();
    void onCopyIdClicked();
};
