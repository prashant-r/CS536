
import java.io.*;
import java.util.*;

public class FileClient
{ 

	public static  int  BLOCKSIZE = -1;
	public static final int EXIT_SUCCESS =  0;
	public static final int EXIT_FAILURE = -1;
	public static final int BUFSIZ = 8192;
	//Calculate the round trip time given two timevals
	//to a granularity of milliseconds
	public long RTT(long startTime , long endTime)
	{
		long milliseconds = 0;
		milliseconds = endTime - startTime;
		return milliseconds;
	}
	//Overall validation of command line arguments

	static PrintWriter printer = new PrintWriter(System.out);

	public static void validateCommandLineArguments(String args[]) throws Exception {
		if (args.length != 5) {
			System.out.println(
				"Check number of arguments - Usage hostname portnumber secretkey filename configfile.dat\n");
			System.exit(1);
		}

		validateSecretKey(args[2]);
		validateFileName(args[3]);
		validateConfigFile(args[4]);
	}


	public static void main(String args[]) throws Exception
	{
		validateCommandLineArguments(args);
		// This the startwatch, stopwatch timevals
		long startWatch = 0;
		long stopWatch = 0;

		// Construct message
		String str = "";
		str = String.format("$%s$", argv[3]);
		str += argv[4];
			

		// convert String into InputStream
		InputStream is = new ByteArrayInputStream(str.getBytes());
		String sentence;
	  	String modifiedSentence;
  		BufferedReader inFromUser = new BufferedReader( new InputStreamReader(is));
  		Socket clientSocket = new Socket(server, port);
  		DataOutputStream outToServer = new DataOutputStream(clientSocket.getOutputStream());
  		BufferedReader inFromServer = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
  		clientSocket.shutdownInput();
  		sentence = inFromUser.readLine();
  		outToServer.writeBytes(sentence + '\n');
		// Shut it down so the no more writes need can be done on the socket.
		// Reads can still be done.
		shutdown(sockfd, SHUT_WR);
	
		// File descriptor for the file we are about to download and store.
		int fdw;


		modifiedSentence = inFromServer.readLine();
  		System.out.println("FROM SERVER: " + modifiedSentence);
  		clientSocket.close();


// 		// Read the response
// 		memset(&recvBuffer[0], 0, BLOCKSIZE);	
// 		int numReadBytes = -1;
// 		int totalNumReadBytes = 0;
// 		numReadBytes = read(sockfd, recvBuffer, BLOCKSIZE);
// 		char * filename = argv[4];
// 		if(numReadBytes > 0)
// 		{
// 		// Open the file if not exists. If it already exists we couln't have come so far because of validateFileName called
// 		// in the commandLinArguments validation code.
// 		fdw = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
// 		if(fdw == -1)
// 		{
// 			perror("Error: In creating new file.");
// 			close(sockfd);
// 			exit(EXIT_FAILURE);
// 		}

// 	}
// 	else
// 	{
// 		// No file was found on the server 
// 		// So can't download exit with failure.
// 		printf("Error : No such file on server. \n");
// 		close(sockfd);
// 		exit(EXIT_FAILURE);

// 	}
// 	// If the file was found start writing the bytes read into the file.
// 	write(fdw,recvBuffer,numReadBytes);
// 	memset(&recvBuffer[0], 0, BLOCKSIZE);
// 	totalNumReadBytes += numReadBytes;
// 	if (-1 == gettimeofday(&startWatch, NULL)) {
// 		perror("resettimeofday: gettimeofday");
// 		close(sockfd);
// 		close(fdw);
// 		return EXIT_FAILURE;
// 	}
// 	// If no bytes were read then error reading
// 	if (numReadBytes < 0) {
// 		printf("\n Error reading \n");
// 		close(sockfd);
// 		close(fdw);
// 		return EXIT_FAILURE;
// 	}

// 	while ((numReadBytes = read(sockfd, recvBuffer, BLOCKSIZE)) > 0) {
// 		write(fdw,recvBuffer,numReadBytes);
// 		memset(&recvBuffer[0], 0, BLOCKSIZE);
// 		totalNumReadBytes += numReadBytes;
// 	}

// 	if (numReadBytes < 0) {
// 		printf("\nError : In reading \n");
// 		close(sockfd);
// 		close(fdw);
// 		return EXIT_FAILURE;
// 	}


// 	if (-1 == gettimeofday(&stopWatch, NULL)) {
// 		perror("resettimeofday: gettimeofday");
// 		close(sockfd);
// 		close(fdw);
// 		return EXIT_FAILURE;
// 	}
// // Validate that the checksum was correct. If not exit the program.
// 	if(validateCheckSum(argv[4]) == 1)
// 	{
// 		long long int time_taken = RTT(&startWatch, &stopWatch);
// 		double completion = time_taken/1000.0;
// 		printf("<- <- <- Generated Report -> -> -> \n ");
// 		printf("Number of bytes transfered : %d Bytes\n", totalNumReadBytes);
// 		printf("Completion Time : %lf seconds \n", completion);
// 		printf("Reliable Throughput : %lf MBps \n" , (totalNumReadBytes/(1000000.0*completion)));
// 	}
// 	else
// 	{
// 		close(sockfd);
// 		close(fdw);
// 		return EXIT_FAILURE;

// 	}
// 	close(sockfd);
// 	close(fdw);
// 	return EXIT_SUCCESS;
}

