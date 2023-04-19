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
    printf(1, "buffer Address = %p\n", &buffer);
    printf(1, "Return Address = %p\n", &adr);
}

int main(int argc, char *argv[])
{
    int fd;

    fd = open("payload", O_RDONLY);
    char payload[100];

    read(fd, payload, 100);

    printf(1, "%d\n", random());

    vulnerable_func(payload);

    close(fd);
    exit();
}
