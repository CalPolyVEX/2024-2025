test1:
	g++ -std=c++11 -o test_infer -I/opt/ros/kinetic/include -I/usr/local/include/tf_js -I/usr/local/include/eigen3 -g -Wall -D_DEBUG -Wshadow -Wno-sign-compare -w  -L/usr/local/lib/libtensorflow_cc `pkg-config --cflags --libs protobuf` -ltensorflow_cc test_infer.cpp

default:
	g++ test_infer.cpp -I/opt/ros/kinetic/include -L/opt/ros/kinetic/lib -Wl,-rpath,/opt/ros/kinetic/lib -ltensorflow -lroscpp -lrosconsole -lrostime -lroscpp_serialization -lboost_system -lboost_thread -lpthread -lactionlib -Wall -Wextra 

default1:
	g++ test_infer.cpp -I/opt/ros/kinetic/include -L/opt/ros/kinetic/lib -lroscpp -lrosconsole -lrostime -lroscpp_serialization -lboost_system -lboost_thread -lpthread -lactionlib -Wall -Wextra 

