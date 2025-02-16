#ifndef VGMTRANS_HEXVIEW_H
#define VGMTRANS_HEXVIEW_H

#include <QtWidgets>

class VGMFile;

class HexView : public QAbstractScrollArea {
    Q_OBJECT

public:
    HexView(VGMFile *vgmfile, QWidget *parent = nullptr);
    ~HexView() override;

private:
    VGMFile *vgmfile;
    int mLineHeight;
    int mLinesPerScreen;
    int mLineBaseline;

//    void drawLineColor(QPainter &painter, QFontMetrics &fontMetrics, uint32_t line);


protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

};


#endif //VGMTRANS_HEXVIEW_H