 	// Validate that the filename is of specified length and doesn't contain illegal characters
public static void validateFileName(String filename) throws Exception {
	String invalid_filename_characters = " \n\t//";
	String string_iter = filename;
	int filename_len = filename.length();
	if (filename_len <= 16 && filename_len >= 0) {
		if(string_iter.contains(invalid_filename_characters)){

			throw new Exception("Filename cannot contain spaces or forward slashes ('/'), and it must not exceed 16 ASCII characters.");	
		}
	} else {
		throw new Exception(
			"Filename cannot contain spaces or forward slashes ('/'), and it must not exceed 16 ASCII characters.");
	}


		// So far, the filename is correct.
		// Now, check if file already exists

		// // Make the file path	
	String dir =  "";
	String gcwd = "";
	gcwd = System.getProperty("user.dir");
	String filePath = "";
	filePath += gcwd + "/" + filename;
		// // Next, call access to check if the filepath is legal. If file exists balk. Else, its a new file download
		// // so continue onwards.		
	File f = new File(filePath);
	if(f.exists() && !f.isDirectory()) { 
		System.out.println("-- File already exists! Delete and retry-- \n");
		System.exit(EXIT_FAILURE);
	}
	else
	{
			// File doesn't exist. We good.
		return;
	}

}

 	// Validating the secret key is of legal size and is comprised of ascii characters solely.
public static void validateSecretKey(String secretKey) throws Exception
{
	int secretKey_len = secretKey.length();

	if(secretKey_len <10 || secretKey_len > 20 )
	{
		throw new Exception("Secret Key must be at least length 10 but not more than 20");
	}
	int i;
	for (i=0; i < secretKey_len;i++)
	{
		if (!isascii(secretKey.charAt(i)))
		{
			throw new Exception("Secret Key must be ASCII representable.");
		}
	}
}

public static boolean isascii(char c)
{
	if (c > 0x7F) {
		return false;
	}
	return true;
}

	// // Validating that the configuration file is available for use
	// // And that a number is read from the file to designate the block size
public static void validateConfigFile(String configFilename) throws Exception
{

	File f = new File(configFilename);
	if(f.exists() && !f.isDirectory()) { 
		Scanner reader = new Scanner(f);

		try{
			BLOCKSIZE = reader.nextInt();
		}
		catch(Exception e)
		{
			throw e;
		}
		if(BLOCKSIZE == -1)
		{
			throw new Exception("Illegal blocksize.");
		}
	}
	else
	{
		throw new Exception("Configfile.dat does not exist in filesystem.");
	}
}

	// This is the same code as myunchecksum from lab1. It checks if the checksum attached to the last 8 bytes
	// of the file received from the server matches the computed checksum generated as part of summing the individual bytes of the file
	// If this value matches then we continue forth , else we will have to abort the program.
public static boolean validateCheckSum(String filename)
{
	try {
		File target = new File(filename);
		RandomAccessFile file = new RandomAccessFile(target,"rwd");
		long check_sum = 0;
		int c; 
		StringBuilder response= new StringBuilder();
		while ((c = file.read()) != -1) {
			check_sum += (char) c ;  
		}
			file.seek(target.length()-8); // Set the pointer to the end of the file
			int num_bytes = 7;
			long last8 = 0;
			for(num_bytes = 7; num_bytes >= 0; num_bytes --)
			{
				char read_byte = file.readChar();
				last8 += read_byte;
			}
			check_sum -= last8;
			long small_check_sum = last8;
			long lhs = small_check_sum;
			long rhs = check_sum;
			boolean verdict = false;
			// Perform the match on the expected checksum and given checksum
			if(lhs == rhs)
			{
				// The verdict is 1 meaning it was a match
				verdict = true;
			}
			else
			{
				printer.printf("Checksum Verdict: %s\n","does not match");
				verdict = false;
			}
			return verdict;

		} catch (FileNotFoundException e) {
			e.printStackTrace();
			System.out.println("Error opening file\n");
		} catch (IOException e) {
			e.printStackTrace();
			System.out.println("IOException has occured\n");
		} finally {
			return false;
		}
	}



}
