@echo off

cd /D "%~dp0"

IF exist "P:\_UAPIBase\" (
	echo Removing existing link P:\_UAPIBase
	rmdir "P:\_UAPIBase\"
)
IF exist "P:\_UniversalApi\" (
	echo Removing existing link P:\_UniversalApi
	rmdir "P:\_UniversalApi\"
)

mklink /J "P:\_UAPIBase\" "%cd%\_UAPIBase\"
mklink /J "P:\_UniversalApi\" "%cd%\_UniversalApi\"

echo Setup P Drive Complete

timeout 10