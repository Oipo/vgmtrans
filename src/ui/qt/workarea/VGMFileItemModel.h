#ifndef VGMTRANS_VGMFILEITEMMODEL_H
#define VGMTRANS_VGMFILEITEMMODEL_H

#include <QAbstractItemModel>

class VGMFile;

class VGMFileItemModel : public QAbstractItemModel {
    Q_OBJECT

public:
    VGMFileItemModel(VGMFile *file);

private:
    VGMFile* vgmfile;

public:
    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
};

#endif //VGMTRANS_VGMFILEITEMMODEL_H
