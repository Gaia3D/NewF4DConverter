@ECHO OFF

CALL config.bat

SET ERROR=0
SET RUN_DOCKER=
SET RUNNING=

:findDocker
    FOR /f %%i IN ('where docker') do SET RUN_DOCKER=%%i
    IF "%RUN_DOCKER%" == "" (
        SET ERROR=1
        ECHO "Error: the 'docker' command was not found.  Please install docker."
        GOTO end
        REM EXIT /b %ERRORLEVEL%
    )
GOTO checkContainer

:checkContainer
    FOR /f %%i IN ('docker ps -a -q --filter "name=%CONTAINER%"') do SET RUNNING=%%i
    IF /i not "%RUNNING%" == "" (
        ECHO "Stopping and removing the previous session..."
        GOTO cleanContainer
    )
GOTO run

:cleanContainer
    docker stop %CONTAINER%
    docker rm %CONTAINER%
GOTO run

:run
    docker run ^
        -d ^
        --privileged ^
        --name "%CONTAINER%" ^
        -p %PORT_SSH%:22 ^
        "%IMAGE%"
GOTO end

:printHelp
    REM Pint Help
    ECHO "Print Help"
GOTO end

:end
  IF %ERROR% == 1 ECHO "Startup of Docker container was unsuccessful.
  ECHO.
PAUSE