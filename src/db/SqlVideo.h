//
// Created by LOOG LS on 2024/7/5.
//

#ifndef PLAYER_SQLVIDEO_H
#define PLAYER_SQLVIDEO_H


#include <QObject>
#include <sqlite_orm/sqlite_orm.h>

using namespace sqlite_orm;


namespace DataModel {

    struct VideoInfo{
        int id;
        std::string title;
        std::string path;
        std::string type;
        std::string year;

    };
}
using namespace DataModel;

class SqlVideo : public QObject {
QOBJECT_H

public:
    explicit SqlVideo(QObject *parent);

    ~SqlVideo();


    void addVideo(VideoInfo videoInfo);
    QList<VideoInfo> getVideos(VideoInfo query, int pageNum);


private:

};


#endif //PLAYER_SQLVIDEO_H
