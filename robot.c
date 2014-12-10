#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

struct expectres {
	char command;
	char* result;
} expected_results[] = {
	{ 'i', "inserted\n" },
	{ 'e', "ejected\n" },
	{ 'p', "parked\n" },
};

void usage(char* a0) {
	printf("usage:\t%s -e|-i|-p|-s [-q] [-T serial]\n", a0);
	printf("or: \t%s -w [-T serial]\n", a0);
	printf("\t-e: eject card\n");
	printf("\t-i: insert card\n");
	printf("\t-p: park arm\n");
	printf("\t-s: stop any movement\n");
	printf("\t-w: request current status (\"where\")\n");
	printf("\t-D: delay (have robot delay before actually doing things.\n");
	printf("\t-Q: quick (do not wait for reply from robot). Always returns success.\n");
	printf("\t-T: select serial device. Defaults to /dev/ttyACM0\n");
	printf("returns: 0 on success (or when -q was specified),\n\t 1 if the result is not what was expected or the request timed out\n");
}

int main(int argc, char**argv) {
	char c;
	bool quick = false;
	bool upper = false;
	char command=0;
	char* serial = "/dev/ttyACM0";

	while((c = getopt(argc, argv, "DehiQpswT:"))>0) {
		switch(c) {
		case 'e':
		case 'i':
		case 'p':
		case 's':
		case 'w':
			command = c;
			break;
		case 'D':
			upper = true;
			break;
		case 'Q':
			quick = true;
			break;
		case 'T':
			serial = strdup(optarg);
			break;
		case 'h':
			usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	if(command == 'w' && quick) {
		fprintf(stderr, "E: -q and -w are mutually exclusive!\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if(!command) {
		fprintf(stderr, "E: need a command!\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if(upper) {
		command=toupper(command);
	}

	int dev = open(serial, O_RDWR | O_NOCTTY);
	struct termios ios;

	tcgetattr(dev, &ios);
	cfmakeraw(&ios);
	ios.c_iflag |= IGNCR;
	ios.c_oflag |= ONLRET;
	cfsetispeed(&ios, B9600);
	cfsetospeed(&ios, B9600);
	ios.c_cflag |= CREAD | CS8 | CRTSCTS;
	ios.c_cflag &= ~PARENB;
	tcsetattr(dev, TCSANOW, &ios);

	write(dev, "r", 1);

	char line[80];
	int len = 0;
	len = read(dev, line, 79);

	line[len] = 0;
	if(strcmp(line, "READY.\n")) {
		fprintf(stderr, "E: robot not found: received %s, expecting 'READY.'\n", line);
		exit(EXIT_FAILURE);
	}
	write(dev, &command, 1);

	if(quick) return 0;

	len = read(dev, line, 80);
	line[len] = 0;
	printf("%s", line);
	for(int i=0; i<sizeof(expected_results) / sizeof(struct expectres); i++) {
		if(expected_results[i].command == command) {
			if(strcmp(expected_results[i].result, line)) {
				fprintf(stderr, "E: unexpected result; expected %s!\n", expected_results[i].result);
				exit(EXIT_FAILURE);
			}
		}
	}
}
