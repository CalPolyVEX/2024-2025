TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += ../tensorflow
INCLUDEPATH += ../tensorflow/bazel-tensorflow/external/eigen_archive
INCLUDEPATH += ../tensorflow/bazel-tensorflow/external/protobuf/src
INCLUDEPATH += ../tensorflow/bazel-genfiles

LIBS += -L../tensorflow/bazel-bin/tensorflow -ltensorflow_cc

SOURCES += main.cpp
