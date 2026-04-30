#include <iostream>
#include "singelton.hpp"

using namespace std;

int main() 
{
   int* a = hrd41::Singleton<int>::GetInstance();

    return 0;
}
