#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define MESSAGE_SIZE 16
#define BLOCK_SIZE 64
#define HASH_SIZE 8

typedef int word;
/*
These are the fractional parts of the first 64 prime numbers in hex.
I was too lazy to calculate them myself, but I may calculate them myself later

0x428a2f98 0x71374491 0xb5c0fbcf 0xe9b5dba5 0x3956c25b 0x59f111f1 0x923f82a4 0xab1c5ed5
0xd807aa98 0x12835b01 0x243185be 0x550c7dc3 0x72be5d74 0x80deb1fe 0x9bdc06a7 0xc19bf174
0xe49b69c1 0xefbe4786 0x0fc19dc6 0x240ca1cc 0x2de92c6f 0x4a7484aa 0x5cb0a9dc 0x76f988da
0x983e5152 0xa831c66d 0xb00327c8 0xbf597fc7 0xc6e00bf3 0xd5a79147 0x06ca6351 0x14292967
0x27b70a85 0x2e1b2138 0x4d2c6dfc 0x53380d13 0x650a7354 0x766a0abb 0x81c2c92e 0x92722c85
0xa2bfe8a1 0xa81a664b 0xc24b8b70 0xc76c51a3 0xd192e819 0xd6990624 0xf40e3585 0x106aa070
0x19a4c116 0x1e376c08 0x2748774c 0x34b0bcb5 0x391c0cb3 0x4ed8aa4a 0x5b9cca4f 0x682e6ff3
0x748f82ee 0x78a5636f 0x84c87814 0x8cc70208 0x90befffa 0xa4506ceb 0xbef9a3f7 0xc67178f2
*/

char* getWordBits(word w);
void printMessage(word* message);

word rotateRight(word input, int amount);
word addMod(word a, word b);
word addModMore(word* toAdd, int size);
word fracCubeRoot(word n);
void fixMessage(word* message, int size, int endPlaceWord, int endPlaceChar);
word flipBytes(word toFlip);

word Ch(word x, word y, word z);
word Maj(word x, word y, word z);
word sigma0(word x);
word sigma1(word x);
word sigmoid0(word x);
word sigmoid1(word x);
/*
These are the fractional parts of the cube root of the first 64 prime numbers in hex.
I was too lazy to calculate them myself, but I may calculate them myself later
*/
word k[] = {0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};

