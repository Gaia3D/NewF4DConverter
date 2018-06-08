@ECHO OFF

CLS
ECHO "Building Dockerfile"
ECHO.

CALL config.bat

docker build ^
		--tag "%IMAGE%" ^
		--build-arg IMAGE="%IMAGE%" ^
		--build-arg BUILD_DATE=%date% ^
        --file Dockerfile ^
		.