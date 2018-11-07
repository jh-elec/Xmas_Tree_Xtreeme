::/*
::* Fuses setzen damit später mit mehr Speed programmiert werden kann..
::*/

H:\Fertigung_QP\Homann\avrdude\avrdude.exe -c avrispmkII -B 125kHz -p m32 -U hfuse:w:0xD9:m -U lfuse:w:0xFF:m

::/*
::* Jetzt programmieren..
::*/

H:\Fertigung_QP\Homann\avrdude\avrdude.exe -c avrispmkII -B 2MHz -p m32 -e -U flash:w:"H:\Artikel\Software_Binaerdateien\300610\300610.hex":i -u