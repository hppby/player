cmake_minimum_required(VERSION 3.5)

message("====开始编译player了=====")
set(CMAKE_TOOLCHAIN_FILE "/Users/loog/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")


project(player VERSION 0.1 LANGUAGES CXX)

#set(CMAKE_TOOLCHAIN_FILE /Users/loog/vcpkg/scripts/buildsystems/vcpkg.cmake
#        CACHE STRING "Vcpkg toolchain file")


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


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
        src/videoplayer/videoplayer.h
        src/videoplayer/videoplayer.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(player
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        images.qrc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET player APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(player SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")

    else()
        add_executable(player
            ${PROJECT_SOURCES}
        )
    endif()
endif()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
    set(CMAKE_PROJECT_NAME "hppby")
    set(CMAKE_MACOSX_BUNDLE_IDENTIFIER "com.hppby")

#    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/opencv/include/opencv4)
#    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/opencv/lib)
#    link_libraries(opencv_core
#            opencv_imgproc
#            opencv_highgui
#            opencv_video
#            opencv_videoio)




#    include_directories(${PROJECT_SOURCE_DIR}/lib/vlc/mac/include)
#    link_directories(${PROJECT_SOURCE_DIR}/lib/vlc/mac/lib)
#
#    target_link_libraries(player PRIVATE libvlc.dylib)
#    target_link_libraries(player PRIVATE libvlccore.dylib)


#    link_libraries(libvlc
#                libvlccore)
#    find_library(libvlc
#                NAMES vlc
#                PATHS ${PROJECT_SOURCE_DIR}/lib/vlc/mac/lib/libvlc)

#    message("libvlc====${libvlc}")




endif()

target_link_libraries(player PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(player PRIVATE Qt6::Multimedia)
target_link_libraries(player PRIVATE Qt6::MultimediaWidgets)



# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.player)
endif()
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
