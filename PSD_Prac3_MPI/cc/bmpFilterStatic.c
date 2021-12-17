#include "bmpBlackWhite.h"
#include "mpi.h"

/** Show log messages */
#define SHOW_LOG_MESSAGES 1

/** Enable output for filtering information */
#define DEBUG_FILTERING 0

/** Show information of input and output bitmap headers */
#define SHOW_BMP_HEADERS 0


int main(int argc, char** argv){

	tBitmapFileHeader imgFileHeaderInput;			/** BMP file header for input image */
	tBitmapInfoHeader imgInfoHeaderInput;			/** BMP info header for input image */
	tBitmapFileHeader imgFileHeaderOutput;			/** BMP file header for output image */
	tBitmapInfoHeader imgInfoHeaderOutput;			/** BMP info header for output image */
	char* sourceFileName;							/** Name of input image file */
	char* destinationFileName;						/** Name of output image file */
	int inputFile, outputFile;						/** File descriptors */
	unsigned char *outputBuffer;					/** Output buffer for filtered pixels */
	unsigned char *inputBuffer;						/** Input buffer to allocate original pixels */
	unsigned char *auxPtr;							/** auxiliary pointer */
	unsigned int rowSize;							/** Number of pixels per row */
	unsigned int rowsPerProcess;					/** Number of rows to be processed (at most) by each worker */
	unsigned int rowsSentToWorker;					/** Number of rows to be sent to a worker process */
	unsigned int threshold;							/** Threshold */
	unsigned int currentRow;						/** Current row being processed */
	unsigned int currentPixel;						/** Current pixel being processed */
	unsigned int outputPixel;						/** Output pixel */
	unsigned int readBytes;							/** Number of bytes read from input file */
	unsigned int writeBytes;						/** Number of bytes written to output file */
	unsigned int totalBytes;						/** Total number of bytes to send/receive a message */
	unsigned int numPixels;							/** Number of neighbour pixels (including current pixel) */
	unsigned int currentWorker;						/** Current worker process */
	tPixelVector vector;							/** Vector of neighbour pixels */
	int imageDimensions[2];							/** Dimensions of input image */
	double timeStart, timeEnd;						/** Time stamps to calculate the filtering time */
	int size, rank, tag;							/** Number of process, rank and tag */
	MPI_Status status;								/** Status information for received messages */

		// Init
		MPI_Init(&argc, &argv);
		MPI_Comm_size(MPI_COMM_WORLD, &size);
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		tag = 1;
		srand(time(NULL));

		// Check the number of processes
		if (size<2){
			printf("%d\n", size);
			if (rank == 0)
				printf ("This program must be launched with (at least) 2 processes\n");

			MPI_Finalize();
			exit(0);
		}

		// Check arguments
		if (argc != 4){

			if (rank == 0)
				printf ("Usage: ./bmpFilterStatic sourceFile destinationFile threshold\n");

			MPI_Finalize();
			exit(0);
		}

		// Get input arguments...
		sourceFileName = argv[1];
		destinationFileName = argv[2];
		threshold = atoi(argv[3]);


		// Master process
		if (rank == 0){

			// Process starts
			timeStart = MPI_Wtime();

			// Read headers from input file
			readHeaders (sourceFileName, &imgFileHeaderInput, &imgInfoHeaderInput);
			readHeaders (sourceFileName, &imgFileHeaderOutput, &imgInfoHeaderOutput);

			// Write header to the output file
			writeHeaders (destinationFileName, &imgFileHeaderOutput, &imgInfoHeaderOutput);

			// Calculate row size for input and output images
			rowSize = (((imgInfoHeaderInput.biBitCount * imgInfoHeaderInput.biWidth) + 31) / 32 ) * 4;
			// Show headers...
			if (SHOW_BMP_HEADERS){
				printf ("Source BMP headers:\n");
				printBitmapHeaders (&imgFileHeaderInput, &imgInfoHeaderInput);
				printf ("Destination BMP headers:\n");
				printBitmapHeaders (&imgFileHeaderOutput, &imgInfoHeaderOutput);
			}

			// Open source image
			if((inputFile = open(sourceFileName, O_RDONLY)) < 0){
				printf("ERROR: Source file cannot be opened: %s\n", sourceFileName);
				exit(1);
			}

			// Open target image
			if((outputFile = open(destinationFileName, O_WRONLY | O_APPEND, 0777)) < 0){
				printf("ERROR: Target file cannot be open to append data: %s\n", destinationFileName);
				exit(1);
			}

			// Allocate memory to copy the bytes between the header and the image data
			outputBuffer = (unsigned char*) malloc ((imgFileHeaderInput.bfOffBits-BIMAP_HEADERS_SIZE) * sizeof(unsigned char));

			// Copy bytes between headers and pixels
			lseek (inputFile, BIMAP_HEADERS_SIZE, SEEK_SET);
			read (inputFile, outputBuffer, imgFileHeaderInput.bfOffBits-BIMAP_HEADERS_SIZE);
			write (outputFile, outputBuffer, imgFileHeaderInput.bfOffBits-BIMAP_HEADERS_SIZE);
			
			//Master-----------------------------------------------

			rowsSentToWorker = imgInfoHeaderInput.biHeight / (size-1); //number of rows sent to each worker

			//allocate memory for input output data
			inputBuffer = (unsigned char*) malloc (rowSize *imgInfoHeaderInput.biHeight * sizeof(unsigned char));
			outputBuffer = (unsigned char*) malloc (rowSize *imgInfoHeaderOutput.biHeight * sizeof(unsigned char));

			//get the data into inputbuffer from the file
			readBytes = read(inputFile, inputBuffer, (rowSize * imgInfoHeaderInput.biHeight * sizeof(unsigned char)));

			if(readBytes > 0){
				
				auxPtr = inputBuffer; //point to the first element
				currentRow = 0;

				for(int i = 1; i < size; i++){

					MPI_Send(&rowSize, 1, MPI_UNSIGNED, i, tag, MPI_COMM_WORLD);// send the row size
					MPI_Send(&currentRow, 1, MPI_UNSIGNED, i, tag, MPI_COMM_WORLD); //send the row where the worker starts filtering
					
					if(currentRow + rowsSentToWorker > imgInfoHeaderInput.biHeight){
						rowsSentToWorker = imgInfoHeaderInput.biHeight - currentRow;
					}

					MPI_Send(&rowsSentToWorker, 1, MPI_UNSIGNED, i, tag, MPI_COMM_WORLD); //send the data length
					MPI_Send(auxPtr, (rowSize * rowsSentToWorker * sizeof(unsigned char)), MPI_UNSIGNED_CHAR, i, tag, MPI_COMM_WORLD); //send the data

					currentRow += rowsSentToWorker;
					auxPtr += (rowsSentToWorker * rowSize * sizeof(unsigned char)); //move the pointer to the next row
				
				}

				for(int i = 1; i < size; i++){ //while there are remaining rows

					MPI_Recv(&rowsPerProcess, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status); //receive the initial row any worker sends
					MPI_Recv(&currentRow, 1, MPI_UNSIGNED, status.MPI_SOURCE, tag, MPI_COMM_WORLD, &status); //receive the initial row any worker sends
					auxPtr = outputBuffer + currentRow * rowSize * sizeof(unsigned char);
					MPI_Recv(auxPtr, (rowSize * rowsPerProcess * sizeof(unsigned char)), MPI_UNSIGNED_CHAR, status.MPI_SOURCE, tag, MPI_COMM_WORLD, &status); //receive the data of the same worker
				
				}

				write(outputFile, outputBuffer, rowSize * imgInfoHeaderOutput.biHeight * sizeof(unsigned char));

			}
			
			//-----------------------------------------------------

			// Close files
			close (outputFile);
			close (inputFile);

			// Process ends
			timeEnd = MPI_Wtime();

			// Show processing time
			printf("Filtering time: %f\n",timeEnd-timeStart);

		}else{
			//Worker-----------------------------------------------

			MPI_Recv(&rowSize, 1, MPI_UNSIGNED, 0, tag, MPI_COMM_WORLD, &status); //receive the row size
			MPI_Recv(&currentRow, 1, MPI_UNSIGNED, 0, tag, MPI_COMM_WORLD, &status); //receive the initial row
			MPI_Recv(&rowsPerProcess, 1, MPI_UNSIGNED, 0, tag, MPI_COMM_WORLD, &status); //receive the length

			//allocate memory for input/output data (only a row)
			inputBuffer = (unsigned char*) malloc(rowSize * rowsPerProcess * sizeof(unsigned char));
			outputBuffer = (unsigned char*) malloc(rowSize * rowsPerProcess * sizeof(unsigned char));
			
			MPI_Recv(inputBuffer, (rowSize * rowsPerProcess * sizeof(unsigned char)), MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD, &status); //receive the data			
			
			//Make calculations
			for(int i = 0; i < rowsPerProcess; i++){
				for(int j = 0; j < rowSize; j++){

					unsigned int numPixels = 0;
					if(j == 0 || j == rowSize - 1){
						if(j == 0){//for the first pixels
							vector[0] = inputBuffer[(i * rowSize)]; //first pixel
							vector[1] = inputBuffer[(i * rowSize) + 1]; //second pixel
						}
						else{//for the last pixels
							vector[0] = inputBuffer[j + (i * rowSize) -1]; //first pixel
							vector[1] = inputBuffer[j + (i * rowSize)]; //second pixel
						}
						numPixels = 2;
					}
					else{//for the intermediate pixels
						vector[0] = inputBuffer[j + (i * rowSize) -1];//first pixel
						vector[1] = inputBuffer[j + (i * rowSize)];//middle pixel
						vector[2] = inputBuffer[j + (i * rowSize) + 1];//last pixel
						numPixels = 3;
					}

					outputBuffer[j + (i * rowSize)] = calculatePixelValue(vector, numPixels, threshold, DEBUG_FILTERING);
				}
			}
			MPI_Send(&rowsPerProcess, 1, MPI_UNSIGNED, 0, tag, MPI_COMM_WORLD);//send the row where the worker started processig
			MPI_Send(&currentRow, 1, MPI_UNSIGNED, 0, tag, MPI_COMM_WORLD);//send the row where the worker started processig
			MPI_Send(outputBuffer, (rowSize * rowsPerProcess * sizeof(unsigned char)), MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD); //send the data back to the master
			//-----------------------------------------------------
		}
		// Finish MPI environment
		MPI_Finalize();
}
