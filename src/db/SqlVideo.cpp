//
// Created by LOOG LS on 2024/7/5.
//

#include "SqlVideo.h"
#include <QDebug>
#include <sqlite_orm/sqlite_orm.h>

using namespace sqlite_orm;


static auto getStorage() {
    using namespace DataModel;
    using namespace sqlite_orm;

    auto storage = make_storage("db.sqlite",
                                                 make_table("videos",
                                           make_column("id", &VideoInfo::id, primary_key().autoincrement()),
                                           make_column("title", &VideoInfo::title),
                                           make_column("path", &VideoInfo::path),
                                           make_column("type", &VideoInfo::type),
                                           make_column("year", &VideoInfo::year)
                                )
    );
    return storage;
}

SqlVideo::SqlVideo(QObject *parent) {

   auto storage = getStorage();

    try {
        storage.sync_schema(); // 尝试同步模式，这将会创建表（如果尚不存在）
        qDebug()  << "Table created successfully" ;
    } catch (const std::exception &e) {
        qDebug()  << "Error: " << e.what() ;
    }

    //执行插入操作
//    auto insertedId = storage.insert(user);

}

SqlVideo::~SqlVideo() {

}

void SqlVideo::addVideo(VideoInfo videoInfo) {
    auto storage = getStorage();
    storage.insert(videoInfo);
}

QList<VideoInfo> SqlVideo::getVideos(VideoInfo query, int pageNum) {

    auto storage = getStorage();

    // 分页查询 query 动态化参数
    auto list = storage.select(
            columns(&VideoInfo::title, &VideoInfo::path, &VideoInfo::type, &VideoInfo::year),
                          where(like(&VideoInfo::title, "%" + query.title + "%")),
                          limit(10, offset((pageNum - 1) * 10))
                          );

    QList<DataModel::VideoInfo> myList;

    for(auto &t : list) {
        auto &title = std::get<0>(t);
        auto &path = std::get<1>(t);
        auto &type = std::get<2>(t);
        auto &year = std::get<3>(t);

        qDebug() << "===title===" << title;


        myList.append(VideoInfo{
                .title=std::string(title),
                .path=std::string(path),
                .type=std::string(type),
                .year=std::string(year)
        });
    }
    return myList;
}


