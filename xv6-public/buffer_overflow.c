#include "types.h"
#include "user.h"
#include "fcntl.h"

void foo()
{
    printf(1, "SECRET_STRING");
}


void vulnerable_func(char *payload)
{
    char buffer[4];
    printf(1, "Written\n");
    strcpy(buffer, payload);

    void *adr = __builtin_extract_return_addr(__builtin_return_address(0));
    printf(1, "Return Address = %p\n", adr);


    // char *addr = buffer;
    // for (int i = 0; i < 10; i++) {
    //     printf(1, "%p ", *(char *)(addr + i));
    // }
    // printf(1, "\n");
    // printf(1, "Foo Addr = %p\n", &foo);
}

int main(int argc, char *argv[])
{
    int fd;

    printf(1, "Main Adde = %p\n", main);
    // printf(1, "fd Adde  = %p\n", fd);
    printf(1, "Foo Addr = %p\n", foo);
    printf(1, "Vulnerable Func Addr = %p\n", vulnerable_func);

    fd = open("payload", O_RDONLY);
    char payload[100];

    read(fd, payload, 100);

    vulnerable_func(payload);

    close(fd);
    exit();
}
