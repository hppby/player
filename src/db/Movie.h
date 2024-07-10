//
// Created by LOOG LS on 2024/7/6.
//

#ifndef PLAYER_MOVIE_H
#define PLAYER_MOVIE_H


#include <QtCore/QObject>


struct User{
    int id;
    std::string firstName;
    std::string lastName;
    int birthDate;
    std::unique_ptr<std::string> imageUrl;
    int typeId;
};
struct UserType {
    int id;
    std::string name;
};

class Movie: public QObject {
QOBJECT_H

public:
    Movie();
    ~Movie();




};


#endif //PLAYER_MOVIE_H
