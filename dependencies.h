#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>


struct editorConfig {
  struct termios orig_termios;
};
struct editorConfig E;

// Use Ctrl-Q to quit
#define CTRL_KEY(k) ((k) & 0x1f)
/*
Disable raw Mode and restore terminal's original attributes when our program exits.
*/
struct termios orig_termios;

void die(const char *s) {
  // Clear the screen on exit 
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

// Set terminal to original configs (Default)

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

/*
Drawing a column of tildes(~) on
The left handside of the screen like vim does.
*/
void editorDrawRows()
{
  int y;
  for(y = 0; y < 24; y++)
  {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
  
}


void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);
  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}


// Refactor keyboard input

/*
This function for one keypress, and return it.
*/
char editorReadKey()
{
  int nread;
  char c;
  while((nread = read(STDIN_FILENO, &c, 1)) != 1)
  {
    if (nread == -1 && errno != EAGAIN) die("read");

  }
  return c;
}

/* Set window size, the easy way*/


/*
This Function waits for a keypress, and then handles it
*/
void editorProcessKeypress()
{
  char c = editorReadKey();

  switch (c)
  {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
  }
}

// Clear the screen

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3);

  /*
  We got 4 bytes in our function
  The first byte is \x1b is the escape character, or 27 in decimal.
  The other bytes rae [2J
  J command Erase in display
  It say to clear the entire screen
  */
}

