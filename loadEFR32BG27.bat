@ECHO OFF
SETLOCAL
		
SET JLINK="C:\Program Files\SEGGER\JLink_V794b\JLink.exe"
		
%JLINK% -CommanderScript loadEFR32BG27.jlink
if %ERRORLEVEL% NEQ 0 GOTO ERROR
				
GOTO DONE
:ERROR
@ECHO An error occurred.
				
:DONE
	
ENDLOCAL
pause