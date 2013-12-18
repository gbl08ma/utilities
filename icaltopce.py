#!/usr/bin/python
from icalendar import Calendar, Event, vDatetime
from datetime import datetime, time, date, timedelta
import os, sys, unicodedata
from pytz import timezone
import pytz
caltz = timezone('UTC')
print "iCalendar to Portable Calendar Event converter - (C) 2013 tny. internet media"
if len(sys.argv) == 1:
  print "Usage: icaltopce.py fileToConvert.ics"
  print "ERROR: You must specify a file to convert as an argument"
  sys.exit()
print "Conversion started"
g = open(str(sys.argv[1]),'rb')
gcal = Calendar.from_ical(g.read())
totalfiles = 0
totaleventcount = 0
lastConvertedEvent = 0
lastCalendarName = ""
if len(sys.argv) == 3:
  category = str(sys.argv[2])
else:
  category = 1
while 1:
  if totalfiles == 99: break;
  totalfiles = totalfiles + 1
  if totalfiles < 10: filename = '0000' + str(totalfiles) + '.pce'
  else: filename = '000' + str(totalfiles) + '.pce'
  o = open(filename,'wb')
  o.write("PCALEVT")
  eventcount = 0
  curevent = 0
  for component in gcal.walk():
    if component.name == "VCALENDAR":
      if component.get('x-wr-timezone'):
        caltz = timezone(component.get('x-wr-timezone'))
      if component.get('x-wr-calname'):
        if lastCalendarName != str(component.get('x-wr-calname')):
          print 'Parsing calendar "' + str(component.get('x-wr-calname')) + '"'
          print 'Please type the category (color) for the events in this calendar:'
          rcategory = int(input("0-invisible; 1-black; 2-blue, 3-green, 4-cyan, 5-red, 6-pink, 7-yellow: "))
          if rcategory < 0 or rcategory > 7:
            print "Invalid category, aborting"
            sys.exit()
          else: category = rcategory
        lastCalendarName = str(component.get('x-wr-calname'))
    if component.name == "VEVENT":
      curevent = curevent + 1
      if curevent <= lastConvertedEvent:
        continue;
      lastConvertedEvent = lastConvertedEvent + 1
      if component.get('summary'): print "Converting: " + str(component.get('summary').encode('utf8'))
      o.write(str(category)) #category
      o.write("\x1d")
      o.write("0") #daterange
      o.write("\x1d")
      
      dts = datetime(2013, 1, 1, 0, 0, 0, tzinfo=timezone('UTC'))
      dte = datetime(2013, 1, 1, 0, 0, 0, tzinfo=timezone('UTC'))
      timed = 1
      if component.get('dtstart'):
        try:
          dts = component.get('dtstart').dt.replace(tzinfo=timezone('UTC'))
          dts = component.get('dtstart').dt.astimezone(caltz)
        except:
          dts = component.get('dtstart').dt
          dts = datetime(dts.year, dts.month, dts.day, 0, 0, 0, tzinfo=caltz)
          timed = 0
      if component.get('dtend'):
        try:
          dte = component.get('dtend').dt.replace(tzinfo=timezone('UTC'))
          dte = component.get('dtend').dt.astimezone(caltz)
        except:
          dte = component.get('dtend').dt
          dte = datetime(dte.year, dte.month, dte.day, 0, 0, 0, tzinfo=caltz) - timedelta(1)
          timed = 0
        
      # event start date:
      o.write(str(dts.day)) #start day
      o.write("\x1d")
      o.write(str(dts.month)) #start month
      o.write("\x1d")
      o.write(str(dts.year)) #start year
      o.write("\x1d")
      # event end date:
      o.write(str(dte.day)) #end day
      o.write("\x1d")
      o.write(str(dte.month)) #end month
      o.write("\x1d")
      o.write(str(dte.year)) #end year
      o.write("\x1d")
      # day of week (start)
      if dts.weekday == 6: weekday = 0
      else: weekday = dts.weekday() + 1
      o.write(str(weekday))
      o.write("\x1d")
      o.write("0") # repeat
      o.write("\x1d")
      o.write(str(timed))
      o.write("\x1d")
      
      # event start time:
      o.write(str(dts.hour)) #start hour
      o.write("\x1d")
      o.write(str(dts.minute)) #start minute
      o.write("\x1d")
      o.write(str(dts.second)) #start second
      o.write("\x1d")
      # event end time:
      o.write(str(dte.hour)) #end hour
      o.write("\x1d")
      o.write(str(dte.minute)) #end minute
      o.write("\x1d")
      o.write(str(dte.second)) #end second
      o.write("\x1d")
      
      if component.get('summary'):
        o.write(unicodedata.normalize('NFKD',component.get('summary')).encode('ascii','ignore')[:21]) #title
      o.write("\x1d") # write field separator even if field is empty
      if component.get('location'):
        o.write(unicodedata.normalize('NFKD',component.get('location')).encode('ascii','ignore')[:128]) #location
      o.write("\x1d")
      # if summary was too big to fit in title, put it in description too:
      remainDescChars = 1024
      if component.get('summary'):
        if len(unicodedata.normalize('NFKD',component.get('summary')).encode('ascii','ignore')) > 21:
          o.write("Summary: ")
          remainDescChars = remainDescChars - 11 # len("Summary: ") + len("; ")
          o.write(unicodedata.normalize('NFKD',component.get('summary')).encode('ascii','ignore')[:remainDescChars])
          remainDescChars = remainDescChars - len(unicodedata.normalize('NFKD',component.get('summary')).encode('ascii','ignore')[:remainDescChars])
          o.write("; ")
      if component.get('description'):
        o.write(unicodedata.normalize('NFKD',component.get('description')).encode('ascii','ignore')[:remainDescChars]) #description
      o.write("\x1e")
      eventcount = eventcount + 1
      totaleventcount = totaleventcount + 1
      if eventcount == 100:
        break;
  o.close()
  if eventcount == 0:
    os.remove(filename)
    break;
g.close()
print "Converted " + str(totaleventcount) + " events."
print "To import the events into the Utilities calendar:"
print "Connect your calculator to the computer through USB (select F1:MassStorage when asked). Your calculator will appear as a flash drive, now place the *.pce files that were just created, in the folder called \"@UTILS\". If that folder doesn't exist, create it and put the files inside."
print "Safely remove your calculator and, from the main menu, open Utilities. Press F3, then EXE to select Calendar. Now, press the OPTN key and select \"Import events\". Finally, press F1 and the import will take place."