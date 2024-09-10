//
// Created by LOOG LS on 2024/7/5.
//

#include "VideoPage.h"

#include "../network/HttpClient.h"
#include "../download/HandleMagnet.h"


#include <iostream>
#include <string>
#include <curl/curl.h> // 引入 libcurl 库
#include <libxml/HTMLparser.h> // 引入 libxml2 库
#include <pugiconfig.hpp>
#include <pugixml.hpp>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QTableWidget>

using namespace DataModel;

VideoPage::VideoPage(QWidget *parent) {

    sql_video = new SqlVideo(this);
//    HttpClient("https://baidu.com").debug(true).success([](const QString &response) {
//        qDebug() << "===response===" << response;
//    }).fail([](const QString &error, int errorCode) {
//        qDebug() << "===error===" << error;
//        qDebug().noquote() << error << errorCode;
//    }).get();



//    HandleMagnet *handleMagnet = new HandleMagnet(this);


//    getSearchPage(QString("https://www.dyttcn.com/"));

    initUI();
}

VideoPage::~VideoPage() {

}

void VideoPage::initUI() {

    this->setFixedSize(600,400);
    // 写一个爬虫工具
    // 创建UI组件
    QLineEdit *name_line_edit = new QLineEdit();
    // 设置高 55 宽 200
    name_line_edit->setFixedHeight(33);
    name_line_edit->setFixedWidth(200);

    QHBoxLayout *h_layout_name = new QHBoxLayout();

    h_layout_name->addStretch(1);
    h_layout_name->addWidget(name_line_edit);
    h_layout_name->setAlignment(name_line_edit, Qt::AlignCenter);
h_layout_name->addStretch(1);

    QTableWidget *m_table=new QTableWidget();//创建表格
    QHBoxLayout *h_layout_table = new QHBoxLayout();
    h_layout_table->addWidget(m_table);

    QVBoxLayout *v_layout = new QVBoxLayout(this);
    v_layout->addLayout(h_layout_name);
    v_layout->addLayout(h_layout_table);


    m_table->setRowCount(10);//设置行数
    QStringList column_name = {"名称","类型","年份","地址", "操作"};
    m_table->setColumnCount(column_name.length());//设置列数
    m_table->setHorizontalHeaderLabels(column_name);//一次添加很多
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);


    QList<VideoInfo> list = sql_video->getVideos(VideoInfo{
        .title=std::string(""),
        .path=std::string("h")
},1);

    for (int i = 0; i < list.size(); i++) {
        DataModel::VideoInfo videoInfo = list.at(i);

        m_table->setItem(i,0,new QTableWidgetItem(QString::fromStdString(videoInfo.title)));
        m_table->setItem(i,1,new QTableWidgetItem(QString::fromStdString(videoInfo.type)));
        m_table->setItem(i,2,new QTableWidgetItem(QString::fromStdString(videoInfo.year)));
        m_table->setItem(i,3,new QTableWidgetItem(QString::fromStdString(videoInfo.path)));

        QPushButton *btn = new QPushButton();
        btn->setText("查看");
        m_table->setCellWidget(i,column_name.length()-1,btn);
    }
}

void VideoPage::getRootSearchPage() {
    std::string url = "https://cn.bing.com/search?q=sql&PC=U316&FORM=CHROMN";
    std::string pageContent = DownloadPage(url);
    QList<QString> list =  parseRootSearchHtml(pageContent);


//    for (int i = 0; i < list.size(); i++) {
//
//    }
//QString path = list.at(0);

    getSearchPage(QString("https://www.dyttcn.com/"));
}

void VideoPage::getSearchPage(QString url) {
    std::string pageContent = DownloadPage(url.toStdString());
    QList<DataModel::VideoInfo> list =  parseSearchHtml(pageContent, url);

    for (int i = 0; i < list.size(); i++) {
        DataModel::VideoInfo videoInfo = list.at(i);
//        qDebug() << "===videoInfo===" << videoInfo.title << videoInfo.path;
        sql_video->addVideo(videoInfo);
//        sql_video->update(videoInfo);
//        sql_video->deleteById(videoInfo.id);
//        sql_video->selectAll();
//        sql_video->selectById(videoInfo.id);
//        sql_video->selectByTitle(videoInfo.title);
    }
}





