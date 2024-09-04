//
// Created by LOOG LS on 2024/7/5.
//

#include "SqlVideo.h"
#include <QDebug>
#include <sqlite_orm/sqlite_orm.h>

using namespace sqlite_orm;



SqlVideo::SqlVideo(QObject *parent) {

    storage = make_storage("db.sqlite",
                                         make_table("videos",
                                                    make_column("id", &VideoInfo::id, primary_key().autoincrement()),
                                                    make_column("title", &VideoInfo::title),
                                                    make_column("path", &VideoInfo::path)
                                         )
   );;

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

void SqlVideo::addVideo(SqlVideo::VideoInfo videoInfo) {

}

auto SqlVideo::getStorage() {

    auto storage = make_storage("db.sqlite",
                                make_table("videos",
                                           make_column("id", &VideoInfo::id, primary_key().autoincrement()),
                                           make_column("title", &VideoInfo::title),
                                           make_column("path", &VideoInfo::path)
                                )
    );

    return storage;
}
