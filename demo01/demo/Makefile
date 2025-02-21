# 引入头文件
INCLUDEDIR=-I/home/dd/cpp/demo01/tools
LIBDIR=-L/home/dd/cpp/demo01/tools

all: demo01

demo01: demo01.cpp
	g++ -o demo01 demo01.cpp $(INCLUDEDIR) $(LIBDIR) -lpublic -lmyapi
clean: 
	rm -f demo01