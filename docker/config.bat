@ECHO OFF

REM Docker organization to pull the images from
SET ORG=gaia3d

REM Container
SET CONTAINER=mago3d

REM  Name of image
SET IMG=centos

REM root image version
SET IMG_VER=centos7

REM Docker TAG
SET TAG=latest

REM Docker Image
SET IMAGE="%ORG%/%CONTAINER%-converter:%IMG%-%IMG_VER%-%TAG%"
SET IMAGE_LATEST="%ORG%/%CONTAINER%-converter"
