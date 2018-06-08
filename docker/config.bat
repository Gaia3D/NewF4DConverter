@ECHO OFF

REM Docker organization to pull the images from
SET ORG=gaia3d

REM  Name of image
SET IMG=centos

REM Docker TAG
SET TAG=dev

REM Docker Image
SET IMAGE="%ORG%/%IMG%:%TAG%"

REM Container
SET CONTAINER=mago3d

SET PORT_SSH=20022