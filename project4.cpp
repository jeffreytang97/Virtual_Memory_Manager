
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <cstdlib>
#include <sstream>
#include <cstddef> // for null

using namespace std;

int listOfLogicalAddress[1000];
int pageTable[256]; //page table with 256 entries 
int physicalMemory[256*256];

struct TLB { //define an object for the TLB which contains 16 entries
	int pageNumber = 0;
	int frameNumber = 0;
};

int bitMaskingOffset(int logicalAddress) { //function to get the offset
	unsigned n = logicalAddress;
	unsigned offset = n & 0xFF; //lower 8 bits for the offset
	return offset;
}

int bitMaskingPageNum(int logicalAddress) {//function to get the page number
	unsigned x = logicalAddress & 0xFF00;  //masking
	unsigned pageNumber = x >> 8; //bit-shifting by 8
	return pageNumber;
}

int concatenate(int pageNumber, int offset) {

	int physicalAddress = (pageNumber * 256) + offset;

	return physicalAddress; //physical memory is now available!
}

int getNextFreeFrame() {

	for (int index = 0; index < 256; index++)
	{
		if (physicalMemory[index*256] == NULL) {
			return index;
		}
	}
}

int getDataFromBackingStore(int pageNumber) {
	FILE *f;
	char storage[256];

	//open file in read mode
	f = fopen("BACKING_STORE.bin", "r");

	//a size of page is 256 bytes, so if we multiply by page number, we will get to the right page.
	int locationOfPage = 256 * pageNumber;

	//Looking for the location of the page
	fseek(f, locationOfPage, SEEK_SET);

	//Store data in temporary storage in memory
	fread(storage, 1, 256, f);

	int free_frame = getNextFreeFrame();
	for (int i = 0; i < 256; i++)
	{
		physicalMemory[(free_frame*256) + i] = (int) storage[i]; //store the page in a free frame
	}

	fclose(f);

	return free_frame;
}

double pageFaultRate(double pageFault, double number_of_translation) {
	double rate = 0.0;
	rate = pageFault / number_of_translation;
	return rate;
}

double tlbHitRate(double tlb_hit, double number_of_translation) {
	double rate = 0.0;
	rate = tlb_hit / number_of_translation;
	return rate;
}
             
int main() {
	
	string logicalAddress;
	TLB tlb[16]; //the TLB has 16 entries
	int offset = 0;
	int pageNumber = 0;
	int frameNumber = 0;
	int physicalAddress = 0;
	double tlb_hit = 0;
	double pageFault = 0;  
	bool found;
	bool valid = false;
	double number_of_translation = 0;

	//Initialise every value of TLB to 0
	for (int i = 0; i < 16; i++)
	{
		tlb[i].pageNumber = 0;
		tlb[i].frameNumber = 0;
	}

	for (int i = 0; i < 256; i++) //initialise page table
	{
		pageTable[i] = -1; 
	}

	ifstream myfile("addresses.txt"); //Opening file
	if (myfile.is_open())
	{
		int i = 0;
		while (getline(myfile, logicalAddress))
		{ 
			int logical = stoi(logicalAddress);
			listOfLogicalAddress[i] = logical;
			i++;
		}
		myfile.close();
	}
	else {                                                                
		cout << "Unable to open file";
	}                   

	ofstream file("pbs_output.txt");

	//Translation execution
	for (int i = 0; i < 1000; i++) 
	{
		number_of_translation++;
		//Bit masking
		offset = bitMaskingOffset(listOfLogicalAddress[i]);
		pageNumber = bitMaskingPageNum(listOfLogicalAddress[i]);

		//search in TLB for a frame associated to the page number
		for (int i = 0; i < 16; i++)
		{
			if (tlb[i].pageNumber == pageNumber) {
				physicalAddress = concatenate(pageNumber, offset);
				tlb_hit++;
				found = true;
				break;
			}
			else
				found = false;
		}

		//If not found in TLB
		if (pageTable[pageNumber] < 0 && !found) // If not found in the pageTable, go grab the page in backing_store
		{
			pageFault++;
			frameNumber = getDataFromBackingStore(pageNumber);                          
			physicalAddress = concatenate(frameNumber, offset);
			pageTable[pageNumber] = frameNumber;
			
			//Insert the frame number and page number in TLB
			//Shift every value to the right first
			for (int i = 15; i > 0; i--)
			{
				tlb[i].pageNumber = tlb[i - 1].pageNumber;
				tlb[i].frameNumber = tlb[i - 1].frameNumber;
			}

			//Then, insert the frame number + page number in TBL from the pageTable
			tlb[0].pageNumber = pageNumber;
			tlb[1].frameNumber = frameNumber;
		}
		else //If frame exist at location of page number
		{
			//int frameUsed = pageTable[pageNumber];
			physicalAddress = concatenate(pageNumber, offset);

			//Insert the frame number and page number in TLB
			//Shift every value to the right first
			for (int i = 15; i > 0; i--)
			{
				tlb[i].pageNumber = tlb[i - 1].pageNumber;
				tlb[i].frameNumber = tlb[i - 1].frameNumber;
			}

			//Then, insert the frame number + page number in TBL from the pageTable
			tlb[0].pageNumber = pageNumber;
			tlb[1].frameNumber = frameNumber;
		}

		//Write output to a file called pbs_output.txt

		if (file.is_open())
		{
			file << "Virtual address: " << listOfLogicalAddress[i]
				<< " Physical address: " << physicalAddress << " Value: " << physicalMemory[physicalAddress] << endl;
		}
		else cout << "Unable to open file";
	} 

	if (file.is_open())
	{
		file << "Number of Translated Addresses = " << number_of_translation << endl;
		file << "Page faults = " << pageFault << endl;
		file << "Page Fault Rate = " << pageFaultRate(pageFault, number_of_translation) << endl;
		file << "TLB hits = " << tlb_hit << endl;
		file << "TLB Hit Rate = " << tlbHitRate(tlb_hit, number_of_translation) << endl;
	}
	else 
		cout << "Unable to open file";

	myfile.close();

	//Only a test to showcase the results
	cout << "Number of Translated Addresses = " << number_of_translation << endl;
	cout << "Page faults = " << pageFault << endl;
	cout << "Page Fault Rate = " << pageFaultRate(pageFault, number_of_translation) << endl;
	cout << "TLB hits = " << tlb_hit << endl;
	cout << "TLB Hit Rate = " << tlbHitRate(tlb_hit, number_of_translation) << endl;
	
	cin.get();
	return 0;
}