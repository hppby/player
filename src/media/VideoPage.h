//
// Created by LOOG LS on 2024/7/5.
//

#ifndef PLAYER_VIDEOPAGE_H
#define PLAYER_VIDEOPAGE_H


#include <QWidget>

class VideoPage : public QWidget{
Q_OBJECT

public:
    explicit VideoPage(QWidget *parent = nullptr);
    ~VideoPage() override;

private:
    void initUI();


};


#endif //PLAYER_VIDEOPAGE_H
