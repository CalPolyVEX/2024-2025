TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += ../../../local/include
INCLUDEPATH += ../../../tensorflow
INCLUDEPATH += ../../../tensorflow/bazel-tensorflow/external/eigen_archive
INCLUDEPATH += ../../../tensorflow/bazel-tensorflow/external/protobuf/src
INCLUDEPATH += ../../../tensorflow/bazel-genfiles
INCLUDEPATH += ../../../tensorflow/bazel-tensorflow/external/nsync/public

LIBS += -L../../../tensorflow/bazel-bin/tensorflow -ltensorflow_cc -ltensorflow_framework

SOURCES += main.cpp
