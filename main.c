#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define ENVVAR	"PWDPROMPT"

#define STDIN	0
#define STDOUT	1
#define STDERR	2

int main(int argc, char **argv) {
	struct termios t;
	tcgetattr(STDIN, &t);
	t.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON);
	tcsetattr(STDIN, TCSAFLUSH, &t);

	char *default_prompt = "pwd > ";
	char *prompt = getenv(ENVVAR);
	if (prompt == NULL) prompt = default_prompt;

	write(STDERR, prompt, strlen(prompt));

	size_t buflen = 128;
	char buf[buflen*2];

	// set second half to obscured
	memset(buf+buflen, '*', buflen);
	int hide = 1;

	char back[buflen];
	memset(back, '\b', buflen);

	char tmp[2];

	size_t nchars = 0;
	size_t cursorpos = 0;

	while (1) {
		read(STDIN, tmp, 1);
		switch (*tmp) {
			case '\033':
				struct pollfd pfd = { STDIN, POLLIN, POLLIN };
				if (poll(&pfd, 1, 0) == 0) {
					hide = !hide;
					write(STDERR, back, cursorpos);
					write(STDERR, buf+(buflen*hide), nchars);
					write(STDERR, back, nchars-cursorpos);
					break;
				} else {
					read(STDIN, &tmp, 2);
					if (tmp[1] == 'C') {
						// right
						if (cursorpos < nchars) {
							write(STDERR, buf+cursorpos, 1);
							cursorpos += 1;
						}
					} else if (tmp[1] == 'D') {
						// left
						cursorpos -= 1 ? cursorpos > 0 : 0;
						write(STDERR, "\b", 1);
					}
				}
				break;
			case 127:
				// backspace
				if (cursorpos > 0) {
					for (size_t i = cursorpos; i <= nchars; i++) {
						buf[i-1] = buf[i];
					}
					cursorpos--;
					nchars--;
					write(STDERR, "\b", 1);
					write(STDERR, buf+(buflen*hide)+cursorpos, nchars-cursorpos);
					write(STDERR, " ", 1);
					write(STDERR, back, nchars-cursorpos+1);
				}
				break;
			case '\n':
				buf[nchars] = '\0';
				goto finish;
			default:
				if (nchars < buflen) {
					if (cursorpos < nchars) {
						for (size_t i = nchars; i >= cursorpos; i--) {
							buf[i+1] = buf[i];
						}
					}
					buf[cursorpos] = *tmp;
					write(STDERR, buf+(buflen*hide)+cursorpos, nchars-cursorpos+1);
					write(STDERR, back, nchars-cursorpos);
					nchars++;
					cursorpos++;
				}
				break;
		}
	}

finish:
	write(STDERR, "\n", 1);
	write(STDOUT, buf, nchars);
	return EXIT_SUCCESS;
}
