# Entering raw mode

read() and STDIN_FILENO come from <unistd.h> library.
Asking read() to read 1 byte from the standard input into variable c, and keep doing it until there are no more bytes to read.
read() returns the number of bytes that it read, and will return 0 when it reaches the end of a file.

**Press q to quit**

We will have to type a line of text that includes a 'q' in it, then press enter, at which point the while loop will stop and the program will exit.
Any characters after the 'q' will be lift unread on the input queue.

The termios functions describe a general terminal interface that is provided to control asynchronous communications ports.

```
#include <termios.h>

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);

``` 

tcgetattr()	Reads current terminal attributes into a struct termios
tcsetattr()	Writes modified attributes back to the terminal

* The struct termios structure

``` 
struct termios {
    tcflag_t c_iflag;   // Input modes
    tcflag_t c_oflag;   // Output modes
    tcflag_t c_cflag;   // Control modes (baud rate, data bits, parity)
    tcflag_t c_lflag;   // Local modes (echo, canonical mode, signals)
    cc_t     c_cc[NCCS]; // Control characters (e.g., VMIN, VTIME, VEOF)
};

```

struct termios, tcgetattr(), tcsetattr(), ECHO, and TCSAFLUSH all come from <termios.h>.

The ECHO feature causes each key we type to be printed to the terminal, so we can see what we're typing.
Terminal attributes can be read into a termios struct tcgetattr().
Modifying them, you can then apply them to the terminal using tcsetattr().
TCSAFLUSH argument specifies when to apply the change

ECHO is a bitflag, defined as 00000000000000000000000000001000 in binary. We use the bitwise-NOT operator (~) on this value to get 11111111111111111111111111110111  ~(ECHO). 
We then bitwise-AND this value with the flags field, which forces the fourth bit in the flags field to become 0, and causes every other bit to retain its current value. Flipping bits like this is common in C.


**Display keypresses**

iscntrl() tests whether a character is a control character.
Control characters are nonprintable characters that we don't want to print to the screen.

ISIG as Ctrl-C can be read as a 3 byte and ctrl-z can be read as a 26 byte.