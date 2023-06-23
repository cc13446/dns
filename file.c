//
// Created by cc on 23-6-23.
//
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "file.h"
#include "debug.h"

char* readAllContent(char* filePath) {
    ddbg("Read from file %s", filePath)
    int fd = open(filePath, O_RDONLY);
    if (fd < 0) {
        printf("Open file %s fail, code %d %s", filePath, errno, strerror(errno));
        exit(errno);
    }
    ddbg("Open file success")
    long length = lseek(fd,0,SEEK_END);
    lseek(fd, 0, SEEK_SET);
    ddbg("The file length is %ld", length)

    char* fileContent = (char *) malloc(length * sizeof(char));
    long readBytes = read(fd, fileContent, length * sizeof(char));
    ddbg("Read from %s %ld bytes", filePath, readBytes)
    ddbg("The file content is : \n%s", fileContent)
    return fileContent;
}
