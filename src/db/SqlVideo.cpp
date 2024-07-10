//
// Created by LOOG LS on 2024/7/5.
//

#include "SqlVideo.h"
#include "sqlite3.h"
#include "Movie.h"
#include <QDebug>
#include <sqlite_orm/sqlite_orm.h>

using namespace sqlite_orm;

SqlVideo::SqlVideo(QObject *parent) {

    auto storage = make_storage("db.sqlite",
                                make_table("users",
                                           make_column("id", &User::id, primary_key().autoincrement()),
                                           make_column("first_name", &User::firstName),
                                           make_column("last_name", &User::lastName),
                                           make_column("birth_date", &User::birthDate),
                                           make_column("image_url", &User::imageUrl),
                                           make_column("type_id", &User::typeId)),
                                make_table("user_types",
                                           make_column("id", &UserType::id, primary_key().autoincrement()),
                                           make_column("name", &UserType::name, default_value("name_placeholder"))));

    try {
        storage.sync_schema(); // 尝试同步模式，这将会创建表（如果尚不存在）
        qDebug()  << "Table created successfully" ;
    } catch (const std::exception &e) {
        qDebug()  << "Error: " << e.what() ;
    }

    // user 对象
    User user{-1, "Jonh", "Doe", 664416000,
                         std::make_unique<std::string>("url_to_heaven"), 3 };

    //执行插入操作

    auto insertedId = storage.insert(user);
    //  insertedId = 8
    qDebug()  << "insertedId = " << insertedId;
    qDebug() << "===storage===" << &storage;
}

SqlVideo::~SqlVideo() {

}
