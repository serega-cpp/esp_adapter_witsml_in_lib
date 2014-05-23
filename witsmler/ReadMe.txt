Witsmler (simple utility application for test esp_adapter_witsml_in_lib)
========================================================================

Used to test Sybase ESP Adapter for WitsML protocol. Just send XML
file (with WitsML) to ESP Adapter via TCP socket. Only one note --
utility adds special separator (&&) to the end of message. This
separator will be used by ESP Adapter for messages saparation.

Witsmler can be used in two modes:
1. Simple sending one specified XML file many times
2. Generate XML file based on specified template and send it

(mode1) Usage:
$ witsmler.a localhost 8001 example2.xml 100
       where:
	   localhost 8001 - ESP Adapter listen host and port
	   example2.xml   - file with WitsML
	   100			  - send count (file will be sent 100 times)

or

(mode2) Usage: 
$ witsmler.a localhost 8001 tempate2.xml settings.xml
       where:
	   localhost 8001 - ESP Adapter listen host and port
	   tempate2.xml   - file with WitsML template (contains placeholders @@STARTTIME@@, @@ENDTIME@@, @@DATA@@, @@TZ@@)
	   settings.xml	  - file with additional settings for generator

========================================================================
Using tips:

1. After Build successfully completed, binary executable ./witsmler is in ./release directory
2. To run WITSMLER utility (in ./release directory)
   - mode1: 
     $ copy example2.xml from ./witsmler to ./release
     $ witsmler.a localhost 12345 example2.xml 100
   - mode2: 
     $ copy *.xml, *.csv files from ./witsmler to ./release
	 $ witsmler.a localhost 12345 template2.xml settings.xml

========================================================================
settings.xml sample:

<?xml version="1.0" encoding="UTF-8"?>
<settings>
  <column>
    <file>col01.csv</file>
  	<index>1</index>
  </column>
  <column>
    <file>col02.csv</file>
	<index>2</index>
  </column>
  ...
  <column>
    <file>col14.csv</file>
	<index>14</index>
  </column>
  <beginTime>2014-04-28 00:00:00</beginTime>
  <endTime>2014-04-28 02:00:00</endTime>
  <timeZone>+02:00</timeZone>
  <timeStepSec>0.5</timeStepSec>
  <timeFilePeriodSec>30</timeFilePeriodSec>
</settings>

where:
<column> - describes one column of data.
		 <file> -> specifies CSV file with points of data for generator, used by interpolation algorithm.
		 <index> -> specify column index (index used to get the right order of <column> nodes only).
		 // The (amount of <column> nodes) is equal to (amount of <logCurveInfo> nodes - 1),
		 // due to the first <logCurveInfo> node describes the timestamp column which is in our data-model
		 // populated in separate field.

<beginTime> - start time for generation
<endTime>   - end time for generation
<timeZone>  - timezone (processed as a string)
<timeStepSec> - data generation step (seconds)
<timeFilePeriodSec> - [Witsmler can split generated data to the portions and send they in separated XML files],
                       this parameter specifies the max time period of generated data for one XML file (seconds)

========================================================================
col01.csv sample:

DATE	VALUE
2014-04-28 00:00:00	5.017
2014-04-28 01:00:00	5.099
2014-04-28 02:00:00	4.001

where:
- first line is a header always
- separator is a TAB character
- two values on line: TIMESTAMP [format YYYY-MM-DD HH:MM:SS] and VALUE [double]
- lines define a points for linear interpolation algorithm
