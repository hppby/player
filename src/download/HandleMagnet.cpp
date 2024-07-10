//
// Created by LOOG LS on 2024/7/9.
//

#include "HandleMagnet.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <QDebug>


#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/disabled_disk_io.hpp>
#include <libtorrent/write_resume_data.hpp> // for write_torrent_file


void HandleMagnet::handle(const QString &magnet) {

    lt::session_params params;
    params.disk_io_constructor = lt::disabled_disk_io_constructor;

    params.settings.set_int(lt::settings_pack::alert_mask, lt::alert_category::status | lt::alert_category::error);

    lt::session ses(std::move(params));

    lt::add_torrent_params atp = lt::parse_magnet_uri("magnet:?xt=urn:btih:c7d20fd4cb9307b64e48f20ca2a33f8441fc36de");
    atp.save_path = ".";
    atp.flags &= ~(lt::torrent_flags::auto_managed | lt::torrent_flags::paused);
    atp.file_priorities.resize(100, lt::dont_download);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    lt::torrent_handle h = ses.add_torrent(std::move(atp));

    for (;;) {
        std::vector<lt::alert *> alerts;
        ses.pop_alerts(&alerts);

        for (lt::alert *a: alerts) {
            std::cout << a->message() << std::endl;
            if (auto const *mra = lt::alert_cast<lt::metadata_received_alert>(a)) {
                std::cerr << "metadata received" << std::endl;
                auto const handle = mra->handle;
                std::shared_ptr<lt::torrent_info const> ti = handle.torrent_file();
                if (!ti) {
                    std::cerr << "unexpected missing torrent info" << std::endl;
                    goto done;
                }

                // in order to create valid v2 torrents, we need to download the
                // piece hashes. libtorrent currently only downloads the hashes
                // on-demand, so we would have to download all the content.
                // Instead, produce an invalid v2 torrent that's missing piece
                // layers
                if (ti->v2()) {
                    std::cerr << "found v2 torrent, generating a torrent missing piece hashes" << std::endl;
                }
                handle.save_resume_data(lt::torrent_handle::save_info_dict);
                handle.set_flags(lt::torrent_flags::paused);
            } else if (auto *rda = lt::alert_cast<lt::save_resume_data_alert>(a)) {
                // don't include piece layers
                rda->params.merkle_trees.clear();
                lt::entry e = lt::write_torrent_file(rda->params, lt::write_flags::allow_missing_piece_layer);
                std::vector<char> torrent;
                lt::bencode(std::back_inserter(torrent), e);
                std::ofstream out("/Users/loog/Desktop/text.js");

                QByteArray byteArray;
                for (char byte : torrent) {
                    byteArray.append(byte);
                }

                // 将QByteArray转换为十六进制字符串
                QString hexString;
                for (int i = 0; i < byteArray.size(); ++i) {
                    // 使用std::hex和std::setw来格式化每个字节为两位十六进制数
                    std::stringstream ss;
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byteArray[i]);
                    hexString += QString::fromStdString(ss.str());

                    // 如果不是最后一个字节，则添加分隔符（如空格或"-"）
                    if (i < byteArray.size() - 1) {
                        hexString += " ";
                    }
                }


                qDebug() << "===hexString===" << hexString;
                out.write(torrent.data(), int(torrent.size()));
                goto done;
            } else if (auto const *rdf = lt::alert_cast<lt::save_resume_data_failed_alert>(a)) {
                std::cerr << "failed to save resume data: " << rdf->message() << std::endl;
                goto done;
            }
        }
        ses.wait_for_alert(std::chrono::milliseconds(200));
    }
    done:
    std::cerr << "done, shutting down" << std::endl;
}

HandleMagnet::HandleMagnet(QObject *parent) : QObject(parent) {
    this->handle("123");
}

HandleMagnet::~HandleMagnet() {

}
