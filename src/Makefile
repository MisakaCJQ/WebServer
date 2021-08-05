http_server:main.o Epoll.o HttpProcessor.o MyHttpServer.o Socket.o ThreadPool.o TimingWheel.o
	g++ *.o -o http_server -pthread
main.o:main.cpp
	g++ -c main.cpp
Epoll.o:Epoll.cpp Epoll.h
	g++ -c Epoll.cpp
HttpProcessor.o:HttpProcessor.cpp HttpProcessor.h
	g++ -c HttpProcessor.cpp
MyHttpServer.o:MyHttpServer.cpp MyHttpServer.h
	g++ -c MyHttpServer.cpp
Socket.o:Socket.cpp Socket.h
	g++ -c Socket.cpp
ThreadPool.o:ThreadPool.cpp ThreadPool.h
	g++ -c ThreadPool.cpp
TimingWheel.o:TimingWheel.cpp TimingWheel.h
	g++ -c TimingWheel.cpp
clean:
	-rm *.o
	-rm http_server
