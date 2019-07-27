/*
 *	Clayton Johnson
 *	cpjohnson@mavs.coloradomesa.edu
 *
 *	This program will take a file (preferably a binary) and the offset and offset size for the text segmentin hex.
 *	The goal is to have this program replace random sections of the text segment with unknown opcodes derived from sandsifter. 
 *
 *	To-Do:
 *	=================================================================================
 *	[x] 	Provide PoC for changing singular bytes in segment
 *	[ ] 	Replace hard-coded values with constants for cleanliness
 *	[ ]	Import opcodes and replace sections of the same size
 *	[ ] 	Test if the program still runs as expected; if so, save file and exit
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

const int MAX_FILENAME_LENGTH = 100;
const int MAX_OFFSET_SIZE = 12;
const int MAX_OPCODE_ARRAY = 50;

int parent_pid;
int my_pid;

char get_escape_char(char bn, char sn); //You can look at this below if you want

int get_write_point(offset_size, offset_loc) {
	//How does one produce a random int in C?
	int rand_num = rand();

	return (rand_num % offset_size) + offset_loc;
}

char* get_opcode(int *index, char *arr[]) {
	//Take supplied char* array and current index
	//	If index will be out of bounds
	//		arr needs to be refilled <- Need to figure out details
	//		opcode = arr[0]
	//		index = 0
	//	Else 	
	//		opcode = arr[index]
	//	    	index++
	//	return opcode 
	
	if ((*index) >= MAX_OPCODE_ARRAY || (*index) < 0) {
		//Read in from the opcode file and set all values in the array
		int i = 0, j = 0;
		FILE *fp;
		char *buff = (char*)malloc(sizeof(char) * 18);
		char *hex_buff = (char*)malloc(sizeof(char) * 49);
		char *byte_buff = (char*)malloc(sizeof(char) * 2);
		fp = fopen("logs_05_21_vm_short", "r");

		//Loop over each line coming in
		while( i < MAX_OPCODE_ARRAY - 1 && fgets(buff, 18, fp) != NULL ) {
			//puts(buff);
			for (j = 0; j < strlen(buff) - 1; j+=2) {
				//Is buff[j] and buff[j+1] 0-9 or a-z? (I'm so sorry)
				if( ((buff[j] >= 48) && (buff[j] <= 57) || (buff[j] >= 97 && buff[j] <= 122)) && ((buff[j+1] >= 48) && (buff[j+1] <= 57) || (buff[j+1] >= 97 && buff[j+1] <= 122))) {
					//byte_buff[0] = '\\';
					//byte_buff[1] = 'x';
					//byte_buff[2] = buff[j];
					//byte_buff[3] = buff[j+1];

					byte_buff[0] = get_escape_char(buff[j], buff[j+1]);
					byte_buff[1] = '\000';
					strcat(hex_buff, byte_buff);
				}
			}
			hex_buff[48] = '\000';
			strncpy(arr[i], hex_buff, 47);
			//printf("%d: %s\n", i, arr[i]);
			strcpy(hex_buff, "");
			i++;
		}
		free(buff);
		free(hex_buff);
		free(byte_buff);
		
		//Need to build all elements into hex-escaped characters
		close(fp);
		(*index) = 0;
	}
	//return "\x90";
	return arr[(*index)];
}

void write_opcode(char *filename_mal, int write_point, char *opcode) {
	
	//Reopen _mal file
	int fd_mal = open(filename_mal, O_RDWR);

	//Prepares file and buff
	lseek(fd_mal, write_point, SEEK_SET);

	//Convert from ascii to hex
	char *buff = (char*) malloc(sizeof(char) * 16);


	//printf("About to write at %d in %s\n", write_point, filename_mal);
		
	//Write the adjusted value back to the same location
	lseek(fd_mal, write_point, SEEK_SET);
	write(fd_mal, opcode, strlen(opcode));	
	
	close(fd_mal);
}

int test_file(char *filename_mal) {
	int success = 0;

	//Run new file
	int ret = fork();
	if(ret == 0) {
		//puts("Child starting...");
		//printf("Child: pid=%d\n", getpid());
		execl(filename_mal, "", NULL);
		puts("Other side of the child");
	} else {
		//puts("Parent waiting...");
		int status;
		pid_t result = waitpid(-1, &status, 0);
		if(WIFEXITED(status)) {
			success = 1;
			//puts("true");
		} else {
			//puts("false");
		}
	}
	return success;
}

char* make_test_file(char *filename, char *filename_mal) {

	//Open file + create file_mal
	int fd = open(filename, O_RDONLY);

	if(fd == -1) {
		printf("File could not be opened: %s", filename);
		exit(1);
	}

	//Create malicious filename
	filename_mal = (char*) malloc(sizeof(char)*MAX_FILENAME_LENGTH + 4);
	strcpy(filename_mal, filename);
	filename_mal[MAX_FILENAME_LENGTH] = '\000';
	strcat(filename_mal, "_mal");

	//Create malicious file
	int fd_mal;
	fd_mal = open(filename_mal, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU|S_IRWXG|S_IROTH);
	if(fd_mal == -1) {
		printf("File could not be opened/created: %s\n", filename_mal);
		exit(1);
	}


	//Copy file to file_mal
	char* buff = (char*) malloc(sizeof(char)*2);
	buff[1] = '\000';
	int bytes_read = 1, bytes_written = 0;
	int read_offset = 0;
	while(bytes_read == 1 && bytes_written != -1) {
		//Go to next byte and read it
		lseek(fd, read_offset, SEEK_SET);
		bytes_read = read(fd, buff, 1);

		//Go to next byte and if bytes were read, write them
		lseek(fd_mal, read_offset, SEEK_SET);
		if(bytes_read == 1) {
			bytes_written = write(fd_mal, buff, 1);
		}

		read_offset++;
	}
	close(fd);
	close(fd_mal);
	return filename_mal;
}


int main(int argc, char *argv[]) {
	
	char *filename, *filename_mal, *offset, *offset_size;
	filename = (char*) malloc(sizeof(char)*MAX_FILENAME_LENGTH);
	filename = (char*) malloc(sizeof(char)*MAX_FILENAME_LENGTH + 4);
	offset = (char*) malloc(sizeof(char)*MAX_OFFSET_SIZE);
	offset_size = (char*) malloc(sizeof(char)*MAX_OFFSET_SIZE);
	parent_pid = (int) getpid();

	//Testing for inputs, need to test and build fail cases
	if(argc < 4) {
		//Read from stdin
		scanf("%s", filename);
		scanf("%s", offset);
		scanf("%s", offset_size);
	} else {
		//Arguments provided
		strncpy(filename, argv[1], MAX_FILENAME_LENGTH);
		strncpy(offset,argv[2], MAX_OFFSET_SIZE);
		strncpy(offset_size,argv[3], MAX_OFFSET_SIZE);
	}

	//Make sure to terminate char buffer
	filename[MAX_FILENAME_LENGTH - 1] = '\000';
	offset[MAX_OFFSET_SIZE - 1] = '\000';
	offset_size[MAX_OFFSET_SIZE - 1] = '\000';		

	//Start loop
	int success = 0;
	char *opcode = "";
	char *op_arr[MAX_OPCODE_ARRAY];
	for(int i = 0; i < MAX_OPCODE_ARRAY; i++) {
		op_arr[i] = (char*) malloc(sizeof(char)*49);
	}
	int op_index = -1;
	int w_point = 0; //Write point for the opcode in the text segment
	int offset_loc = (int) strtol(offset, NULL, 0);
	int offset_size_int = (int) strtol(offset_size, NULL, 0);
	srand(time(NULL));
	int loops = 0;

	while(!success) {
		//Copy over clean file
		filename_mal = make_test_file(filename, filename_mal);

		//Get random values for the following
		opcode = get_opcode(&op_index, op_arr);
		op_index++;
		w_point = get_write_point(offset_size_int, offset_loc);

		//Check bounds
		if( (w_point + strlen(opcode)) <= (offset_loc + offset_size_int) && w_point > 0) {
			//Write to file
			write_opcode(filename_mal, w_point, opcode);

			//Test file
			if (test_file(filename_mal)) {
				success = 1;
				//printf("%d : Success!\n", loops);
				printf("%d\n", w_point);
			} else {
				//printf("%d : Failure! %s at offset+%d\n", loops, opcode, w_point);
			}
		}
		//if(loops == 5) success = 1;
		loops++;
	}
	/*//PoC : Set first byte to 0x90 
		
		//Convert ascii hexadecimal offset_start to int decimal
		int offset_converted = (int) strtol(offset, NULL, 0);

		//Convert ascii hexadecimal offset_size to int decimal
		int offset_size_converted = (int) strtol(offset_size, NULL, 0);
		
		//Reopen _mal file
		fd_mal = open(filename_mal, O_RDWR);

		//Prepares file and buff
		lseek(fd_mal, offset_converted, SEEK_SET);
		strcpy(buff, "\x90");
			
		//Write the adjusted value back to the same location
		lseek(fd_mal, offset_converted, SEEK_SET);
		write(fd_mal, buff, 1);	
		
	close(fd_mal);*/
	

	/*//Run new file
	int ret = fork();
	if(ret == 0) {
		puts("Child starting...");
		execl(filename_mal, "", NULL);
		puts("Child done.");
	} else {
		puts("Parent waiting...");
		int status;
		pid_t result = waitpid(ret, &status);
		while((result=waitpid(ret, &status, 0)) == 1) {}
		if(WIFEXITED(status)) {
			puts("true");
		} else {
			puts("false");
		}

	}*/


	//Free up dynamic memory
	//puts("Freeing all op_arr elements");
	for(int i = 0; i < MAX_OPCODE_ARRAY; i++) {
		
		free(op_arr[i]); 
	}
	//puts("Freeing filename");
	free(filename);
	//puts("Freeing offset");
	free(offset);
	//puts("Freeing offset_size");
	free(offset_size);
	return 0;
}

