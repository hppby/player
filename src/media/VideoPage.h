//
// Created by LOOG LS on 2024/7/5.
//

#ifndef PLAYER_VIDEOPAGE_H
#define PLAYER_VIDEOPAGE_H


#include <QWidget>
#include <libxml/tree.h>
#include "../db/SqlVideo.h"


class QTableWidget;

class VideoPage : public QWidget{
Q_OBJECT

public:
    explicit VideoPage(QWidget *parent = nullptr);
    ~VideoPage() override;



private:
    void initUI();
    QTableWidget *m_table;

    SqlVideo *sql_video;
//    size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);

    std::string DownloadPage(const std::string &url);



    void PrintNode(xmlNodePtr node, int level);

    void getRootSearchPage();

    void getSearchPage(QString url);

    QList<QString> parseRootSearchHtml(std::string htmlContent);
    QList<DataModel::VideoInfo> parseSearchHtml(std::string htmlContent,  QString domainName);
};


#endif //PLAYER_VIDEOPAGE_H
