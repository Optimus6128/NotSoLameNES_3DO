REM STEP 3 - covert to Opera file system ISO
tools\opera\3doiso.exe -in CD -out lamenes.iso
echo 3 of 4: ISO file system done

REM STEP 4 - Sign it
cd tools\sign
3doEncrypt.exe genromtags ..\..\lamenes.iso
echo 4 of 4: signed lamenes.iso
echo Great Success!

pause