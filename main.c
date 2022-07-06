#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>

int main(int argc, char **argv) {
    const char *path = "media/maze.png";
    int fd = open(path, O_RDONLY);
    if(fd == -1) err(1, "Failed to open %s", path);

    close(fd);
    return 0;
}