char get_escape_char(char bn, char sn) {
	//I'm doing this as a last resort, please send help
	char ret;

	switch(bn){
		case '0':
			switch(sn){
				case '0':
					ret = '\x00';
					break;
				case '1':
					ret = '\x01';
					break;
				case '2':
					ret = '\x02';
					break;
				case '3':
					ret = '\x03';
					break;
				case '4':
					ret = '\x04';
					break;
				case '5':
					ret = '\x05';
					break;
				case '6':
					ret = '\x06';
					break;
				case '7':
					ret = '\x07';
					break;
				case '8':
					ret = '\x08';
					break;
				case '9':
					ret = '\x09';
					break;
				case 'a':
					ret = '\x0a';
					break;
				case 'b':
					ret = '\x0b';
					break;
				case 'c':
					ret = '\x0c';
					break;
				case 'd':
					ret = '\x0d';
					break;
				case 'e':
					ret = '\x0e';
					break;
				case 'f':
					ret = '\x0f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case '1':
			switch(sn){
				case '0':
					ret = '\x10';
					break;
				case '1':
					ret = '\x11';
					break;
				case '2':
					ret = '\x12';
					break;
				case '3':
					ret = '\x13';
					break;
				case '4':
					ret = '\x14';
					break;
				case '5':
					ret = '\x15';
					break;
				case '6':
					ret = '\x16';
					break;
				case '7':
					ret = '\x17';
					break;
				case '8':
					ret = '\x18';
					break;
				case '9':
					ret = '\x19';
					break;
				case 'a':
					ret = '\x1a';
					break;
				case 'b':
					ret = '\x1b';
					break;
				case 'c':
					ret = '\x1c';
					break;
				case 'd':
					ret = '\x1d';
					break;
				case 'e':
					ret = '\x1e';
					break;
				case 'f':
					ret = '\x1f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case '2':
			switch(sn){
				case '0':
					ret = '\x20';
					break;
				case '1':
					ret = '\x21';
					break;
				case '2':
					ret = '\x22';
					break;
				case '3':
					ret = '\x23';
					break;
				case '4':
					ret = '\x24';
					break;
				case '5':
					ret = '\x25';
					break;
				case '6':
					ret = '\x26';
					break;
				case '7':
					ret = '\x27';
					break;
				case '8':
					ret = '\x28';
					break;
				case '9':
					ret = '\x29';
					break;
				case 'a':
					ret = '\x2a';
					break;
				case 'b':
					ret = '\x2b';
					break;
				case 'c':
					ret = '\x2c';
					break;
				case 'd':
					ret = '\x2d';
					break;
				case 'e':
					ret = '\x2e';
					break;
				case 'f':
					ret = '\x2f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case '3':
			switch(sn){
				case '0':
					ret = '\x30';
					break;
				case '1':
					ret = '\x31';
					break;
				case '2':
					ret = '\x32';
					break;
				case '3':
					ret = '\x33';
					break;
				case '4':
					ret = '\x34';
					break;
				case '5':
					ret = '\x35';
					break;
				case '6':
					ret = '\x36';
					break;
				case '7':
					ret = '\x37';
					break;
				case '8':
					ret = '\x38';
					break;
				case '9':
					ret = '\x39';
					break;
				case 'a':
					ret = '\x3a';
					break;
				case 'b':
					ret = '\x3b';
					break;
				case 'c':
					ret = '\x3c';
					break;
				case 'd':
					ret = '\x3d';
					break;
				case 'e':
					ret = '\x3e';
					break;
				case 'f':
					ret = '\x3f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case '4':
			switch(sn){
				case '0':
					ret = '\x40';
					break;
				case '1':
					ret = '\x41';
					break;
				case '2':
					ret = '\x42';
					break;
				case '3':
					ret = '\x43';
					break;
				case '4':
					ret = '\x44';
					break;
				case '5':
					ret = '\x45';
					break;
				case '6':
					ret = '\x46';
					break;
				case '7':
					ret = '\x47';
					break;
				case '8':
					ret = '\x48';
					break;
				case '9':
					ret = '\x49';
					break;
				case 'a':
					ret = '\x4a';
					break;
				case 'b':
					ret = '\x4b';
					break;
				case 'c':
					ret = '\x4c';
					break;
				case 'd':
					ret = '\x4d';
					break;
				case 'e':
					ret = '\x4e';
					break;
				case 'f':
					ret = '\x4f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case '5':
			switch(sn){
				case '0':
					ret = '\x50';
					break;
				case '1':
					ret = '\x51';
					break;
				case '2':
					ret = '\x52';
					break;
				case '3':
					ret = '\x53';
					break;
				case '4':
					ret = '\x54';
					break;
				case '5':
					ret = '\x55';
					break;
				case '6':
					ret = '\x56';
					break;
				case '7':
					ret = '\x57';
					break;
				case '8':
					ret = '\x58';
					break;
				case '9':
					ret = '\x59';
					break;
				case 'a':
					ret = '\x5a';
					break;
				case 'b':
					ret = '\x5b';
					break;
				case 'c':
					ret = '\x5c';
					break;
				case 'd':
					ret = '\x5d';
					break;
				case 'e':
					ret = '\x5e';
					break;
				case 'f':
					ret = '\x5f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case '6':
			switch(sn){
				case '0':
					ret = '\x60';
					break;
				case '1':
					ret = '\x61';
					break;
				case '2':
					ret = '\x62';
					break;
				case '3':
					ret = '\x63';
					break;
				case '4':
					ret = '\x64';
					break;
				case '5':
					ret = '\x65';
					break;
				case '6':
					ret = '\x66';
					break;
				case '7':
					ret = '\x67';
					break;
				case '8':
					ret = '\x68';
					break;
				case '9':
					ret = '\x69';
					break;
				case 'a':
					ret = '\x6a';
					break;
				case 'b':
					ret = '\x6b';
					break;
				case 'c':
					ret = '\x6c';
					break;
				case 'd':
					ret = '\x6d';
					break;
				case 'e':
					ret = '\x6e';
					break;
				case 'f':
					ret = '\x6f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case '7':
			switch(sn){
				case '0':
					ret = '\x70';
					break;
				case '1':
					ret = '\x71';
					break;
				case '2':
					ret = '\x72';
					break;
				case '3':
					ret = '\x73';
					break;
				case '4':
					ret = '\x74';
					break;
				case '5':
					ret = '\x75';
					break;
				case '6':
					ret = '\x76';
					break;
				case '7':
					ret = '\x77';
					break;
				case '8':
					ret = '\x78';
					break;
				case '9':
					ret = '\x79';
					break;
				case 'a':
					ret = '\x7a';
					break;
				case 'b':
					ret = '\x7b';
					break;
				case 'c':
					ret = '\x7c';
					break;
				case 'd':
					ret = '\x7d';
					break;
				case 'e':
					ret = '\x7e';
					break;
				case 'f':
					ret = '\x7f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case '8':
			switch(sn){
				case '0':
					ret = '\x80';
					break;
				case '1':
					ret = '\x81';
					break;
				case '2':
					ret = '\x82';
					break;
				case '3':
					ret = '\x83';
					break;
				case '4':
					ret = '\x84';
					break;
				case '5':
					ret = '\x85';
					break;
				case '6':
					ret = '\x86';
					break;
				case '7':
					ret = '\x87';
					break;
				case '8':
					ret = '\x88';
					break;
				case '9':
					ret = '\x89';
					break;
				case 'a':
					ret = '\x8a';
					break;
				case 'b':
					ret = '\x8b';
					break;
				case 'c':
					ret = '\x8c';
					break;
				case 'd':
					ret = '\x8d';
					break;
				case 'e':
					ret = '\x8e';
					break;
				case 'f':
					ret = '\x8f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case '9':
			switch(sn){
				case '0':
					ret = '\x90';
					break;
				case '1':
					ret = '\x91';
					break;
				case '2':
					ret = '\x92';
					break;
				case '3':
					ret = '\x93';
					break;
				case '4':
					ret = '\x94';
					break;
				case '5':
					ret = '\x95';
					break;
				case '6':
					ret = '\x96';
					break;
				case '7':
					ret = '\x97';
					break;
				case '8':
					ret = '\x98';
					break;
				case '9':
					ret = '\x99';
					break;
				case 'a':
					ret = '\x9a';
					break;
				case 'b':
					ret = '\x9b';
					break;
				case 'c':
					ret = '\x9c';
					break;
				case 'd':
					ret = '\x9d';
					break;
				case 'e':
					ret = '\x9e';
					break;
				case 'f':
					ret = '\x9f';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case 'a':
			switch(sn){
				case '0':
					ret = '\xa0';
					break;
				case '1':
					ret = '\xa1';
					break;
				case '2':
					ret = '\xa2';
					break;
				case '3':
					ret = '\xa3';
					break;
				case '4':
					ret = '\xa4';
					break;
				case '5':
					ret = '\xa5';
					break;
				case '6':
					ret = '\xa6';
					break;
				case '7':
					ret = '\xa7';
					break;
				case '8':
					ret = '\xa8';
					break;
				case '9':
					ret = '\xaa';
					break;
				case 'a':
					ret = '\xaa';
					break;
				case 'b':
					ret = '\xab';
					break;
				case 'c':
					ret = '\xac';
					break;
				case 'd':
					ret = '\xad';
					break;
				case 'e':
					ret = '\xae';
					break;
				case 'f':
					ret = '\xaf';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case 'b':
			switch(sn){
				case '0':
					ret = '\xb0';
					break;
				case '1':
					ret = '\xb1';
					break;
				case '2':
					ret = '\xb2';
					break;
				case '3':
					ret = '\xb3';
					break;
				case '4':
					ret = '\xb4';
					break;
				case '5':
					ret = '\xb5';
					break;
				case '6':
					ret = '\xb6';
					break;
				case '7':
					ret = '\xb7';
					break;
				case '8':
					ret = '\xb8';
					break;
				case '9':
					ret = '\xb9';
					break;
				case 'a':
					ret = '\xba';
					break;
				case 'b':
					ret = '\xbb';
					break;
				case 'c':
					ret = '\xbc';
					break;
				case 'd':
					ret = '\xbd';
					break;
				case 'e':
					ret = '\xbe';
					break;
				case 'f':
					ret = '\xbf';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case 'c':
			switch(sn){
				case '0':
					ret = '\xc0';
					break;
				case '1':
					ret = '\xc1';
					break;
				case '2':
					ret = '\xc2';
					break;
				case '3':
					ret = '\xc3';
					break;
				case '4':
					ret = '\xc4';
					break;
				case '5':
					ret = '\xc5';
					break;
				case '6':
					ret = '\xc6';
					break;
				case '7':
					ret = '\xc7';
					break;
				case '8':
					ret = '\xc8';
					break;
				case '9':
					ret = '\xc9';
					break;
				case 'a':
					ret = '\xca';
					break;
				case 'b':
					ret = '\xcb';
					break;
				case 'c':
					ret = '\xcc';
					break;
				case 'd':
					ret = '\xcd';
					break;
				case 'e':
					ret = '\xce';
					break;
				case 'f':
					ret = '\xcf';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case 'd':
			switch(sn){
				case '0':
					ret = '\xd0';
					break;
				case '1':
					ret = '\xd1';
					break;
				case '2':
					ret = '\xd2';
					break;
				case '3':
					ret = '\xd3';
					break;
				case '4':
					ret = '\xd4';
					break;
				case '5':
					ret = '\xd5';
					break;
				case '6':
					ret = '\xd6';
					break;
				case '7':
					ret = '\xd7';
					break;
				case '8':
					ret = '\xd8';
					break;
				case '9':
					ret = '\xd9';
					break;
				case 'a':
					ret = '\xda';
					break;
				case 'b':
					ret = '\xdb';
					break;
				case 'c':
					ret = '\xdc';
					break;
				case 'd':
					ret = '\xdd';
					break;
				case 'e':
					ret = '\xde';
					break;
				case 'f':
					ret = '\xdf';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case 'e':
			switch(sn){
				case '0':
					ret = '\xe0';
					break;
				case '1':
					ret = '\xe1';
					break;
				case '2':
					ret = '\xe2';
					break;
				case '3':
					ret = '\xe3';
					break;
				case '4':
					ret = '\xe4';
					break;
				case '5':
					ret = '\xe5';
					break;
				case '6':
					ret = '\xe6';
					break;
				case '7':
					ret = '\xe7';
					break;
				case '8':
					ret = '\xe8';
					break;
				case '9':
					ret = '\xe9';
					break;
				case 'a':
					ret = '\xea';
					break;
				case 'b':
					ret = '\xeb';
					break;
				case 'c':
					ret = '\xec';
					break;
				case 'd':
					ret = '\xed';
					break;
				case 'e':
					ret = '\xee';
					break;
				case 'f':
					ret = '\xef';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		case 'f':
			switch(sn){
				case '0':
					ret = '\xf0';
					break;
				case '1':
					ret = '\xf1';
					break;
				case '2':
					ret = '\xf2';
					break;
				case '3':
					ret = '\xf3';
					break;
				case '4':
					ret = '\xf4';
					break;
				case '5':
					ret = '\xf5';
					break;
				case '6':
					ret = '\xf6';
					break;
				case '7':
					ret = '\xf7';
					break;
				case '8':
					ret = '\xf8';
					break;
				case '9':
					ret = '\xf9';
					break;
				case 'a':
					ret = '\xfa';
					break;
				case 'b':
					ret = '\xfb';
					break;
				case 'c':
					ret = '\xfc';
					break;
				case 'd':
					ret = '\xfd';
					break;
				case 'e':
					ret = '\xfe';
					break;
				case 'f':
					ret = '\xff';
					break;
				default:
					puts("Unrecognized hexadecimal character!");
			}
				break;
		default:
			puts("Unrecognized hexadecimal character!");
	}


	return ret;
}
