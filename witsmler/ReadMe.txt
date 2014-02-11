Witsmler (simple utility application for test esp_adapter_witsml_in_lib)
========================================================================

Used to test Sybase ESP Adapter for WitsML protocol. Just send XML
file (with WitsML) to ESP Adapter via TCP socket. Only one interesting
thing -- utility adds special separator (&&) to the end of message. This
separator will be used by ESP Adapter for messages saparation.

Usage: witsmler.a example2.xml localhost 8001
       where:
	   example2.xml   - file with WitsML
	   localhost 8001 - ESP Adapter listen host and port
