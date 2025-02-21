all: libpublic.so libmyapi.so

# 如果依赖的文件发生变化就要重新编译
libpublic.so: public.h public.cpp
	g++ -fPIC -shared -o libpublic.so public.cpp
libmyapi.so: myapi.h myapi.cpp
	g++ -fPIC -shared -o libmyapi.so myapi.cpp

clean:
	rm -f libpublic.so libmyapi.so