
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>



#define KILO_VERSION "1.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

/* =========================
   Data structures
   ========================= */
typedef struct erow /* Editor row to store a line of a text*/
{
    int size;
    char *chars;
} erow;
struct editorConfig {
    int cx, cy;
    int screenrows;
    int screencols;
    int numrows;
    erow row;
    struct termios orig_termios;
};

struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT { NULL, 0 }

/* Global editor state */
struct editorConfig E;

/* =========================
   Function prototypes
   ========================= */

void die(const char *s);
void disableRawMode(void);
void enableRawMode(void);

void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

void editorMoveCursor(int key);
void editorDrawRows(struct abuf *ab);
int editorReadKey(void);
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);

void editorProcessKeypress(void);
void editorRefreshScreen(void);
void initEditor(void);

/* =========================
   Error handling
   ========================= */

void die(const char *s)
{
    /* Clear screen and move cursor to top-left. */
    (void)write(STDOUT_FILENO, "\x1b[2J", 4);
    (void)write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(EXIT_FAILURE);
}

/* =========================
   Raw terminal mode
   ========================= */

void disableRawMode(void)
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH,
                  &E.orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode(void)
{
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
        die("tcgetattr");
    }

    if (atexit(disableRawMode) != 0) {
        die("atexit");
    }

    struct termios raw = E.orig_termios;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}

/* =========================
   Append buffer
   ========================= */

void abAppend(struct abuf *ab, const char *s, int len)
{
    if (len <= 0) {
        return;
    }

    char *new_buf = realloc(ab->b, (size_t)ab->len + (size_t)len);

    if (new_buf == NULL) {
        return;
    }

    memcpy(new_buf + ab->len, s, (size_t)len);

    ab->b = new_buf;
    ab->len += len;
}

void abFree(struct abuf *ab)
{
    free(ab->b);
    ab->b = NULL;
    ab->len = 0;
}

/* =========================
   Cursor movement
   ========================= */

void editorMoveCursor(int key)
{
    switch (key) {
        case ARROW_LEFT:
            if (E.cx > 0) {
                E.cx--;
            }
            break;

        case ARROW_RIGHT:
            if (E.screencols > 0 &&
                E.cx < E.screencols - 1) {
                E.cx++;
            }
            break;

        case ARROW_UP:
            if (E.cy > 0) {
                E.cy--;
            }
            break;

        case ARROW_DOWN:
            if (E.screenrows > 0 &&
                E.cy < E.screenrows - 1) {
                E.cy++;
            }
            break;
    }
}

/* =========================
   Drawing rows
   ========================= */

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    if (y >= E.numrows) {
      if (y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "Kilo editor -- version %s", KILO_VERSION);
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = E.row.size;
      if (len > E.screencols) len = E.screencols;
      abAppend(ab, E.row.chars, len);
    }
    abAppend(ab, "\x1b[K", 3);
    if (y < E.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

/* =========================
   Keyboard input
   ========================= */

int editorReadKey(void)
{
    int nread;
    char c;

    while ((nread = (int)read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN &&
            errno != EINTR) {
            die("read");
        }
    }

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }

        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return '\x1b';
        }

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) {
                    return '\x1b';
                }

                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    }

    return (unsigned char)c;
}

/* =========================
   Terminal size detection
   ========================= */

int getCursorPosition(int *rows, int *cols)
{
    char buf[32];
    size_t i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }

    while (i < sizeof(buf) - 1) {
        ssize_t nread = read(STDIN_FILENO, &buf[i], 1);

        if (nread != 1) {
            break;
        }

        if (buf[i] == 'R') {
            break;
        }

        i++;
    }

    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') {
        return -1;
    }

    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
        return -1;
    }

    return 0;
}

int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 ||
        ws.ws_col == 0) {

        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }

        return getCursorPosition(rows, cols);
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return 0;
}

void editorOpen()
{
    char *line = "Hello Folks";
    ssize_t linelen = 12;

    E.row.size = linelen;
    E.row.chars = malloc(linelen + 1);
    memcpy(E.row.chars, line, linelen);
    E.row.chars[linelen] = '\0';
    E.numrows = 1;
}

/* =========================
   Keypress handling
   ========================= */

void editorProcessKeypress(void)
{
    int c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            (void)write(STDOUT_FILENO, "\x1b[2J", 4);
            (void)write(STDOUT_FILENO, "\x1b[H", 3);
            exit(EXIT_SUCCESS);

        case HOME_KEY:
            E.cx = 0;
            break;

        case END_KEY:
            E.cx = E.screencols > 0 ? E.screencols - 1 : 0;
            break;

        case PAGE_UP:
        case PAGE_DOWN: {
            int times = E.screenrows;

            while (times-- > 0) {
                editorMoveCursor(
                    c == PAGE_UP ? ARROW_UP : ARROW_DOWN
                );
            }
            break;
        }

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

/* =========================
   Screen refresh
   ========================= */

void editorRefreshScreen(void)
{
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    /* Position cursor at the current coordinates. */
    char buf[32];
    int len = snprintf(buf, sizeof(buf),
                       "\x1b[%d;%dH",
                       E.cy + 1, E.cx + 1);

    if (len > 0 && (size_t)len < sizeof(buf)) {
        abAppend(&ab, buf, len);
    }

    abAppend(&ab, "\x1b[?25h", 6);

    if (ab.len > 0) {
        (void)write(STDOUT_FILENO, ab.b, (size_t)ab.len);
    }

    abFree(&ab);
}

/* =========================
   Initialization and main
   ========================= */

void initEditor()
{
    E.cx = 0;
    E.cy = 0;
    E.numrows;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        die("getWindowSize");
    }

    /* Reserve the last row for a status bar in future versions. */
    if (E.screenrows > 0) {
        E.screenrows--;
    }
}


