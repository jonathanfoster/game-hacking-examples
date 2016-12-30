#include <stdio.h>
#include <windows.h>

static int value = 0;
static int increment = 1;
static int secret = 1337;

int main()
{
    int simple = 1234;

    while (true)
    {
        value += increment;
        printf("Value = %d, Increment = %d, secret = %d, simple = %d\n", value, increment, secret, simple);   
        system("pause");
    }

    system("pause");
    return 0;
}
