http_server:main.o MyHttpServer.o
	g++ main.o MyHttpServer.o -o http_server -pthread
main.o:main.cpp
	g++ -c main.cpp
MyHttpServer.o:MyHttpServer.cpp MyHttpServer.h
	g++ -c MyHttpServer.cpp
clean:
	-rm main.o
	-rm MyHttpServer.o
	-rm http_server
