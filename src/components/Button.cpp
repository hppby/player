//
// Created by LOOG LS on 24-9-6.
//

#include "Button.h"


Button::Button(QWidget *parent) : QPushButton(parent) {
    setStyleSheet("QPushButton{border:2px solid rgb(255,255,255);border-radius:4px;background-color:rgb(255,255,255);color:rgb(0,0,0);}");
}
Button::Button(const QString &text, QWidget *parent) : QPushButton(text, parent) {
    setStyleSheet("QPushButton{border:2px solid rgb(255,255,255);border-radius:4px;background-color:rgb(255,255,255);color:rgb(0,0,0);}");
    this->setText(text);
}


