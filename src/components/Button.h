//
// Created by LOOG LS on 24-9-6.
//

#ifndef PLAYER_BUTTON_H
#define PLAYER_BUTTON_H


#include <QPushButton>

class Button  : public QPushButton {
    QOBJECT_H

public:
    explicit Button(QWidget *parent = nullptr);
    explicit Button(const QString &text, QWidget *parent = nullptr);



};


#endif //PLAYER_BUTTON_H
