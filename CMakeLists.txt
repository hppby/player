cmake_minimum_required(VERSION 3.5)

message("====开始编译player了=====")
#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
#        CACHE STRING "Vcpkg toolchain file")

project(player VERSION 0.1 LANGUAGES CXX)


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -stdlib=libc++ -framework OpenGL -framework AppKit -framework Security -framework CoreFoundation -framework CoreVideo -framework CoreMedia -framework QuartzCore -framework CoreFoundation -framework VideoDecodeAcceleration -framework Cocoa -framework AudioToolbox -framework VideoToolbox -framework OpenCL ")

# 配置QT
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Multimedia)
find_package(Qt6 REQUIRED COMPONENTS MultimediaWidgets)


message("QT_VERSION_MAJOR====${QT_VERSION_MAJOR}")
message("CMAKE_CURRENT_SOURCE_DIR====${CMAKE_CURRENT_SOURCE_DIR}")

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        src/home/home.cpp
        src/home/home.h
        src/videoplayer/videoplayer.cpp
        src/videoplayer/videoplayer.h
        src/videodecoder/VideoDecoder.cpp
        src/videodecoder/VideoDecoder.h
        src/videodecoder/VideoDecoderThread.cpp
        src/videodecoder/VideoDecoderThread.h
        src/videodecoder/DecodeVideo.cpp
        src/videodecoder/DecodeVideo.h
        src/videodecoder/DecodeAudio.cpp
        src/videodecoder/DecodeAudio.h
        src/videodecoder/AudioDecoderThread.cpp
        src/videodecoder/AudioDecoderThread.h
        src/videodecoder/ReadFrameThread.cpp
        src/videodecoder/ReadFrameThread.h
        src/videodecoder/DecodeState.cpp
        src/videodecoder/DecodeState.h
        src/videodecoder/VideoPlayThread.cpp
        src/videodecoder/VideoPlayThread.h
        src/videodecoder/AudioPlayThread.cpp
        src/videodecoder/AudioPlayThread.h
        src/media/VideoPage.cpp
        src/media/VideoPage.h
        src/db/SqlVideo.cpp
        src/db/SqlVideo.h
        src/db/Movie.cpp
        src/db/Movie.h
        src/network/HttpClient.cpp
        src/network/HttpClient.h
        src/download/HandleMagnet.cpp
        src/download/HandleMagnet.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(player
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        images.qrc
    )
else()
    if(ANDROID)
        add_library(player SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(player
            ${PROJECT_SOURCES}
        )
    endif()
endif()

#if(APPLE)
##    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
#    set(CMAKE_PROJECT_NAME "hppby")
#    set(CMAKE_MACOSX_BUNDLE_IDENTIFIER "com.hppby")
#
#
#endif()
find_package(Opus CONFIG REQUIRED)
target_link_libraries(player PRIVATE Opus::opus)


target_link_libraries(player PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(player PRIVATE Qt6::Multimedia)
target_link_libraries(player PRIVATE Qt6::MultimediaWidgets)




# 配置ffmpeg
find_package(FFmpeg REQUIRED)
if(FFMPEG_FOUND)
    message("=====找到了 FFmpeg 的库文件=====")

    target_include_directories(player PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_directories(player PRIVATE ${FFMPEG_LIBRARY_DIRS})
    target_link_libraries(player PRIVATE  ${FFMPEG_LIBRARIES})

#    message("=====FFmpeg::opus ======${FFmpeg::opus}")
    message("=====FFMPEG_LIBRARIES=====${FFMPEG_LIBRARIES}")
    message("=====FFMPEG_LIBRARY_DIRS=====${FFMPEG_LIBRARY_DIRS}")

else()
    message("=====没有找到了 FFmpeg =====")
endif()

#add_library(player STATIC libopus.o)
target_link_libraries(
        player
        PRIVATE
        ${FFMPEG_LIBRARY_DIRS}/libavfilter.a
        ${FFMPEG_LIBRARY_DIRS}/libavformat.a
        ${FFMPEG_LIBRARY_DIRS}/libavcodec.a
        ${FFMPEG_LIBRARY_DIRS}/libswscale.a
        ${FFMPEG_LIBRARY_DIRS}/libavutil.a
        ${FFMPEG_LIBRARY_DIRS}/libswresample.a
        ${FFMPEG_LIBRARY_DIRS}/libopus.a

        #        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libavfilter.so
        #        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libavformat.so
        #        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libavcodec.so
        #        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libavutil.so
        #        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libswresample.so
        #        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libswscale.so
#        z #z库 ffmpeg用到
        libbz2.dylib
        libiconv.dylib
)

find_package(SDL2 REQUIRED)
target_link_libraries(
        player
        PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

find_package(SDL2_mixer CONFIG REQUIRED)
target_link_libraries(player PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)


find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(player PRIVATE unofficial::sqlite3::sqlite3)

find_package(SqliteOrm CONFIG REQUIRED)
target_link_libraries(player PRIVATE sqlite_orm::sqlite_orm)

find_package(httplib CONFIG REQUIRED)
target_link_libraries(player PRIVATE httplib::httplib)


find_package(LibtorrentRasterbar CONFIG REQUIRED)
target_link_libraries(player PRIVATE LibtorrentRasterbar::torrent-rasterbar)

find_package(LibXml2 REQUIRED)
target_link_libraries(player PRIVATE LibXml2::LibXml2)

find_package(CURL REQUIRED)
target_link_libraries(player PRIVATE CURL::libcurl)

set_target_properties(player PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS player
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(player)
endif()


message("=====加载结束=====")
