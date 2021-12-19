TEMPLATE = app
CONFIG += console
CONFIG += c++17
CONFIG += strict_c++
CONFIG += rtti_off

CONFIG -= app_bundle
CONFIG -= qt

CONFIG += tolerant

SOURCES += tests.cpp

tolerant {
DEFINES += TOLERANT_JSON
SOURCES += shortjson_tolerant.cpp
} else {
SOURCES += shortjson_strict.cpp
}

HEADERS += \
  shortjson.h