int main(int argc, char** argv){

	FILE* fp;

	int continueReading = 1;
	int paddingNext = 0;
	int fileSize = 0;
	int fileSizeToAdd;
	//Why these values? They're the first fractional part of the square root of the first 8 prime numbers, I think...
	word hash[HASH_SIZE] = {0x6a09e667, 0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
	word tempHash[8];
	word addToHash[HASH_SIZE];
	word messageBuffer[MESSAGE_SIZE];
	//This is when you can't fit the padding into the last 2 words, so you need a whole new message, which is annoying.
	word paddingMessageBuffer[MESSAGE_SIZE];

	for(int x = 0; x < MESSAGE_SIZE; x++) paddingMessageBuffer[x] = 0;

	word block[BLOCK_SIZE];

	word testMessage[MESSAGE_SIZE] = {0x61626364, 0x62636465, 0x63646566, 0x64656667, 0x65666768, 0x66676869, 0x6768696a, 0x68696a6b, 0x696a6b6c, 0x6a6b6c6d, 0x6b6c6d6e, 0x6c6d6e6f, 0x6d6e6f70, 0x6e6f7071, 0x80000000, 0x00000000};

	//printf("%x\n", rotateRight(0x05,3));
	//printf("%x\n", 0xffffffff >> 8);
	//printf("%x\n", rotateRight(hash[0],8));

	if(argc > 1) fp = fopen(argv[1], "r");
	else fp = fopen("test1.txt","r");

	while( continueReading ){

		//First, we read the text in 512 bit blocks, and if we reach the end we pad the text out so that it is 512 bits

		fileSizeToAdd = 0;
		if(!paddingNext){
			for(int x = 0; x < MESSAGE_SIZE; x++){
				int toAdd = fread(messageBuffer + x, sizeof(char), 4, fp);
				messageBuffer[x] = flipBytes(messageBuffer[x]);
				fileSize += toAdd * 8;
				//printf("%x\t%x\n",toAdd,flipBytes(toAdd));

				//It's padding time! Yay!
				if(toAdd != sizeof(word)){

					//fileSize += toAdd *;

					printMessage(messageBuffer);
					printf("%d\n",toAdd);
					word temp;
					if(fileSize % 32 == 0){
						messageBuffer[x] = 0x80000000;
					}
					else{
						temp = (0xffffffff << (32 - fileSize % 32));
						messageBuffer[x] &= temp;
						printf("%d %x\n",fileSize % 32, temp);

						temp = 0x80000000;
						temp = rotateRight(temp, (fileSize % 32) );
						messageBuffer[x] = messageBuffer[x] |temp;
					}
					
					printMessage(messageBuffer);
					for(int rest = x + 1; rest < MESSAGE_SIZE; rest++) messageBuffer[rest] = 0;
					printMessage(messageBuffer);


					if(x < MESSAGE_SIZE - 2){

						int64_t longLongSize = (int64_t) fileSize;

						messageBuffer[MESSAGE_SIZE - 2] = longLongSize << 32;
						messageBuffer[MESSAGE_SIZE - 1] = longLongSize & 0xffffffff;

						printMessage(messageBuffer);

						//printf("%x %x\n",messageBuffer[MESSAGE_SIZE - 2],messageBuffer[MESSAGE_SIZE - 1]);

						continueReading = 0;
					} else {

						printf("Padding needed!\n");
						int64_t longLongSize = (int64_t) fileSize;

						paddingMessageBuffer[MESSAGE_SIZE - 2] = longLongSize << 32;
						paddingMessageBuffer[MESSAGE_SIZE - 1] = longLongSize & 0xffffffff;
					}
					printf("%d\n",fileSize);
					
					break;
				}
			}
			if(fileSizeToAdd == MESSAGE_SIZE) fileSize += fileSizeToAdd;

		}

		//Now, we set the first 16 blocks to the message

		if(paddingNext){
			for(int x = 0; x < MESSAGE_SIZE; x++){
				block[x] = paddingMessageBuffer[x];
			}
			printMessage(paddingMessageBuffer);
			printf("This is the padding step");
			continueReading = 0;
		}
		else{
			for(int x = 0; x < MESSAGE_SIZE; x++){
				block[x] = messageBuffer[x];
			}
			printMessage(messageBuffer);

		}


		//For the rest of the blocks, we set them to specific values

		for(int x = MESSAGE_SIZE; x < BLOCK_SIZE; x++){
			//word stuffToAdd[4] = {sigmoid1(block[x-2]), block[x-7],sigmoid0(block[x-15]),block[x-16]};
			//block[x] = addModMore(stuffToAdd, 4);

			block[x] = sigmoid1(block[x-2]) + block[x - 7] + sigmoid0(block[x-15]) + block[x-16];
		}

		//Now, we need to do BLOCK_SIZE rounds of just switching stuff around.
		//This is the main loop!

		//First, we need to set a temporary starting hash

		for(int x = 0; x < 8; x++){
			tempHash[x] = hash[x];
		}

			//Now, we create new variables that we will add to the hash later
			//We set these variables currently to the current hash

		for(int x = 0; x < HASH_SIZE; x++){
			addToHash[x] = hash[x];
		}

		printf("\n");

		for(int j = 0; j < BLOCK_SIZE; j++){

			//word toAdd1[5] = {addToHash[7], sigma1(addToHash[4]) , Ch(addToHash[4],addToHash[5],addToHash[6]), k[j], block[j]};
			//word t1 = addModMore(toAdd1, 5);

			word t1 = addToHash[7] + sigma1(addToHash[4]) + Ch(addToHash[4],addToHash[5],addToHash[6]) + k[j] + block[j];

			//word t2 = addMod(sigma0(addToHash[0]), Maj(addToHash[0],addToHash[1],addToHash[2]));

			word t2 = sigma0(addToHash[0]) + Maj(addToHash[0],addToHash[1],addToHash[2]);

			addToHash[7] = addToHash[6];
			addToHash[6] = addToHash[5];
			addToHash[5] = addToHash[4];
			addToHash[4] = addToHash[3] + t1;
			addToHash[3] = addToHash[2];
			addToHash[2] = addToHash[1];
			addToHash[1] = addToHash[0];
			addToHash[0] = t1 + t2;

			//This is just for printing the hash as it goes through the main loop

			printf("j = %d\t",j);
			for(int x = 0; x < HASH_SIZE; x++){
				printf("%x\t", addToHash[x]);
			}
			printf("\n");

		}

		for(int x = 0; x < HASH_SIZE; x++){
			hash[x] += addToHash[x];
		}

		if(paddingMessageBuffer[MESSAGE_SIZE - 1] != 0){
			paddingNext = 1;
		}

	}

	//This is for printing the hash

	printf("This is your finished hash: ");

	for(int x = 0; x < 8; x++){
		printf("%8.x ", hash[x]);
	}

	printf("\n");

	//printf("%d\n", fileSize);*/

	fclose(fp);
	return 0;

}

char* getWordBits(word w){
	char* toRet;
	toRet = malloc(sizeof(word) * 8);
	word currentWord = w;
	for(int x = 0; x < (sizeof(word) * 8); x++){
		if(currentWord & 1 == 1) toRet[((sizeof(word)*8)-x-1)] = '1';
		else toRet[((sizeof(word)*8)-x-1)] = '0';
		currentWord = currentWord >> 1;
	}

	return toRet;
}

void printMessage(word* message){
	for(int x = 0; x < MESSAGE_SIZE; x++){
		//printf("%s|",getWordBits(message[x]));
		printf("%08x ",message[x]);
	}
	printf("\n");
}

word rotateRight(word toRotate, int amount){
    word temp;
    word toRet = toRotate;
    for(int b = 0; b < amount; b++){
        temp = toRet & 0x01;
        temp = temp << 31;
        toRet = toRet >> 1;
        toRet = toRet & 0x7fffffff;
        toRet = toRet | temp;
    }

    return toRet;
}

//This fixes the message in case it is smaller than the message size

void fixMessage(word* message, int size, int endPlaceWord, int endPlaceChar){

	//printf("%d\t%d\n",endPlaceWord,endPlaceChar);
	//printf("%s\n",getWordBits(message[endPlaceWord]));

	printMessage(message);

	/*message[endPlaceWord] = message[endPlaceWord] & (0xffffffff >> (sizeof(word) * 8) - (endPlaceChar * 8));

	printf("%x\n",message[endPlaceWord]);

	message[endPlaceWord] = message[endPlaceWord] | (1 << endPlaceChar * 8);

	//printf("%s\n",getWordBits(message[endPlaceWord]));

	for(int x = endPlaceWord + 1; x < MESSAGE_SIZE - 2; x++){
		message[x] = message[x] & 0;
	}*/

	//printf("%d\n",size);

	int64_t longLongSize = (int64_t) size * 8;

	message[MESSAGE_SIZE - 2] = longLongSize << 32;
	message[MESSAGE_SIZE - 1] = longLongSize & 0xffffffff;

	//printMessage(message);

}

word addMod(word a, word b){
	return (word)((int64_t)(a + b) % 0x100000000);
}

word addModMore(word* toAdd, int size){
	if(size == 2) return addMod(toAdd[0],toAdd[1]);
	else return addMod(toAdd[0], addModMore(toAdd + 1, size - 1));
}

word Ch(word x, word y, word z){
	return (x & y) ^ ((~x) & z);
}

word Maj(word x, word y, word z){
	return (x & y) ^ (x & z) ^ (y & z);
}

word sigma0(word x){
	return rotateRight(x, 2) ^ rotateRight(x, 13) ^ rotateRight(x, 22);
}

word sigma1(word x){
	return rotateRight(x, 6) ^ rotateRight(x, 11) ^ rotateRight(x, 25);
}

word sigmoid0(word x){
	//printf("%s %s %s %s\n",getWordBits(x),getWordBits(rotateRight(x, 7)),getWordBits(rotateRight(x,18)),getWordBits(((x >> 3) & 0x1fffffff)));
	return rotateRight(x, 7) ^ rotateRight(x, 18) ^ ((x >> 3) & 0x1fffffff);
}

word sigmoid1(word x){
	//printf("%s %s %s %s\n",getWordBits(x),getWordBits(rotateRight(x, 17)),getWordBits(rotateRight(x,19)),getWordBits((x >> 10) & 0x003fffff));
	return rotateRight(x, 17) ^ rotateRight(x, 19) ^ ((x >> 10) & 0x003fffff);
}

word flipBytes(word toFlip){
	word flipped = 0;
	word remainder = toFlip;
	word temp;

	for(int x = 0; x < 3; x++){
		temp = remainder & 0xff000000;
		flipped = flipped | temp;
		flipped = flipped >> 8;
		remainder = remainder << 8;
	}

	temp = remainder & 0xff000000;
	flipped = flipped | temp;

	return flipped;
}