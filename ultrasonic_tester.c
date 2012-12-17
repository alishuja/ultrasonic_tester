#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>

enum PARITY_TYPE {NO_PARITY=0, EVEN_PARITY, ODD_PARITY};

char * DEVICE_FILENAME = NULL;
int BAUD_RATE = 2400;
int FD = 0;

unsigned char SEND_CONFIG[2] = {'\0','\0'};

const char * OPT_STRING="b:c:p:h";

enum PARITY_TYPE PARITY; 

void display_help(){
	fprintf(stdout, "USAGE: ultrasonic_tester [OPTION]... [SERIAL DEVICE FILE NAME]\n");
	fprintf(stdout, "\t-b BAUDRATE\t Set the baudrate.\n");
	fprintf(stdout, "\t-c CONFIG\t Set the configuration in HEX format.\n");
	fprintf(stdout, "\t-p PARITY\t NO PARITY=0,7E1=1, 7O1=2.\n");
	fprintf(stdout, "\t-h\t\t Display this message.\n");
}

int parse_input(int argc, char ** argv) {
	int option_return=-1;
	unsigned int temp_input = 0;
	while((option_return = getopt(argc, argv, OPT_STRING)) !=-1){
		switch(option_return){
			case 'b':
				sscanf(optarg,"%d", &BAUD_RATE);
				fprintf(stdout, "Baud Rate Set: %d.\n", BAUD_RATE);
				break;
			case 'c':
				sscanf(optarg, "%x", &temp_input);
				SEND_CONFIG[1] = (unsigned char)((temp_input & 0xff00) >>8);
				SEND_CONFIG[0] = (unsigned char)((temp_input & 0xff));
				if (SEND_CONFIG[0]>0x7f || SEND_CONFIG[1]>0x7f){
					fprintf(stderr, "Set each config byte between 0x00 and 0x7f.\n");
					return(EXIT_FAILURE);
				}
				break;
			case 'p':
				sscanf(optarg, "%d", &temp_input);
				if(temp_input <0 || temp_input>2){
					fprintf(stderr, "Set Parity between 0 and 2.\n");
					return(EXIT_FAILURE);
				}
				PARITY = temp_input;
				break;
			case 'h':
			default:
				return(EXIT_FAILURE);
		}
	}
	if (argv[optind]==NULL){
		fprintf(stderr, "No device file specified.\n");
		return(EXIT_FAILURE);
	}
	else{
		DEVICE_FILENAME = argv[optind];
		fprintf(stdout, "Device Filename: [%s].\n", DEVICE_FILENAME);
	}
	//Displaying configuration
	fprintf(stdout, "Baud rate: %d.\n", BAUD_RATE);
	fprintf(stdout, "Parity: %d.\n", PARITY);
	fprintf(stdout, "Config: 0x%02x%02x.\n",SEND_CONFIG[1], SEND_CONFIG[0]); 

	return(EXIT_SUCCESS);
}


int main(int argc, char ** argv){
	struct termios config;
	if(parse_input(argc, argv)== EXIT_FAILURE){
		display_help();
		return(EXIT_FAILURE);
	}
	FD = open(DEVICE_FILENAME, O_RDWR| O_NOCTTY| O_NDELAY);
	if (FD ==-1){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return(EXIT_FAILURE);
	}
	if (!isatty(FD)){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return(EXIT_FAILURE);
	}
	
	if(tcgetattr(FD, &config) <0){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return(EXIT_FAILURE);
	}

	config.c_flag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
	close(FD);
	return(EXIT_SUCCESS);
}
