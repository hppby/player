//
// Created by LOOG LS on 2024/7/9.
//

#ifndef PLAYER_HANDLEMAGNET_H
#define PLAYER_HANDLEMAGNET_H

#include <QObject>

class HandleMagnet: public QObject {
QOBJECT_H

public:
    HandleMagnet(QObject *parent = nullptr);
    ~HandleMagnet();

    void handle(const QString &magnet);




};


#endif //PLAYER_HANDLEMAGNET_H
