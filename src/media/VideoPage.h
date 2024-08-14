//
// Created by LOOG LS on 2024/7/5.
//

#ifndef PLAYER_VIDEOPAGE_H
#define PLAYER_VIDEOPAGE_H


#include <QWidget>
#include <libxml/tree.h>

class VideoPage : public QWidget{
Q_OBJECT

public:
    explicit VideoPage(QWidget *parent = nullptr);
    ~VideoPage() override;

private:
    void initUI();


//    size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);

    std::string DownloadPage(const std::string &url);

    void ParseHtml(const std::string &htmlContent);

    void PrintNode(_xmlDoc *doc, int level);
};


#endif //PLAYER_VIDEOPAGE_H
