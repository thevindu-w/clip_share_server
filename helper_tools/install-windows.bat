@ECHO OFF

NET SESSION >NUL 2>&1
if %ERRORLEVEL% == 0 (
    ECHO This installer must not be run as administrator.
    ECHO Please run without administrator rights.
    ECHO Installation aborted.
    PAUSE
    EXIT
)

IF NOT EXIST clip_share*.exe (
    ECHO 'clip_share.exe' file does not exist.
    ECHO Please download the 'clip_share.exe' Windows version and place it in this folder.
    ECHO Install failed.
    PAUSE
    EXIT
)

ECHO This will install clip_share to run on startup.
SET /P confirm=Proceed? [y/N] 
IF /I NOT "%confirm%" == "y" (
    ECHO Aborted.
    ECHO You can still use clip_share by manually running the program.
    PAUSE
    EXIT
)

RENAME clip_share*.exe clip_share.exe
MKDIR "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\" >NUL 2>&1
COPY clip_share.exe "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\clip_share.exe" >NUL 2>&1

CD %USERPROFILE%
IF NOT EXIST clipshare.conf (
    MKDIR "%USERPROFILE%\Downloads" >NUL 2>&1
    ECHO working_dir=%USERPROFILE%\Downloads>clipshare.conf
    ECHO Created new configuration file %USERPROFILE%\clipshare.conf
)
ECHO Installed ClipShare to run on startup.

SET /P start_now=Start ClipShare now? [y/N] 
IF /I "%start_now%" == "y" (
    START "" "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\clip_share.exe"
    ECHO Started ClipShare server.
)
PAUSE
EXIT
