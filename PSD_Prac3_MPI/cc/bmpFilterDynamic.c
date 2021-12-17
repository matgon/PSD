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
	unsigned char *auxPtr;							/** Auxiliary pointer */
	unsigned int rowSize;							/** Number of pixels per row */
	unsigned int rowsPerProcess;					/** Number of rows to be processed (at most) by each worker */
	unsigned int rowsSentToWorker;					/** Number of rows to be sent to a worker process */
	unsigned int receivedRows;						/** Total number of received rows */
	unsigned int threshold;							/** Threshold */
	unsigned int currentRow;						/** Current row being processed */
	unsigned int currentPixel;						/** Current pixel being processed */
	unsigned int outputPixel;						/** Output pixel */
	unsigned int readBytes;							/** Number of bytes read from input file */
	unsigned int writeBytes;						/** Number of bytes written to output file */
	unsigned int totalBytes;						/** Total number of bytes to send/receive a message */
	unsigned int numPixels;							/** Number of neighbour pixels (including current pixel) */
	unsigned int currentWorker;						/** Current worker process */
	unsigned int *processIDs;
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

			if (rank == 0)
				printf ("This program must be launched with (at least) 2 processes\n");

			MPI_Finalize();
			exit(0);
		}

		// Check arguments
		if (argc != 5){

			if (rank == 0)
				printf ("Usage: ./bmpFilterDynamic sourceFile destinationFile threshold numRows\n");

			MPI_Finalize();
			exit(0);
		}

		// Get input arguments...
		sourceFileName = argv[1];
		destinationFileName = argv[2];
		threshold = atoi(argv[3]);
		rowsPerProcess = atoi(argv[4]);

		// Allocate memory for process IDs vector
		processIDs = (unsigned int *) malloc (size*sizeof(unsigned int));

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

			// Show info before processing
			if (SHOW_LOG_MESSAGES){
				printf ("[MASTER] Applying filter to image %s (%dx%d) with threshold %d. Generating image %s\n", sourceFileName,
						rowSize, imgInfoHeaderInput.biHeight, threshold, destinationFileName);
				printf ("Number of workers:%d -> Each worker calculates (at most) %d rows\n", size-1, rowsPerProcess);
			}

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

			//-------------------------------------------------

			//allocate memory for input output data
			inputBuffer = (unsigned char*) malloc (rowSize *imgInfoHeaderInput.biHeight * sizeof(unsigned char));
			outputBuffer = (unsigned char*) malloc (rowSize *imgInfoHeaderInput.biHeight * sizeof(unsigned char));

			//get the data into inputbuffer from the file
			readBytes = read(inputFile, inputBuffer, (rowSize * imgInfoHeaderInput.biHeight * sizeof(unsigned char)));

			if(readBytes > 0){
				auxPtr = inputBuffer; //point to the first element
				currentRow = 0;

				// Distribute rows...
				for (int i = 1; i < size; i++){
					
					MPI_Send (&rowSize, 1, MPI_UNSIGNED, i, tag, MPI_COMM_WORLD);//send the row size					
					MPI_Send (&rowsPerProcess, 1, MPI_UNSIGNED, i, tag, MPI_COMM_WORLD);// Send the number of rows to be processed
					
					//set the row of the index table for the process i
					processIDs[i] = currentRow;

					// Send the data
					MPI_Send (auxPtr, (rowSize * rowsPerProcess * sizeof(unsigned char)), MPI_UNSIGNED_CHAR, i, tag, MPI_COMM_WORLD);

					// Update pointer and index
					currentRow += rowsPerProcess;		
					auxPtr += (rowSize * rowsPerProcess * sizeof(unsigned char));
				}

				//While there are remaining rows...
				unsigned int processedRows = 0;
				unsigned char *auxPtrF;

				while (processedRows < imgInfoHeaderOutput.biHeight){
					MPI_Recv (&receivedRows, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);// Receive number of rows
					// Set destination buffer
					auxPtrF = outputBuffer + (processIDs[status.MPI_SOURCE]*rowSize*sizeof(unsigned char));
					MPI_Recv (auxPtrF, (rowSize * receivedRows * sizeof(unsigned char)), MPI_UNSIGNED_CHAR, status.MPI_SOURCE, tag, MPI_COMM_WORLD, &status);// Receive result data

					// Update processed rows
					processedRows += receivedRows;

					// Send remaining rows...
					if (currentRow < imgInfoHeaderInput.biHeight){
						
						// Send the number of rows to be processed
						if ((currentRow + rowsPerProcess) > imgInfoHeaderInput.biHeight)
							rowsSentToWorker = imgInfoHeaderInput.biHeight - currentRow;
						else
							rowsSentToWorker = rowsPerProcess;

						MPI_Send (&rowsSentToWorker, 1, MPI_UNSIGNED, status.MPI_SOURCE, tag, MPI_COMM_WORLD);

						// Send the rows data and update the index table
						processIDs[status.MPI_SOURCE] = currentRow;
						auxPtr = inputBuffer + rowSize * currentRow;
						MPI_Send (auxPtr, (rowsSentToWorker*rowSize*sizeof(unsigned char)), MPI_UNSIGNED_CHAR, status.MPI_SOURCE, tag, MPI_COMM_WORLD);

						currentRow += rowsSentToWorker;		
					}
					else{
						//send end of processing
						rowsSentToWorker = 0;
						MPI_Send (&rowsSentToWorker, 1, MPI_UNSIGNED, status.MPI_SOURCE, tag, MPI_COMM_WORLD);
					}
				}

				write(outputFile, outputBuffer, rowSize * imgInfoHeaderOutput.biHeight * sizeof(unsigned char));
			}
			//-------------------------------------------------

			// Close files
			close (inputFile);
			close (outputFile);

			// Process ends
			timeEnd = MPI_Wtime();

			// Show processing time
			printf("Filtering time: %f\n",timeEnd-timeStart);
		}else{

			//Worker-----------------------------------------------
			
			MPI_Recv(&rowSize, 1, MPI_UNSIGNED, 0, tag, MPI_COMM_WORLD, &status); //receive the row size

			do{
				MPI_Recv(&receivedRows, 1, MPI_UNSIGNED, 0, tag, MPI_COMM_WORLD, &status); //receive the length

				if(receivedRows != 0){


					inputBuffer = (unsigned char*) malloc(rowSize * receivedRows * sizeof(unsigned char));//malloc of input/output data
					outputBuffer = (unsigned char*) malloc(rowSize * receivedRows * sizeof(unsigned char));
					
					MPI_Recv(inputBuffer, (rowSize * receivedRows * sizeof(unsigned char)), MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD, &status); //receive the data			
					
					//Make calculations
					for(int i = 0; i < receivedRows; i++){
						for(int j = 0; j < rowSize; j++){

							tPixelVector aux;
							unsigned int numPixels = 0;
							if(j == 0 || j == rowSize - 1){
								if(j == 0){//for the first pixels
									aux[0] = inputBuffer[j + (i * rowSize)]; //first pixel
									aux[1] = inputBuffer[j + (i * rowSize) + 1]; //second pixel
								}
								else{//for the last pixels
									aux[0] = inputBuffer[j + (i * rowSize) -1]; //first pixel
									aux[1] = inputBuffer[j + (i * rowSize)]; //second pixel
								}
								numPixels = 2;
							}
							else{//for the intermediate pixels
								aux[0] = inputBuffer[j + (i * rowSize) -1];//first pixel
								aux[1] = inputBuffer[j + (i * rowSize)];//middle pixel
								aux[2] = inputBuffer[j + (i * rowSize) + 1];//last pixel
								numPixels = 3;
							}
							outputBuffer[j + (i * rowSize)] = calculatePixelValue(aux, numPixels, threshold, DEBUG_FILTERING);
						}
					}

					MPI_Send(&receivedRows, 1, MPI_UNSIGNED, 0, tag, MPI_COMM_WORLD);
					MPI_Send(outputBuffer, (rowSize * receivedRows * sizeof(unsigned char)), MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD); //send the data back to the master
					
				}	
			}while(receivedRows > 0);
			//-----------------------------------------------------
		}

		// Finish MPI environment
		MPI_Finalize();
}
