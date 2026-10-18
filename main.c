#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include "dependencies.h"




int main()
{
    enableRawMode();
    

    while(1)
    editorRefreshScreen();
    editorProcessKeypress();

    return 0;
}