WITSML Input Adapter Project
============================

Supports both Win and Linux platform
(uses Boost cross-platform library)

Build
=====
1. Setup Project
   Project should have the same kind of architecture as ESP Server (32/64 bit)
2.1. OPTIONAL: Build Boost library
   Boost library should be installed separatelly
   - download Boost (preffered v.1.54.0): http://www.boost.org/users/download/
   - unpack and build it:
   [win, 64-bit]
     * unpack it in any directory [e.g. C:\boost-build]
	 * open VS Command Prompt: 
	   {start} -> {Visual Studio} -> {Command prompt x64 Win64}
	 * go into directory where Boost was unpacked and run: 
	   ...\>bootstrap.bat
	   and than: 
	   ...\>b2 --toolset=msvc-10.0 architecture=x86 address-model=64
	     - this will build Boost for x64, libraries will be in ./stage directory
	 * copy Boost include files into Visual C++'s standard include directory
	   [...\>boost --> $(VCInstallDir)/include/boost]
	   (it will allow to include Boost headers as: #include <boost/thread.hpp>
	 * copy Boost lib files into Visual C++'s standard lib directory
	   [...\>stage/lib --> $(VCInstallDir)/lib/amd64" as I did] 
   [linux, 64-bit]
	 * unpack Boost archive somewhere
	 * under root user run ./bootstrap.sh
	   (you can use --show-libraries option to view total list of libs)
	 * under root user run ./b2 install architecture=x86 address-model=64
	   - this will automatically put Boost include files into /usr/local/include 
	     and Boost libs into /usr/local/lib64 (or /usr/local/lib)
	   - if libs were placed into /usr/local/lib, move it to /usr/local/lib64
	 * update loader cache (sudo ldconfig)
2.2. OPTIONAL: Install Sybase ESP Server
   Download last version from (64-bit): http://downloads.sybase.com
   Generate license here: https://sybase.subscribenet.com/
   [win, 64-bit] 
     * perform GUI Typical Installation
	 * check C:\Sybase files & folders permissions (if some objects are
	   inaccessible for current user, own them)
   [linux, 64-bit]
     * install in GUI Mode, Standard User, Typical installation
	 * as root create /opt/sybase, then change owner to Standard User
	 * run setup.bin as Standard User and perform typical installation
	   into /opt/sybase
	 * modify /opt/sybase/SYBASE.sh
	   - add adapters library paths to LD_LIBRARY_PATH
	     (/opt/sybase/ESP-5_1/lib/adapters:/opt/sybase/ESP-5_1/lib)
	   - append to the end the command to run ESP studio
	     (/opt/sybase/ESP-5_1/studio/esp_studio)
	 * start ESP studio as Standard User by command ./SYBASE.sh
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
3. Run WITSMLER utility (in ./release directory)
   - mode1: 
     $ copy example2.xml from ./witsmler to ./release
     $ witsmler.a localhost 12345 example2.xml 100
   - mode2: 
     $ copy *.xml, *.csv files from ./witsmler to ./release
	 $ witsmler.a localhost 12345 template2.xml settings.xml
4. Ensure cvs lines can be viewed in StreamView window 
   (ctx_menu: <Show In> for DataTable and ColumnTable in Run-Test Perspective)
6. Stop Project

Using
=====
A. Logging facility
   Adapter has two separate logging streams:
   1. Internal log into text files (can log at any time):
     a) logs to files from the adapter start point
	    (linux default '/tmp/', windows 'C:\Temp')
	 b) configured via adapter description XML file in adapter home 
	    directory (c:\Sybase\ESP-5_1\lib\adapters\witsml_in.cnxml):
		- section <Internal id="x_InternalLogEnable" ...>
		- section <Internal id="x_InternalLogPath" ...>
		* So the configuration of Internal log is Global and static 
		  (ESP Studio & Server restart required to changes take effect)
		* Status of Internal log is printed by Standard ESP Log 
		  (INTERNAL_LOG:ON/OFF)
		* Configuration should be different for Windows and Linux
		  (due to different InternalLogPath in those OSs)
	 c) current logged events
		"DllMain", INFO, "Adapter attached to process"
		"DllMain", INFO, "Adapter deattached from process"
		"DllExport", ERROR, "exported API::getNext(): message Queue::Pop() failed"
		"DllExport", ERROR, "exported API::getNext(): wrong columns count in prepared row"
		"DllExport", ERROR, "exported API::getNext(): adapter library rejected prepared row"
		"DllExport", INFO,  "exported API::createAdapter() is called"
		"DllExport", INFO,  "exported API::deleteAdapter() is called"
		"DllExport", INFO,  "exported API::setCallBackReference() is called"
		"DllExport", INFO,  "exported API::getNext(): waiting for message"
		"DllExport", INFO,  "exported API::getNext(): message waiting was interrupted"
		"DllExport", INFO,  "exported API::getNext(): waiting for message completed"
		"DllExport", INFO,  "exported API::reset() is called"
		"DllExport", INFO,  "exported API::start() is called"
		"DllExport", INFO,  "exported API::stop() is called"
		"DllExport", INFO,  "exported API::cleanup() is called"
		"InputAdapter", ERROR, "Failed to create listen socket"
		"InputAdapter", ERROR, "Failed to parse incoming WitsML"
		"InputAdapter", INFO,  "Starting on XXXX port"
		"InputAdapter", INFO,  "Adapter has stopped"
        "InputAdapter", ERROR, "Failed open file with Witsml Rules"
        "InputAdapter", ERROR, "Witsml Rule file processing failed"
		"InputAdapter", INFO,  "Incoming connection acceptor has started"
		"InputAdapter", INFO,  "New incoming connection has accepted"
		"InputAdapter", INFO,  "Incoming connection acceptor has finished"
		"InputAdapter", INFO,  "New client processor has started"
		"InputAdapter", INFO,  "New client processor has finished"
		"InputAdapter", DEBUG, "<MSG-CHUNK>"
		"InputAdapter", DEBUG, "Adapter Object has created"
   2. Sybase ESP standard logging (can log only after was initialized by ESP):
     a) logs to screen in realtime (Console window) and to file in project 
	    directory (e.g. C:\SybaseESP\5.1\workspace\default.witsml.0\esp_server.log),
		but starts only after initialization of adapter has completed
	 b) enable in Studio:
	    - Authoring Perspective
		- right click on witsml.ccr -> Open with - Project Configuration Editor
		then for Studio SP01:
		- open Advanced tab -> in List: Project Deployment - Project Options
		- edit (or add new) option 'debug-level', set value [0-7]
		for Studio SP04:
		- open Advanced tab -> in List select Project Deployment
		- on the right side modify 'Debug Level' option, set value [0-7]
		Available Levels values:
		0:EMERG,1:ALERT,2:CRITICAL,3:ERR[default],4:WARNING,5:NOTICE,6:INFO,7:DEBUG
     c) current logged events (to view all of them, specify at least debug-level=6)
	    L_INFO - "Adapter has started (INTERNAL_LOG:ON/OFF)"
	    L_INFO - "Adapter has stopped"
 	    L_INFO - "getNext::Waiting interrupted"
	    L_ERR  - "getNext::Pop failed"
	    L_ERR  - "getNext::Wrong columns count"
	    L_ERR  - "getNext::toRow failed"
		L_ERR  - "Failed open file with Witsml Rules"
		L_ERR  - "Witsml Rule file processing failed"
		