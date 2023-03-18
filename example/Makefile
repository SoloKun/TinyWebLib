PROJECT_PATH=/home/kkk/桌面/webmuduo/
HEADER_PATH=-I ${PROJECT_PATH}/include/tiny_network/base \
			-I ${PROJECT_PATH}/include/tiny_network/net \
			-I ${PROJECT_PATH}/include/tiny_network/net/poller \
			-I ${PROJECT_PATH}/include/tiny_network/log \
			-I ${PROJECT_PATH}/include/tiny_network/time 
LIB_PATH= -L ${PROJECT_PATH}/lib -ltiny_network -lpthread


CFLAGS= -g -Wall ${LIB_PATH} ${HEADER_PATH}

all: Echo

Echo: echo.cpp
	g++ echo.cpp ${CFLAGS} -o Echo

clean:
	rm -r Echo
