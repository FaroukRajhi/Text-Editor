#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "dependencies.h"




int main(int argc, char *argv[])
{
    enableRawMode();
    initEditor();
    if(argc >= 2)
    {
        editorOpen(argv[1]);
    } 
    
    editorSetStatusMessage("HELP: Ctrl-Q = quit");


    while(1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    

    return 0;
}