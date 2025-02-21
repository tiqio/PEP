# 开发框架头文件路径
PUBINCL = -I/home/dd/cpp/project/public

# 开发框架cpp文件名
PUBCPP = /home/dd/cpp/project/public/_public.cpp

# 编译选项
CFLAGS = -g

all: inetd

inetd: inetd.cpp
	g++ $(CFLAGS) -o inetd inetd.cpp $(PUBINCL) $(PUBCPP)
	mv -f inetd bin/.