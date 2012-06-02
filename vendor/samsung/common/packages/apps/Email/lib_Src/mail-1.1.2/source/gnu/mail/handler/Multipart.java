/*
 * Multipart.java
 * Copyright(C) 2003 Chris Burdess <dog@gnu.org>
 *
 * This file is part of GNU JavaMail, a library.
 * 
 * GNU JavaMail is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 *(at your option) any later version.
 * 
 * GNU JavaMail is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
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

/*
 * @history : 03/15/2011, changed import to use additionnal.jar
*/

package gnu.mail.handler;

/*GPL->GNU@03/15/2011 - changed import to use additionnal.jar start*/
import myjava.awt.datatransfer.DataFlavor;
import myjava.awt.datatransfer.UnsupportedFlavorException;
/*GPL->GNU@03/15/2011 - changed import to use additionnal.jar end*/
import java.io.*;
import javax.activation.*;
import javax.mail.MessagingException;
import javax.mail.internet.MimeMultipart;

/**
 * A JAF data content handler for the multipart/* family of MIME content
 * types.
 * This provides the basic behaviour for any number of MimeMultipart-handling
 * subtypes which simply need to override their default constructor to provide
 * the correct MIME content-type and description.
 */
public class Multipart
  implements DataContentHandler
{

  /**
   * Our favorite data flavor.
   */
  protected DataFlavor flavor;
  
  /**
   * Generic constructor.
   */
  public Multipart()
  {
    this("multipart/*", "multipart");
  }
  
  /**
   * Constructor specifying the data flavor.
   * @param mimeType the MIME content type
   * @param description the description of the content type
   */
  public Multipart(String mimeType, String description)
  {
    flavor = new ActivationDataFlavor(javax.mail.internet.MimeMultipart.class,
        mimeType, description);
  }

  /**
   * Returns an array of DataFlavor objects indicating the flavors the data
   * can be provided in.
   * @return the DataFlavors
   */
  public DataFlavor[] getTransferDataFlavors()
  {
    DataFlavor[] flavors = new DataFlavor[1];
    flavors[0] = flavor;
    return flavors;
  }

  /**
   * Returns an object which represents the data to be transferred.
   * The class of the object returned is defined by the representation class
   * of the flavor.
   * @param flavor the data flavor representing the requested type
   * @param source the data source representing the data to be converted
   * @return the constructed object
   */
  public Object getTransferData(DataFlavor flavor, DataSource source)
    throws UnsupportedFlavorException, IOException
  {
    if (this.flavor.equals(flavor))
      return getContent(source);
    return null;
  }

  /**
   * Return an object representing the data in its most preferred form.
   * Generally this will be the form described by the first data flavor
   * returned by the <code>getTransferDataFlavors</code> method.
   * @param source the data source representing the data to be converted
   * @return a byte array
   */
  public Object getContent(DataSource source)
    throws IOException
  {
    try
    {
      return new MimeMultipart(source);
    }
    catch (MessagingException e)
    {
      /* This loses any attached exception */
      throw new IOException(e.getMessage());
    }
  }

  /**
   * Convert the object to a byte stream of the specified MIME type and
   * write it to the output stream.
   * @param object the object to be converted
   * @param mimeType the requested MIME content type to write as
   * @param out the output stream into which to write the converted object
   */
  public void writeTo(Object object, String mimeType, OutputStream out)
    throws IOException
  {
    if (object instanceof MimeMultipart)
    {
      try
      {
       ((MimeMultipart)object).writeTo(out);
      }
      catch (MessagingException e)
      {
        /* This loses any attached exception */
        throw new IOException(e.getMessage());
      }
    }
    else
      throw new UnsupportedDataTypeException();
  }

}
