WitsML protocol adapter for Sybase ESP
======================================

Used to receive raw data in WitsML protocol and put it
into specially prepared ESP Project. Project conceptually
split data into two dimensions - data columns (attributes)
and data rows (values). Adapter has own XML language to
describe attributes and values nodes in incoming data.

Version: 0.1, currently under development, not released.

Requirements:
+ Sybase ESP 5.1
+ gcc43/msvc10

This repository contains 3 projects:
+ **esp_adapter_witsml_in_lib** - WitsML adapter
+ **witsmler** - WitsML source [*send xml file with WitsML for testing*]
+ **esp_adapter_debug** - helper project [*for debugging*]

More information about building, installation and using see in 
*ReadMe.txt* file in project directory.

In case of other questions fill free to contact me:
serega.cpp@gmail.com