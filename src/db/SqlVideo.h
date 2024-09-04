//
// Created by LOOG LS on 2024/7/5.
//

#ifndef PLAYER_SQLVIDEO_H
#define PLAYER_SQLVIDEO_H


#include <QObject>
#include <sqlite_orm/sqlite_orm.h>

using namespace sqlite_orm;

class SqlVideo : public QObject {
QOBJECT_H

public:
    explicit SqlVideo(QObject *parent);

    ~SqlVideo();

    struct VideoInfo{
        int id;
        std::string title;
        std::string path;
    };

    void addVideo(VideoInfo videoInfo);


private:
    auto getStorage();

//    decltype(internal::storage_base) storage;

//    internal::storage_base storage;
};


#endif //PLAYER_SQLVIDEO_H
