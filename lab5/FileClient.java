import java.io.*;
import java.util.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.charset.*;

public class FileClient
{ 
    public static int BLOCKSIZE = -1;
    public static final int EXIT_SUCCESS =  0;
    public static final int EXIT_FAILURE = -1;
    public static final int BUFSIZ = 8192;

    public Socket clientSock = null;


    public void makeConnection(String hostname, int port, String message, String filename) throws Exception{
        int result = 0;
        try {
            SocketChannel socketChannel = SocketChannel.open();
            socketChannel.connect(new InetSocketAddress(hostname, port));
            clientSock = socketChannel.socket();
            socketChannel.configureBlocking(true);
        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        if(sendMessage(message)) {
            clientSock.shutdownOutput();
            DataInputStream dis = new DataInputStream(clientSock.getInputStream());
           
            byte[] buffer = new byte[BLOCKSIZE];
            int read = 0;
            int totalRead = 0;
            long startwatch = 0;
            long stopwatch = 0;
            try{
                totalRead += dis.read(buffer, 0 , BLOCKSIZE);
            }
            catch(Exception e)
            {
                throw e;
            }
            if(totalRead <= 0)
                throw new Exception("Check secret key or file name. Server not responding.");
            FileOutputStream fos = new FileOutputStream(filename);
            fos.write(buffer, 0, totalRead);
            startwatch = System.nanoTime();
            while(true){
                try{
                    buffer = new byte[BLOCKSIZE];
                    read = dis.read(buffer, 0 , BLOCKSIZE);
                    totalRead += read;
                    fos.write(buffer, 0, read);
                }
                catch(Exception e)
                {
                    break;
                }
            }
            stopwatch = System.nanoTime();
            // Validate that the checksum was correct. If not exit the program.
            if(validateCheckSum(filename))
            {
                long time_taken = RTT(startwatch, stopwatch);
                double completion = time_taken/1000.0;
                System.out.println("<- <- <- Generated Report -> -> -> \n ");
                System.out.println("Number of bytes transfered : " + totalRead + " Bytes\n");
                System.out.println("Completion Time : " + completion + " seconds \n");
                System.out.println("Reliable Throughput : " + String.format("%.3f",(totalRead/(1000000.0*completion))) + " MBps \n" );
            }
            fos.close();
            dis.close();
        }
        else
        {
            throw new Exception("sendto: failed");
        }
        clientSock.close();
    }

    public boolean sendMessage(String message) {
        try{
            DataOutputStream dos = new DataOutputStream(clientSock.getOutputStream());
            InputStream in = new ByteArrayInputStream(message.getBytes(StandardCharsets.UTF_8));
            byte[] buffer = new byte[BLOCKSIZE];
            int count = 0;
            while ((count = in.read(buffer)) > 0)
            {
                dos.write(buffer, 0, count);
            }
            return true;  
        }
        catch(Exception e)
        {
            return false;
        }
    }

	//Calculate the round trip time given two timevals
	//to a granularity of milliseconds
    public long RTT(long startTime , long endTime)
    {
          long milliseconds = 0;
          milliseconds = (endTime - startTime)/1000000;
          return milliseconds;
    }
	//Overall validation of command line arguments
      public static void validateCommandLineArguments(String args[]) throws Exception {
          if (args.length != 5) {
             System.out.println(
                "Check number of arguments - Usage hostname portnumber secretkey filename configfile.dat\n");
             System.exit(1);
         }
         validateHostPort(args[0], args[1]);
         validateSecretKey(args[2]);
         validateFileName(args[3]);
         validateConfigFile(args[4]);
     }

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

    public static void main(String args[]) throws Exception
    {
        validateCommandLineArguments(args);
		// This the startwatch, stopwatch timevals
        long startWatch = 0;
        long stopWatch = 0;

		// Construct message
        String str = "";
        str = String.format("$%s$", args[2]);
        str += (args[3]);

        FileClient cl = new FileClient();
        cl.makeConnection(args[0], Integer.parseInt(args[1]), str, args[3]);
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

private static long getUnsigned(final byte b) {
    if (b < 0) {
        return (long) b + 256;
    }
    return b;
}
// This is the same code as myunchecksum from lab1. It checks if the checksum attached to the last 8 bytes
// of the file received from the server matches the computed checksum generated as part of summing the individual bytes of the file
// If this value matches then we continue forth , else we will have to abort the program.
public static boolean validateCheckSum(String filename)
{
	try {
		File filed = new File(filename);
		RandomAccessFile file = new RandomAccessFile(filed,"r");
		long check_sum = 0;
		int c; 
		StringBuilder response= new StringBuilder();
		FileInputStream fileInputStream=null;

        byte[] bFile = new byte[(int) filed.length()];

        try {
            //convert file into array of bytes
            fileInputStream = new FileInputStream(filed);
            fileInputStream.read(bFile);
            fileInputStream.close();

            for (int i = 0; i < bFile.length; i++) {
                check_sum += (long) bFile[i];
            }

        }catch(Exception e){
            e.printStackTrace();
        }
		file.seek(filed.length()-8); // Set the pointer to the end of the file
		byte[] read_byte = new byte[8];
        file.read(read_byte, 0, 8);
        long last8 = 0;
        long value = 0;
        for(int a= 0; a < 8; a++)
        {
            last8+=read_byte[a];
            value <<= 8;
            value |= getUnsigned(read_byte[a]);
        }
		check_sum -= last8;
        long small_check_sum = value;
		long lhs = small_check_sum;
		long rhs = check_sum;
		boolean verdict = false;
		// Perform the match on the expected checksum and given checksum
		if(lhs == rhs)
		{
			// The verdict is 1 meaning it was a match
            System.out.println("-----------------------------");
            System.out.println("Checksum Verdict: Match :) \n");
            System.out.println("-----------------------------");
			verdict = true;
		}
		else
		{
			System.out.println("Checksum Verdict: does not match \n");
			verdict = false;
		}
		return verdict;

	} catch (FileNotFoundException e) {
		e.printStackTrace();
		System.out.println("Error opening file\n");
	} catch (IOException e) {
    	e.printStackTrace();
		System.out.println("IOException has occured\n");
	} 
        return false;
    }

}
