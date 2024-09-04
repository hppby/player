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

VideoPage::VideoPage(QWidget *parent) {

    sql_video = new SqlVideo(this);
//    HttpClient("https://baidu.com").debug(true).success([](const QString &response) {
//        qDebug() << "===response===" << response;
//    }).fail([](const QString &error, int errorCode) {
//        qDebug() << "===error===" << error;
//        qDebug().noquote() << error << errorCode;
//    }).get();



//    HandleMagnet *handleMagnet = new HandleMagnet(this);

    getSearchPage(QString("https://www.dyttcn.com/"));
}

VideoPage::~VideoPage() {

}

void VideoPage::initUI() {
    // 写一个爬虫工具
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
    QList<SqlVideo::VideoInfo> list =  parseSearchHtml(pageContent, url);
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
QList<SqlVideo::VideoInfo> VideoPage::parseSearchHtml(std::string htmlContent, QString domainName)  {

    htmlDocPtr doc;
    xmlNodePtr cur;

    doc = htmlReadDoc((xmlChar *) htmlContent.c_str(), NULL, NULL,
                      HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == NULL) {
        std::cerr << "Failed to parse HTML document." << std::endl;
        return QList<SqlVideo::VideoInfo>();
    }

    // XPath 表达式，用于查找 h2 标签内部的 a 标签 以及a 标签里面的文本
    const char* xpathExpr = "//ul/li/a";
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr xpathResult = xmlXPathEvalExpression((xmlChar*)xpathExpr, xpathCtx);

    QList<SqlVideo::VideoInfo> myList;

    if (xpathResult != nullptr && xpathResult->type == XPATH_NODESET) {
        xmlNodeSetPtr nodeSet = xpathResult->nodesetval;
        for (int i = 0; i < nodeSet->nodeNr; ++i) {
            xmlNodePtr node = nodeSet->nodeTab[i];
            const char* href = (const char*)xmlGetProp(node, (const xmlChar*)"href");
            const char* title = (const char*)xmlGetProp(node, (const xmlChar*)"title");

            if (href && title) {
                myList.append(SqlVideo::VideoInfo{
                        .title=std::string(title),
                        .path= domainName.toStdString() + std::string(href)
                });
            }

            xmlFree((void*)href); // 释放内存
            xmlFree((void*)title); // 释放内存
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

