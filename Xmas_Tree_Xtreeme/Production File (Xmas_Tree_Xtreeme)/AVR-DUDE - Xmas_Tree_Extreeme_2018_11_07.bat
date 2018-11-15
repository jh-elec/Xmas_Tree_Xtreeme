@echo off
chcp 1252

color E
cls

echo ( \/ )( \/ ) / _\ / ___)     (_  _)(  _ \(  __)(  __)
echo  )  ( / \/ \/    \\___ \ ____  )(   )   / ) _)  ) _) 
echo (_/\_)\_)(_/\_/\_/(____/(____)(__) (__\_)(____)(____)

::/*
::* Fuses setzen, um spÃ¤ter mit mehr Speed programmieren zu kÃ¶nnen ...
::*/

"H:\\Artikel\Pr�fsoftware\avrdude\avrdude.exe" -c avrispmkII -B 125kHz -p m32 -U hfuse:w:0xD9:m -U lfuse:w:0xFF:m


::/*
::* Starte Flashprozess ...
::*/

"H:\\Artikel\Pr�fsoftware\avrdude\avrdude.exe" -c avrispmkII -B 2MHz -p m32 -e -U flash:w:"H:\Artikel\Software_Binaerdateien\300610\300610.hex":i -u