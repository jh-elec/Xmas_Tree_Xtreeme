@echo off
ECHO ##############################################
ECHO MAKEFILE zur Programmierung des Atmel ATmega32
ECHO Projekt: XMAS_TREE_XTREEME
ECHO Nur fuer die Verwendung mit dem Programmer AVRISP mkII (USB)
ECHO Dieses Batch-File muss aus dem Arbeits-Verzeichnis
ECHO aufgerufen werden !
ECHO Datum: 27.10.2014
ECHO H:\Artikel\Software_Binaerdateien\300610\300610.hex
ECHO made by Hm & Wl
ECHO ##############################################

 
REM /* FUSE REGISTER ( High Byte: 0xD9, Low Byte: 0xFF ) */

REM ( OCDEN     = DISABLE 				   )
REM ( JTAGEN    = DISABLE 				   )
REM ( SPIEN     = ENABLE  				   )
REM ( CKOPT     = DISABLE 				   )
REM ( EESAVE    = DISABLE 				   )
REM ( BOORSZ    = 2048_3800 			   )
REM ( BOOTRST   = DISABLE   			   )
REM ( BODLEVEL  = 2V7       			   )
REM ( BODEN	  = DISABLE     			   )
REM ( SUT_CKSEL = EXTHIFXTALRES_16KCK_64MS )

REM /* Folgende Dateien müssen im Ordner vorhanden sein -> */
	
REM	( - AVRISP-mkII.dat  )
REM	( - STK500.ebn		 )
REM	( - Stk500.exe		 )
REM	( - Stk500Common.dll )
REM	( - Stk500Dll.dll	 )



REM /* Fuses programmieren und anschließend verifizieren + ISP Frequenz setzen ( muss  <= 125 Khz sein, da der µC im Auslieferungszustand auf 1MHZ Takt eingestellt ist. ISP Frequenz muss immer 1/4 davon betragen */
STK500.exe -cUSB -I125000 -dATmega32 -fD8BF -FD8BF -ms 

REM /* Aktuelle Spannung lesen (VTARGET), µC löschen, Serielles programmieren wählen, Datei Pfad auswählen, Flash programmieren, Flash verifizieren */
STK500.exe -cUSB -I2000000 -dATmega32 -wt -e -ms -ifH:\Artikel\Software_Binaerdateien\300610\300610.hex -pf -vf

REM /* Ermittlung der Fehler ( falls welche auftreten ) */
IF ERRORLEVEL 1 goto ERROR1
IF ERRORLEVEL 2 goto ERROR2
IF ERRORLEVEL 3 goto ERROR3
IF ERRORLEVEL 4 goto ERROR4
IF ERRORLEVEL 5 goto ERROR5
IF ERRORLEVEL 6 goto ERROR6
IF ERRORLEVEL 7 goto ERROR7
IF ERRORLEVEL 8 goto ERROR8

goto NO_ERROR

:ERROR1
       ECHO ERROR1
       REM ECHO Can't connect to AVRISP mkII from PC
       PAUSE
       goto ENDE
:ERROR2
       ECHO ERROR2
       REM ECHO Can't connect to AVRISP mkII from PC, USB driver is not installed properly
       PAUSE
       goto ENDE
:ERROR3
       ECHO ERROR3
       REM ECHO ISP cable is not mounted correctly
       PAUSE
       goto ENDE
:ERROR4
       ECHO ERROR4
       REM ECHO There is a problem on the reset line
       PAUSE
       goto ENDE
:ERROR5
       ECHO ERROR5
       REM ECHO The ISP cable is not mounted correctly, or some of the target pins are shorted to GND or VCC, or they are too heavily loaded
       PAUSE
       goto ENDE
:ERROR6
       ECHO ERROR6
       REM ECHO Can't detect target
       PAUSE
       goto ENDE
:ERROR7
       ECHO ERROR7
       REM ECHO The ISP frequency is high
       PAUSE
       goto ENDE
:ERROR8
       ECHO ERROR8
       REM ECHO AVRISP mkII will not work correctly with STK500
       PAUSE
       goto ENDE
:NO_ERROR
         ECHO ####################################
         ECHO #### Programmierung erfolgreich ####
         ECHO ####################################
         ECHO Nun auf die LED´s des Baumes achten.
         goto ENDE
		 

:ENDE


