/*
 * DataSourceDataContentHandler.java
 * Copyright (C) 2004 The Free Software Foundation
 * 
 * This file is part of GNU Java Activation Framework (JAF), a library.
 * 
 * GNU JAF is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * GNU JAF is distributed in the hope that it will be useful,
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

/*
 * @history : 03/15/2011, changed import to use additionnal.jar
*/

package javax.activation;

/*GPL->GNU@03/15/2011 - changed import to use additionnal.jar start*/
import myjava.awt.datatransfer.DataFlavor;
import myjava.awt.datatransfer.UnsupportedFlavorException;
/*GPL->GNU@03/15/2011 - changed import to use additionnal.jar end*/
import java.io.IOException;
import java.io.OutputStream;

/**
 * Data content handler using an existing DCH and a data source.
 *
 * @author <a href='mailto:dog@gnu.org'>Chris Burdess</a>
 * @version 1.1
 */
class DataSourceDataContentHandler
  implements DataContentHandler
{

  private DataSource ds;
  private DataFlavor[] flavors;
  private DataContentHandler dch;
  
  public DataSourceDataContentHandler(DataContentHandler dch, DataSource ds)
  {
    this.ds = ds;
    this.dch = dch;
  }
  
  public Object getContent(DataSource ds)
    throws IOException
  {
    if (dch != null)
      {
        return dch.getContent(ds);
      }
    else
      {
        return ds.getInputStream();
      }
  }
  
  public Object getTransferData(DataFlavor flavor, DataSource ds)
    throws UnsupportedFlavorException, IOException
  {
    if (dch != null)
      {
        return dch.getTransferData(flavor, ds);
      }
    DataFlavor[] tdf = getTransferDataFlavors();
    if (tdf.length > 0 && flavor.equals(tdf[0]))
      {
        return ds.getInputStream();
      }
    else
      {
        throw new UnsupportedFlavorException(flavor);
      }
  }
  
  public DataFlavor[] getTransferDataFlavors()
  {
    if (flavors == null)
      {
        if (dch != null)
          {
            flavors = dch.getTransferDataFlavors();
          }
        else
          {
            String mimeType = ds.getContentType();
            flavors = new DataFlavor[1];
            flavors[0] = new ActivationDataFlavor(mimeType, mimeType);
          }
      }
    return flavors;
  }

  public void writeTo(Object obj, String mimeType, OutputStream out)
    throws IOException
  {
    if (dch == null)
      {
        throw new UnsupportedDataTypeException("no DCH for content type " +
                                               ds.getContentType());
      }
    dch.writeTo(obj, mimeType, out);
  }
    
}

