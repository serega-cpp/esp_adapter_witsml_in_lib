WITSML Input Adapter Project
============================

Supports both Win and Linux platform
(uses Boost cross-platform library)

Build
=====
1. Setup Project
   Project should have the same kind of architecture as ESP Server (32/64 bit)
2. OPTIONAL (project includes pre-builded boost libraries): Build Boost library
   Boost library should be installed separatelly
   - download Boost (preffered v.1.54.0): http://www.boost.org/users/download/
   - unpack and build it:
   [win, 64-bit]
     * unpack it in any directory [e.g. C:\boost-build]
	 * open VS Command Prompt: 
	   {start} -> {Visual Studio} -> {Command prompt x64 Win64}
	 * go into directory where Boost was unpacked and run: 
	   ...\>bootstrap.bat --with-libraries=thread
	   and than: 
	   ...\>b2 --toolset=msvc-10.0 architecture=x86 address-model=64
	   - this will build Boost for x64, libraries will be in ./stage directory
	 * copy Boost include files into Visual C++'s include directory
	   [...\>boost --> $(VCInstallDir)/include/boost]
	   (it will allow to include Boost headers as: #include <boost/thread.hpp>
	 * copy Boost lib files somewhere
	   [...\>stage/lib --> $(VCInstallDir)/boost/lib64" as I did] 
   [linux, 64-bit]
	 * under root user unpack Boost archive into /usr/local
	 * run ./bootstrap.sh --with-libraries=thread
	 * run ./b2 install architecture=x86 address-model=64
	   - this will automatically put Boost include files into /usr/local/include 
	     and Boost libs into /usr/local/lib64; no additional actions required!
3. Build project
   [win] Open Solution file in Visual Studio 2010+.
           - Specify Lib-path to the boost lib directory in Project settings
		     [on step 2 we've copied them into $(VCInstallDir)/boost/lib64]
		   - Build Project and Find executables in '.\x64\Release'
   [linux] - make sure environment variable $ESP_HOME is set
           - run ./make.sh in project directory (to clean build run ./clean.sh)
           - find executables in './release'

Install
=======
1. Copy executable files to the ESP Adapter directory [%ESP_HOME%/lib/adapters]
   [win] 'esp_adapter_witsml_in_lib.dll' and 'esp_adapter_witsml_in_lib.lib'
   [linux] 'libesp_adapter_witsml_in_lib.so'
2. Copy adapter description file to the %ESP_HOME%/lib/adapters
   'witsml_in.cnxml' file
3. Restart ESP Studio (new Custom adapter name should appear in Input Adapters)

Test
====
1. Open ESP-project (based on ./witsml_in.ccl)
   - create new empty ESP Project
   - double click on *.ccl file in ESP Project exlorer
   - switch into Text view (F4 or F6 or with right-click on view)
   - paste content of ./witsml_in.ccl into the New Project's ccl file
   - save Project and return into Visual view
2. Specify Adapter setting [if needed]: port (default: port:12345)
2. Start project
3. Start WITSMLER utility [witsmler.a example_witsml_1.xml localhost 12345]
4. Ensure cvs lines can be viewed in StreamView window (Show In ctx_menu for 
   CustomWindow in Run-Test Perspective)
6. Stop Project

WitsML Processing Algorithm (PoC, Draft version)
================================================

All XML data are transferred into ESP to the one Table/InputWindow.
Table/InputWindow has the follwing columns:
 UID string    - Unique ID for processed XML file
 SALT integer  - Sequence Number (used to create Key -> PRIMARY KEY (UID,SALT))
 GRP string    - Group Name (XML Node with children and attributes inside)
 FNAME string  - XML Parameter Name
 FVALUE string - XML Parameter Value

Note: UID value is equls to a Value of parameter with Name = uidXXX, 
      where XXX is first-level XML Node name in file.

Sample:

<?xml version="1.0" encoding="UTF-8"?>
<wellbores version="1.2.0">
 <wellbore uidSource="Server1" uidWellbore="Well123" uidWell="Well123">
  <nameWell>DemoWell2</nameWell>
  <typeWellbore>Re-entry</typeWellbore>
  <shape>Deviated</shape>
  <tvdPlanned uom="m">1167.65</tvdPlanned>
  <commonData>
   <nameSource>SC001</nameSource>
   <dTimStamp>2007-06-11T14:05:50.6940000+02:00</dTimStamp>
  </commonData>
 </wellbore>
</wellbores>

will be translated (UID Name=uidWellbore Value=Well123)

Well123, 8, wellbore, uidSource, Server1
Well123, 7, wellbore, uidwellbore, Well123
Well123, 9, wellbore, uidWell, Well123
Well123, 10, wellbore, nameWell, DemoWell2
Well123, 13, wellbore, typeWellbore, Re-entry
Well123, 14, wellbore, shape, Deviated
Well123, 19, wellbore, tvdPlanned, 1167.65
Well123, 2, commonData, nameSource, SC001
Well123, 3, commonData, dTimStamp, 2007-06-11T14:05:50.6940000+02:00
