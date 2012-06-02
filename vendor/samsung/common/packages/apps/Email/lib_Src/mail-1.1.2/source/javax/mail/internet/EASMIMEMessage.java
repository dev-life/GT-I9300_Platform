/* 
 * MimeMessage.java 
 * Copyright (C) 2002, 2004, 2005 The Free Software Foundation  
 *   
 * This file is part of GNU JavaMail, a library.  
 *  
 * GNU JavaMail is free software; you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.  
 *   
 * GNU JavaMail is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.  
 *  
 * You should have received a copy of the GNU General Public License 
 * along with this library; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA  
 * 
 * As a special exception, if you link this library with other files to 
 * produce an executable, this library does not by itself cause the  
 * resulting executable to be covered by the GNU General Public License.  
 * This exception does not however invalidate any other reasons why the 
 * executable file might be covered by the GNU General Public License.  
 */
 
package javax.mail.internet;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Enumeration;
import java.util.StringTokenizer;

import javax.mail.Flags;
import javax.mail.Folder;
import javax.mail.MessagingException;
import javax.mail.Session;
import javax.mail.internet.InternetHeaders;
import javax.mail.internet.MimeMessage;
import javax.mail.internet.MimeUtility;
import javax.mail.internet.SharedInputStream;

/**
 *  MimeMessage keeps the Inputstream as contentStream only if it is SharedInputStream otherwise reads all the content and put in Buffer 
 * MyMIMEMessage is alterative to MimeMessage. It keeps the InputStream and
 * only reads from it at the time of writing (no matter wheather it is shared Inputstream or not)
 * Note: We can use this only once to write to an Outputstream. It throws IllegalStateException on second attempt.*/
public class EASMIMEMessage extends MimeMessage{
    InputStream mMessageInuputStream;
	private int bufferSize;
	boolean isWriten = false;
	public EASMIMEMessage(Folder arg0, InputStream arg1, int arg2)
			throws MessagingException {
		super(arg0, arg1, arg2);
		// TODO Auto-generated constructor stub
	}

	public EASMIMEMessage(Folder arg0, int arg1) {
		super(arg0, arg1);
		// TODO Auto-generated constructor stub
	}

	public EASMIMEMessage(Folder arg0, InternetHeaders arg1, byte[] arg2,
			int arg3) throws MessagingException {
		super(arg0, arg1, arg2, arg3);
		// TODO Auto-generated constructor stub
	}

	public EASMIMEMessage(MimeMessage arg0) throws MessagingException {
		super(arg0);
		
		// TODO Auto-generated constructor stub
	}

	public EASMIMEMessage(Session session, InputStream io, int bufferSize)
			throws MessagingException {
		super(session);
		flags = new Flags();
	    mMessageInuputStream = io;
	    headers = createInternetHeaders(mMessageInuputStream);
		saved = true;
		modified = false;
		this.bufferSize = bufferSize;
	}
	public EASMIMEMessage(Session arg0) {
		super(arg0);
		// TODO Auto-generated constructor stub
	}

	/*@Override
	public InputStream getInputStream() throws IOException, MessagingException {
		// TODO Auto-generated method stub
		return mMessageInuputStream;
	}
*/
	@Override
	public Enumeration getNonMatchingHeaders(String[] arg0)
			throws MessagingException {
		// TODO Auto-generated method stub
		return super.getNonMatchingHeaders(arg0);
	}

	@Override
	protected InputStream getContentStream() throws MessagingException {
		// TODO Auto-generated method stub
		return mMessageInuputStream;
	}

	/**
	   * Writes this message to the specified stream in RFC 822 format, without
	   * the specified headers.
	   * @exception IOException if an error occurs writing to the stream or in
	   * the data handler layer
	   */
	  public void writeTo(OutputStream os, String[] ignoreList)
	    throws IOException, MessagingException, IllegalStateException
	  {
		  if(isWriten) {
			  throw new IllegalStateException("EASMIMEMessage can be writen only one time");
		  }
		  if (os == null) {
	            throw new IllegalArgumentException("Output stream may not be null");
	      }
		  if (!saved)
	      {
	        saveChanges();
	      }

	    String charset = "US-ASCII"; // MIME default charset
	    byte[] sep = new byte[] { 0x0d, 0x0a };

	    // Write the headers
	    for (Enumeration e = getNonMatchingHeaderLines(ignoreList);
	         e.hasMoreElements(); )
	      {
	        String line = (String) e.nextElement();
	        StringTokenizer st = new StringTokenizer(line, "\r\n");
	        int count = 0;
	        while (st.hasMoreTokens())
	          {
	            String line2 = st.nextToken();
	            if (count > 0 && line2.charAt(0) != '\t')
	              {
	                // Folded line must start with tab
	                os.write(0x09);
	              }
	            /*
	             * RFC 2822, section 2.1 states that each line should be no more
	             * than 998 characters.
	             * Ensure that any headers we emit have no lines longer than
	             * this by folding the line.
	             */
	            int max = (count > 0) ? 997 : 998;
	            while (line2.length() > max)
	              {
	                String left = line2.substring(0, max);
	                byte[] bytes = left.getBytes(charset);
	                os.write(bytes);
	                os.write(sep);
	                os.write(0x09);
	                line2 = line2.substring(max);
	                max = 997; // make space for the tab
	              }
	            byte[] bytes = line2.getBytes(charset);
	            os.write(bytes);
	            os.write(sep);
	            count++;
	          }
	      }
	    os.write(sep);
	    os.flush();
	    if (modified || content == null && contentStream == null)
	      {
	        // use datahandler
	        os = MimeUtility.encode(os, getEncoding());
	        getDataHandler().writeTo(os);
	        os.flush();
	      }
	    else
	      {
	        // write content directly
	    	InputStream is = mMessageInuputStream;
		byte[] b = new byte[bufferSize];
		for (int l = is.read(b); l != -1; l = is.read(b)) 
	        {
	          os.write(b, 0, l);
	        }
		is.close();
		os.flush();
		isWriten = true;
	      }
	    
	  }
}
