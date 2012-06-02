/*
 * SentDateTerm.java
 * Copyright (C) 2002 The Free Software Foundation
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

package javax.mail.search;

import java.util.Date;
import javax.mail.Message;

/**
 * A comparison of message sent dates.
 *
 * @author <a href="mailto:dog@gnu.org">Chris Burdess</a>
 * @version 1.4
 */
public final class SentDateTerm
  extends DateTerm
{

  /**
   * Constructor.
   * @param comparison the comparison operator
   * @param date the date for comparison
   */
  public SentDateTerm(int comparison, Date date)
  {
    super(comparison, date);
  }

  /**
   * Returns true only if the given message's sent date matches the
   * specified date using the specified operator.
   */
  public boolean match(Message msg)
  {
    try
      {
        Date d = msg.getSentDate();
        if (d != null)
          {
            return super.match(d);
          }
      }
    catch (Exception e)
      {
      }
    return false;
  }

  public boolean equals(Object other)
  {
    return (other instanceof SentDateTerm && super.equals(other));
  }
  
}