// 定义一个回调函数，用于接收从服务器返回的数据
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    size_t totalSize = size * nmemb;
    userp->append((char *) contents, totalSize);
    return totalSize;
}

// 下载页面内容
std::string VideoPage::DownloadPage(const std::string &url) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return readBuffer;
}


// 解析 搜索 HTML 页面
QList<QString> VideoPage::parseRootSearchHtml(std::string htmlContent) {

    htmlDocPtr doc;
    xmlNodePtr cur;

    doc = htmlReadDoc((xmlChar *) htmlContent.c_str(), NULL, NULL,
                      HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == NULL) {
        std::cerr << "Failed to parse HTML document." << std::endl;
        return QList<QString>();
    }

    // XPath 表达式，用于查找 h2 标签内部的 a 标签 以及a 标签里面的文本
    const char* xpathExpr = "//h2/a";
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr xpathResult = xmlXPathEvalExpression((xmlChar*)xpathExpr, xpathCtx);

    QList<QString> myList;

    if (xpathResult != nullptr && xpathResult->type == XPATH_NODESET) {
        xmlNodeSetPtr nodeSet = xpathResult->nodesetval;
        for (int i = 0; i < nodeSet->nodeNr; ++i) {
            xmlNodePtr node = nodeSet->nodeTab[i];
            const char* href = (const char*)xmlGetProp(node, (const xmlChar*)"href");
            const char* text = (const char*)xmlNodeGetContent(node);

            std::cout << "H2 -> A: Href: " << href << ", Text: " << text << std::endl;
            myList.append(QString(href));
            xmlFree((void*)href); // 释放内存
            xmlFree((void*)text); // 释放内存
        }
    } else {
        std::cerr << "Failed to evaluate XPath expression." << std::endl;
    }

    qDebug() << "===myList===" << myList;

    // 清理资源
    xmlXPathFreeObject(xpathResult);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return myList;
}

// 解析 结果 HTML 页面
QList<DataModel::VideoInfo> VideoPage::parseSearchHtml(std::string htmlContent, QString domainName)  {



    htmlDocPtr doc;
    xmlNodePtr cur;

    doc = htmlReadDoc((xmlChar *) htmlContent.c_str(), NULL, NULL,
                      HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == NULL) {
        std::cerr << "Failed to parse HTML document." << std::endl;
        return QList<DataModel::VideoInfo>();
    }

    // XPath 表达式，用于查找 h2 标签内部的 a 标签 以及a 标签里面的文本
    const char* xpathExpr = "//ul/li/a";
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr xpathResult = xmlXPathEvalExpression((xmlChar*)xpathExpr, xpathCtx);

    QList<DataModel::VideoInfo> myList;

    if (xpathResult != nullptr && xpathResult->type == XPATH_NODESET) {
        xmlNodeSetPtr nodeSet = xpathResult->nodesetval;
        for (int i = 0; i < nodeSet->nodeNr; ++i) {
            xmlNodePtr node = nodeSet->nodeTab[i];
            const char* href = (const char*)xmlGetProp(node, (const xmlChar*)"href");
            const char* tmp_title = (const char*)xmlGetProp(node, (const xmlChar*)"title");

            if (href && tmp_title) {

                std::string full_title = std::string(tmp_title);

                // 查找各个子串的位置
                std::size_t start = 0;
                std::size_t end_year = full_title.find("年");
                std::size_t start_type = end_year + 3;
                std::size_t end_type = full_title.find("《");
                std::size_t start_title = end_type + 3;
                std::size_t end_title = full_title.find("》");

// 提取子串
                std::string year = full_title.substr(start, end_year - start);
                std::string type = full_title.substr(start_type, end_type - start_type);
                std::string title = full_title.substr(start_title, end_title - start_title);

                myList.append(DataModel::VideoInfo{
                        .title=std::string(title),
                        .path= domainName.toStdString() + std::string(href),
                        .type=std::string(type),
                        .year=std::string(year),
                });
            }

            xmlFree((void*)href); // 释放内存
            xmlFree((void*)tmp_title); // 释放内存
        }
    } else {
        std::cerr << "Failed to evaluate XPath expression." << std::endl;
    }

//    qDebug() << "===myList===" << myL;

    // 清理资源
    xmlXPathFreeObject(xpathResult);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return myList;
}

