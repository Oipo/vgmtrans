//
// Created by Mike on 8/31/14.
//

#include <QAbstractListModel>
#include <QEvent>
#include <QListView>

#ifndef __VGMFileListView_H_
#define __VGMFileListView_H_


class VGMFileListViewModel : public QAbstractListModel
{
    Q_OBJECT

public:
    VGMFileListViewModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount ( const QModelIndex & parent = QModelIndex() ) const override;

    [[nodiscard]] QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

public slots:
    void changedVGMFiles();
};



class VGMFileListView : public QListView
{
    Q_OBJECT

public:
    VGMFileListView(QWidget *parent = nullptr);

    void keyPressEvent(QKeyEvent* e) override;
//    void mouseDoubleClickEvent(QMouseEvent *event);
public slots:
    void doubleClickedSlot(QModelIndex);
};


#endif //__VGMFileListView_H_
