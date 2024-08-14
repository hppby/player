//
// Created by LOOG LS on 2024/7/5.
//

#include "VideoPage.h"
#include "../db/SqlVideo.h"
#include "../network/HttpClient.h"
#include "../download/HandleMagnet.h"


#include <iostream>
#include <string>
#include <curl/curl.h> // 引入 libcurl 库
#include <libxml/HTMLparser.h> // 引入 libxml2 库

VideoPage::VideoPage(QWidget *parent) {

    SqlVideo *sqlVideo = new SqlVideo(this);
//    HttpClient("https://baidu.com").debug(true).success([](const QString &response) {
//        qDebug() << "===response===" << response;
//    }).fail([](const QString &error, int errorCode) {
//        qDebug() << "===error===" << error;
//        qDebug().noquote() << error << errorCode;
//    }).get();

    std::string url = "https://cn.bing.com/search?q=%E7%94%B5%E5%BD%B1%E5%A4%A9%E5%A0%82&PC=U316&FORM=CHROMN";
    std::string pageContent = DownloadPage(url);
    ParseHtml(pageContent);

    HandleMagnet *handleMagnet = new HandleMagnet(this);
}

VideoPage::~VideoPage() {

}

void VideoPage::initUI() {

    // 写一个爬虫工具


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


// 解析 HTML 页面
void VideoPage::ParseHtml(const std::string &htmlContent) {
    htmlDocPtr doc;
    xmlNodePtr cur;

    doc = htmlReadDoc((xmlChar *) htmlContent.c_str(), NULL, NULL,
                      HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == NULL) {
        std::cerr << "Failed to parse HTML document." << std::endl;
        return;
    }



    // 遍历文档节点
    for (cur = xmlDocGetRootElement(doc)->children; cur != NULL; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name, (const xmlChar *) "body") == 0) {
            PrintNode(cur->doc, 1);

        }
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

void VideoPage::PrintNode(htmlDocPtr doc, int level) {
    xmlNodePtr cur;
    // 遍历文档节点
    for (cur = xmlget(doc)->children; cur != NULL; cur = cur->next) {

        if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name, (const xmlChar *) "a") == 0) {
            xmlChar *link = xmlGetProp(cur, (const xmlChar *) "href");
            if (link) {
                std::cout << "Found link: " << (char *) link << std::endl;
                xmlFree(link);
            }
        } else if (xmlStrcmp(cur->name, (const xmlChar *) "div") == 0
                   || xmlStrcmp(cur->name, (const xmlChar *) "main") == 0
                   || xmlStrcmp(cur->name, (const xmlChar *) "ol") == 0
                   || xmlStrcmp(cur->name, (const xmlChar *) "li") == 0
                   || xmlStrcmp(cur->name, (const xmlChar *) "h2") == 0
                ) {
            PrintNode(cur->doc, level);
        }

    }
}

