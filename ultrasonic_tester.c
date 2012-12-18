#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>


char * DEVICE_FILENAME = NULL;
int BAUD_RATE = 2400;
int FD = 0;
unsigned char SINGLE_CONFIG_MODE = 1; //Single config sent or multiple from a file.

unsigned char SEND_CONFIG[2] = {'\0','\0'};

const char * OPT_STRING="b:c:h";

void display_help(){
	fprintf(stdout, "USAGE: ultrasonic_tester [OPTION]... [SERIAL DEVICE FILE NAME]\n");
	fprintf(stdout, "\t-b BAUDRATE\t Set the baudrate.\n");
	fprintf(stdout, "\t-c CONFIG\t Set the configuration in HEX format.\n");
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
	fprintf(stdout, "Config: 0x%02x%02x.\n",SEND_CONFIG[1], SEND_CONFIG[0]); 

	return(EXIT_SUCCESS);
}

int print_sensor_input(){ //will return -1 on failure, 0 on success
	unsigned char buffer[32];
	int total_sensor_readings = 0;
	int sensor_readings_read = 0;
	int count = 0;
	if (read(FD, buffer, 1)==-1){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}
	fprintf(stdout, "Sequence number received: %d\n.", buffer[0]);

	if(read(FD, buffer, 1)==-1){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}
	fprintf(stdout, "Sensor number: %x.\n", ((buffer[0] & 0xf0)>>4));
	total_sensor_readings = (buffer[0] & 0x0f);
	fprintf(stdout, "Total sensor readings: %d\n.", total_sensor_readings);
	sensor_readings_read = read(FD, buffer, total_sensor_readings * 2);
	if(sensor_readings_read==-1){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}
	if(sensor_readings_read < (total_sensor_readings * 2)){
		fprintf(stderr, "Error: %d of %d readings read.\n", sensor_readings_read, total_sensor_readings);
		return -1;
	}
	for (count = 0; count < total_sensor_readings ; count++){
		fprintf(stdout, "TIMER[%d]: %02x%02x\n.", count, buffer[2*count], buffer[2*count+1]);
	}
	return 0;
}

int main(int argc, char ** argv){
	struct termios old_config, new_config;

	unsigned char buffer[2]; 
	int write_success = 0;
	int read_success = 0;

	if(parse_input(argc, argv)== EXIT_FAILURE){
		display_help();
		return(EXIT_FAILURE);
	}
	FD = open(DEVICE_FILENAME, O_RDWR);
	if (FD ==-1){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return(EXIT_FAILURE);
	}
	if (!isatty(FD)){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return(EXIT_FAILURE);
	}

	if(tcgetattr(FD, &old_config) <0){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return(EXIT_FAILURE);
	}

	cfmakeraw(&new_config);
	new_config.c_cc[VMIN] = 1;
	cfsetspeed(&new_config, BAUD_RATE);

	if (SINGLE_CONFIG_MODE == 1){
		//Sending config to micro controller
		buffer[0] = 0x01;
		buffer[1] = SEND_CONFIG[1];
		buffer[2] = SEND_CONFIG[0];
		write_success = write(FD, buffer, 3);
	}
	if (write_success==-1)
	{
		fprintf(stderr, "Error: %s\n",strerror(errno));
	}
	else if (write_success==0){
		fprintf(stderr, "Error: Nothing was written.\n");
	}
	else
		fprintf(stdout, "%d byte(s) were written.\n", write_success);

	//Reading serial port for acknowledgement 
	read_success = read(FD, buffer, 1);
	if (read_success<0){
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}
	else{
		if(buffer[0] != 0x12){
			fprintf(stderr, "No acknowledgement recevied [%02x].\n", buffer[0]);
		}
		else{
			fprintf(stdout, "Acknowledgement received.\n");
			if(SINGLE_CONFIG_MODE == 1){
				if (print_sensor_input()==-1)
					return(EXIT_FAILURE);
			}
		}
	}

	//Before closing port.
	tcsetattr(FD, TCSANOW, &old_config);
	close(FD);
	return(EXIT_SUCCESS);
}
