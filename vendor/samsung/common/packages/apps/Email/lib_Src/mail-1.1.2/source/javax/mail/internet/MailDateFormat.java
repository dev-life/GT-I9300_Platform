/*
 * MailDateFormat.java
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

package javax.mail.internet;

import java.io.PrintStream;
import java.text.DecimalFormat;
import java.text.FieldPosition;
import java.text.NumberFormat;
import java.text.ParsePosition;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.TimeZone;

/**
 * A date format that applies the rules specified by the Internet Draft
 * draft-ietf-drums-msg-fmt-08 dated January 26, 2000.
 * <p>
 * This class cannot take pattern strings. It always formats the date
 * based on the above specification.
 *
 * @author <a href="mailto:dog@gnu.org">Chris Burdess</a>
 * @version 1.4
 */
public class MailDateFormat
  extends SimpleDateFormat
{

  private static final String[] DAYS_OF_WEEK = {
    null, "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };

  private static final String[] MONTHS = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  public MailDateFormat()
  {
    //super("EEE, d MMM yyyy HH:mm:ss 'ZZZZZ'", Locale.US);
    calendar = new GregorianCalendar(TimeZone.getTimeZone("GMT"));
    numberFormat = new DecimalFormat();
  }

  /**
   * Appends the string representation for the specified field to the
   * given string buffer. This method should be avoided, use
   * <code>format(Date)</code> instead.
   * @param date the date
   * @param buf the buffer to append to
   * @param field the current field position
   * @return the modified buffer
   */
  public StringBuffer format(Date date, StringBuffer buf,
                             FieldPosition field)
  {
    calendar.clear();
    calendar.setTime(date);
    buf.setLength(0);

    // Day of week
    buf.append(DAYS_OF_WEEK[calendar.get(Calendar.DAY_OF_WEEK)]);
    buf.append(',');
    buf.append(' ');

    // Day of month
    buf.append(Integer.toString(calendar.get(Calendar.DAY_OF_MONTH)));
    buf.append(' ');

    // Month
    buf.append(MONTHS[calendar.get(Calendar.MONTH)]);
    buf.append(' ');

    // Year
    int year = calendar.get(Calendar.YEAR);
    if (year < 1000)
      {
        buf.append('0');
        if (year < 100)
          {
            buf.append('0');
            if (year < 10)
              {
                buf.append('0');
              }
          }
      }
    buf.append(Integer.toString(year));
    buf.append(' ');

    // Hour
    int hour = calendar.get(Calendar.HOUR_OF_DAY);
    buf.append(Character.forDigit(hour / 10, 10));
    buf.append(Character.forDigit(hour % 10, 10));
    buf.append(':');

    // Minute
    int minute = calendar.get(Calendar.MINUTE);
    buf.append(Character.forDigit(minute / 10, 10));
    buf.append(Character.forDigit(minute % 10, 10));
    buf.append(':');

    // Second
    int second = calendar.get(Calendar.SECOND);
    buf.append(Character.forDigit(second / 10, 10));
    buf.append(Character.forDigit(second % 10, 10));
    buf.append(' ');

    // Timezone
    // Get time offset in minutes
    int zoneOffset = (calendar.get(Calendar.ZONE_OFFSET) +
                      calendar.get(Calendar.DST_OFFSET)) / 60000;
    
    // Apply + or - appropriately
    if (zoneOffset < 0)
      {
        zoneOffset = -zoneOffset;
        buf.append('-');
      }
    else
      {
        buf.append('+');
      }
    
    // Set the 2 2-char fields as specified above
    int tzhours = zoneOffset / 60;
    buf.append(Character.forDigit(tzhours / 10, 10));
    buf.append(Character.forDigit(tzhours % 10, 10));
    int tzminutes = zoneOffset % 60;
    buf.append(Character.forDigit(tzminutes / 10, 10));
    buf.append(Character.forDigit(tzminutes % 10, 10));

    field.setBeginIndex(0);
    field.setEndIndex(buf.length());
    return buf;
  }

  /**
   * Parses the given date in the format specified by
   * draft-ietf-drums-msg-fmt-08 in the current TimeZone.
   * @param text the formatted date to be parsed
   * @param pos the current parse position
   */
  public Date parse(String text, ParsePosition pos)
  {
    int start = 0, end = -1;
    int len = text.length();
    calendar.clear();
    pos.setIndex(start);
    try
      {
        // Advance to date
        if (Character.isLetter(text.charAt(start)))
          {
            start = skipNonWhitespace(text, start, len);
          }
        start = skipWhitespace(text, start, len);
        pos.setIndex(start);
        end = skipNonWhitespace(text, start + 1, len);
        int date = Integer.parseInt(text.substring(start, end));
        // Advance to month
        start = skipWhitespace(text, end + 1, len);
        pos.setIndex(start);
        end = skipNonWhitespace(text, start + 1, len);
        String monthText = text.substring(start, end);
        int month = -1;
        for (int i = 0; i < 12; i++)
          {
            if (MONTHS[i].equals(monthText))
              {
                month = i;
                break;
              }
          }
        if (month == -1)
          {
            pos.setErrorIndex(end);
            return null;
          }
        // Advance to year
        start = skipWhitespace(text, end + 1, len);
        pos.setIndex(start);
        end = skipNonWhitespace(text, start + 1, len);
        int year = Integer.parseInt(text.substring(start, end));
        calendar.set(Calendar.YEAR, year);
        calendar.set(Calendar.MONTH, month);
        calendar.set(Calendar.DAY_OF_MONTH, date);
        // Advance to hour
        start = skipWhitespace(text, end + 1, len);
        pos.setIndex(start);
        end = skipToColon(text, start + 1, len);
        int hour = Integer.parseInt(text.substring(start, end));
        calendar.set(Calendar.HOUR, hour);
        // Advance to minute
        start = end + 1;
        pos.setIndex(start);
        end = skipToColon(text, start + 1, len);
        int minute = Integer.parseInt(text.substring(start, end));
        calendar.set(Calendar.MINUTE, minute);
        // Advance to second
        start = end + 1;
        pos.setIndex(start);
        end = skipNonWhitespace(text, start + 1, len);
        int second = Integer.parseInt(text.substring(start, end));
        calendar.set(Calendar.SECOND, second);
        
        if (end != len)
          {
            start = skipWhitespace(text, end + 1, len);
            if (start != len)
              {
                // Trailing characters, therefore timezone
                end = skipNonWhitespace(text, start + 1, len);
                char pm = text.charAt(start);
                if (Character.isLetter(pm))
                  {
                    TimeZone tz =
                      TimeZone.getTimeZone(text.substring(start, end));
                    calendar.set(Calendar.ZONE_OFFSET, tz.getRawOffset());
                  }
                else
                  {
                    int zoneOffset = 0;
                    zoneOffset +=
                      600 * Character.digit(text.charAt(++start), 10);
                    zoneOffset +=
                      60 * Character.digit(text.charAt(++start), 10);
                    zoneOffset +=
                      10 * Character.digit(text.charAt(++start), 10);
                    zoneOffset +=
                      Character.digit(text.charAt(++start), 10);
                    zoneOffset *= 60000; // minutes -> ms
                    if ('-' == pm)
                      {
                        zoneOffset = -zoneOffset;
                      }
                    calendar.set(Calendar.ZONE_OFFSET, zoneOffset);
                  }
              }
          }
        pos.setIndex(end);
        
        return calendar.getTime();
      }
    catch (NumberFormatException e)
      {
        pos.setErrorIndex(Math.max(start, end));
      }
    catch (StringIndexOutOfBoundsException e)
      {
        pos.setErrorIndex(Math.max(start, end));
      }
    return null;
  }

  private int skipWhitespace(final String text, int pos, final int len)
  {
    while (pos < len && Character.isWhitespace(text.charAt(pos)))
      {
        pos++;
      }
    return pos;    
  }

  private int skipNonWhitespace(final String text, int pos, final int len)
  {
    while (pos < len && !Character.isWhitespace(text.charAt(pos)))
      {
        pos++;
      }
    return pos;    
  }

  private int skipToColon(final String text, int pos, final int len)
  {
    while (pos < len && text.charAt(pos) != ':')
      {
        pos++;
      }
    return pos;    
  }

  /**
   * Don't allow setting the calendar.
   */
  public void setCalendar(Calendar newCalendar)
  {
    throw new UnsupportedOperationException();
  }

  /**
   * Don't allow setting the NumberFormat.
   */
  public void setNumberFormat(NumberFormat newNumberFormat)
  {
    throw new UnsupportedOperationException();
  }

}

