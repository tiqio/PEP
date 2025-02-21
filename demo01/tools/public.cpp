#include "public.h"

using namespace std;

void func() {
    cout << "public库打印信息" << endl;
}

void AA::show() {
    cout << "AA类打印信息" << endl;
}