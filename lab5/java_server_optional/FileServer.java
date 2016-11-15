import java.util.*;
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.lang.*;
import java.nio.*;

public class FileServer implements Runnable
{
	public static  int  BLOCKSIZE = -1;
	public static final int EXIT_SUCCESS =  0;
	public static final int EXIT_FAILURE = -1;
	public static final int BUFSIZ = 8192;
	public static final String localhost = "localhost";

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

	// Validate that the filename is of specified length and doesn't contain illegal characters
	public static boolean validateFileName(String filename) throws Exception {
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

		// Make the file path	
		String dir =  "";
		String gcwd = "";
		gcwd = System.getProperty("user.dir");
		String filePath = "";
		filePath += gcwd + "/filedeposit/" + filename;

		// Next, call access to check if the filepath is legal. If file exists balk. Else, its a new file download
		// so continue onwards.		
		File f = new File(filePath);
		if(f.exists() && !f.isDirectory()) { 
			return true;
		}
		else
		{
			System.out.println("File doesn't exist \n");
			return false;
		}
	}

	public byte[] longToBytes(long x) {
    	ByteBuffer buffer = ByteBuffer.allocate(Long.BYTES);
    	buffer.putLong(x);
    	return buffer.array();
	}
	public class WorkerRunnable implements Runnable{

		protected Socket clientSocket = null;
		protected String serverText   = null;
		protected String secretKeyGiven = "";
		protected volatile byte[] buffer; 
		protected volatile int count;

		public WorkerRunnable(Socket clientSocket, String serverText, String secretKeyGiven) {
			this.clientSocket = clientSocket;
			this.serverText   = serverText;
			this.secretKeyGiven = secretKeyGiven;
		}

		public synchronized void run() {

		try {
		BufferedReader inFromClient = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
  		
		// Decode the message
  		String commandRequest = inFromClient.readLine();
  		String delims = "[$]";
		String[] tokens = commandRequest.split(delims);
		String secretKey = tokens[1];
		String filename = tokens[2];

		if(secretKey.trim().equals(secretKeyGiven.trim()))
		{
			if(validateFileName(filename))
			{
				try {

					DataOutputStream dos = new DataOutputStream(clientSocket.getOutputStream());			
					String dir =  "";
					String gcwd = "";
					gcwd = System.getProperty("user.dir");
					String filePath = "";
					filePath += gcwd + "/filedeposit/" + filename;
					filename = filePath;
			    	FileInputStream fis = new FileInputStream(filename);
					buffer = new byte[BLOCKSIZE];
					long check_sum = 0;
					count = 0;
					int totalcount = 0;
            		while ((count = fis.read(buffer, 0, BLOCKSIZE)) > 0)
            		{
            			for(int a = 0; a < count; a++)
            			{
            				check_sum+=(long) buffer[a];
            				dos.write(buffer[a]);
            			}
            		}

            		dos.write(longToBytes(check_sum));
					fis.close();
					dos.close();
				} catch (Exception e) {
            		//report exception somewhere.
					e.printStackTrace();
				}
			}
		}
		else
		{
			System.out.println("Secret Key did not match !");	
		}
		clientSocket.close();
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}

		}
	}

	protected int          serverPort   = 8080;
	protected ServerSocket serverSocket = null;
	protected boolean      isStopped    = false;
	protected Thread       runningThread= null;
	protected String 	   secretKeyGiven = "";

	public FileServer(int port, String secretKeyGiven){
		this.serverPort = port;
		this.secretKeyGiven = secretKeyGiven;
	}

	public void run(){
		synchronized(this){
			this.runningThread = Thread.currentThread();
		}
		openServerSocket();
		while(! isStopped()){
			System.out.println("SERVER ON.");
			Socket clientSocket = null;
			try {
				clientSocket = this.serverSocket.accept();
			} catch (IOException e) {
				if(isStopped()) {
					System.out.println("Server Stopped.") ;
					return;
				}
				throw new RuntimeException(
					"Error accepting client connection", e);
			}
			new Thread(
				new WorkerRunnable(
					clientSocket, "Multithreaded Server", secretKeyGiven)
				).start();
		}
		System.out.println("Server Stopped.") ;
	}


	private synchronized boolean isStopped() {
		return this.isStopped;
	}

	public synchronized void stop(){
		this.isStopped = true;
		try {
			this.serverSocket.close();
		} catch (IOException e) {
			throw new RuntimeException("Error closing server", e);
		}
	}

	private void openServerSocket() {
		try {
			this.serverSocket = new ServerSocket(this.serverPort);
		} catch (IOException e) {
			throw new RuntimeException("Cannot open port 8080", e);
		}
	}

	public static void main(String args[]) throws Exception
	{
		validateCommandLineArguments(args);
		FileServer server = new FileServer(Integer.parseInt(args[0]), args[1]);
		new Thread(server).start();
	}

	// Validates the command line arguments
	public static void validateCommandLineArguments(String args[]) throws Exception
	{
		if(args.length != 3)
		{
			System.out.println("Usage portnumber secretkey configfile.dat\n");
			System.exit(1);
		}
		validateHostPort(localhost, args[0]);
		validateSecretKey(args[1]);
		validateConfigFile(args[2]);
	}

	// Validate the hostname and port number
	public static void validateHostPort(String host, String port) throws Exception
	{
		try{
			Integer.parseInt(port);
		}
		catch( Exception e) {
			throw e;
		}
		if(host.isEmpty()) throw new Exception("Host name must not be empty!");
	}

	// Validates the config file exists and read the number in the file.
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

}


