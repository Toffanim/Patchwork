CC=gc
CXX=g++
CXXFLAGS= -std=c++11 -Wall -DBOOST_SYSTEM_NO_DEPRECATED
LIBS= -lboost_system -lSDL2 -lpthread
INCLUDES = -I Include -I Shapes

all : server client tests

server : Server/chat_server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) Server/chat_server.cpp $(LIBS) -o Debug/server

client : Client/chat_client.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) Client/chat_client.cpp $(LIBS) -o Debug/client

tests : ShapesTests/ShapesTests.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) ShapesTests/ShapesTests.cpp $(LIBS) -o Debug/tests


