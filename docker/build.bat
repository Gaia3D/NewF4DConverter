@ECHO OFF

CLS
ECHO "Building Dockerfile"
ECHO.

CALL config.bat

docker build ^
		--tag "%IMAGE%" ^
		--build-arg IMAGE_VERSION="%IMG_VER%" ^
		--build-arg BUILD_DATE=%date% ^
        --file Dockerfile ^
		--compress ^
		.
		
docker build ^
		--tag "%IMAGE_LATEST%" ^
		--build-arg IMAGE_VERSION="%IMG_VER%" ^
		--build-arg BUILD_DATE=%date% ^
        --file Dockerfile ^
		--compress ^
		.