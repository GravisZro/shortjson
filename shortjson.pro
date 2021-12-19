TEMPLATE = app
CONFIG += console
CONFIG += c++17
CONFIG += strict_c++
CONFIG += rtti_off

CONFIG -= app_bundle
CONFIG -= qt

#DEFINES += TOLERANT_JSON

SOURCES += tests.cpp \
  shortjson_strict.cpp \
  shortjson_tolerant.cpp


HEADERS += \
  shortjson.h
