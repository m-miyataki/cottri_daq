#include <arpa/inet.h>
#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstdio>
#include <fstream>
#include <iostream>

#include <TString.h>
#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>

int debug=0;

TFile *file;
TTree *tree;
unsigned char* buf;

#define WINDOW_SIZE  32
#define NUMBER_OF_CH 48
#define NUMBER_OF_RECBE 10
#define NUMBER_OF_BOARD 1

int HEADER_SIZE;
int MY_BUFSIZE;
int POW_2_16; // pow(2,16)

int num_events=0;
int boardID;       // 1byte: identification number of the COTTRI FE
int triggerMode;   // 1byte: type of the DAQ trigger
int timeWindow;    // 1byte: time length of the taken data
int bufferDelay;   // 1byte: time delay length of the taken data 
int triggerNumber; // 4byte: identification number of the trigger signal from the COTTRI MB
int adc[NUMBER_OF_RECBE][NUMBER_OF_CH][WINDOW_SIZE];

int usage(void)
{
	std::string message = "Usage: ./cottriFEdecoder <datafilename> <rootfilename>";
	std::cerr << message << std::endl;
	return 0;
}

int decode()
{
    unsigned short *id = (unsigned short*)&buf[2]; 
    boardID = ntohl(*id);

    unsigned short *mode = (unsigned short*)&buf[3];
    triggerMode = ntohl(*mode);

    unsigned short *window = (unsigned short*)&buf[4];
    timeWindow = ntohl(*window);

    unsigned short*delay = (unsigned short*)&buf[5];
    bufferDelay = ntohl(*window);

	unsigned int *trig = (unsigned int *)&buf[6]; 
	triggerNumber = ntohl(*trig);

	if (debug) {
		printf("num_events  %d  ", num_events);
		printf("boardID %d ", boardID);
		printf("triggerMode %d  ", triggerMode);
		printf("timeWindow %d  ", timeWindow);
		printf("bufferDelay %d  ",bufferDelay );
		printf("triggerNumber  %d  ", triggerNumber);
		printf("\n");
	}

	unsigned short *adc_data;

	for(int clk=0;clk<WINDOW_SIZE;clk++){
        for(int board=0; board<NUMBER_OF_RECBE; board++ ){
		    for(int ch=0;ch<(int)(NUMBER_OF_CH/4);ch++){
		    	adc_data = (unsigned short *)&buf[ch+NUMBER_OF_CH*board+NUMBER_OF_CH*NUMBER_OF_RECBE*clk+HEADER_SIZE]; 
                adc[board][4*ch+0][clk] = ((0b00000011)&(*adc_data)) >> 0;
                adc[board][4*ch+1][clk] = ((0b00001100)&(*adc_data)) >> 2;
                adc[board][4*ch+2][clk] = ((0b00110000)&(*adc_data)) >> 4;
                adc[board][4*ch+3][clk] = ((0b11000000)&(*adc_data)) >> 6;
		    	adc[board][ch][clk] = ntohs(*adc_data);
            }
        }
    }
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		usage();
		exit(1);
	}

	printf("%s %s\n",argv[0],argv[1]);

	POW_2_16 = pow(2, 16);
	if (getenv("DEBUG")) {
		debug=atoi(getenv("DEBUG"));
	}

	HEADER_SIZE = 16; //byte
	MY_BUFSIZE  = HEADER_SIZE + 0.25*NUMBER_OF_CH*(NUMBER_OF_RECBE)*(WINDOW_SIZE);
	buf = (unsigned char*)malloc(MY_BUFSIZE*sizeof(unsigned char));
	if (buf==NULL) {
		fprintf(stderr,"failed to malloc my_buf\n");
		exit(1);
	}

	file = new TFile(argv[2],"recreate");

	tree = new TTree("tree","cottriFE");
	tree->Branch("boardID",&boardID,"boardID/I");
	tree->Branch("triggerMode",&triggerMode,"triggerMode/I");
	tree->Branch("timeWindow",&timeWindow,"timeWindow/I");
	tree->Branch("bufferDelay",&bufferDelay,"bufferDelay/I");
	tree->Branch("triggerNumber",&triggerNumber,"triggerNumber/I");
	tree->Branch("adc",adc,Form("adc[%d][%d][%d]/I",NUMBER_OF_RECBE,NUMBER_OF_CH,WINDOW_SIZE));

	FILE* fp = fopen(Form("%s",argv[1]), "r");

	if (fp == NULL) {
		err(1,"fopen");
	}

	num_events = 0;
	while (1) {
		int nbyte = fread(buf, 1 , MY_BUFSIZE, fp); // try to read (1*MY_BUFSIZE) bytes
		if (debug>=2) printf("num_events %d nbyte %d\n", num_events, nbyte);
		if (nbyte == 0) {
			if (feof(fp)) {          // End of File
				tree->Write();
				file->Close();
				break;
			} else if (ferror(fp)) { // cannot read data
				tree->Write();
				file->Close();
				printf("\ntotal event = %d\nfinish to make root file\n",num_events-1);
				err(1, "fread");
			} else {                 // etc error
				tree->Write();
				file->Close();
				printf("\ntotal event = %d\nfinish to make root file\n",num_events-1);
				errx(1, "unknown error");
			}
		} else if (nbyte != MY_BUFSIZE) {
			errx(1, "short read: try to read %d but returns %d bytes", 1*MY_BUFSIZE, nbyte);
			break;
		}

		decode();

		tree->Fill();
		num_events++;
	}
	printf("\ntotal event = %d\nfinish to make root file\n",num_events);
	return 0;
}
