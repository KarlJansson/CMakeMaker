cmakemaker

if not exist ..\BuildFiles\ (
  mkdir ..\BuildFiles\ 
)

if not exist ..\BuildFiles\public_cmakemaker\ (
  mkdir ..\BuildFiles\public_cmakemaker\ 
)

cd ..\BuildFiles\public_cmakemaker\ 
cmake -G "Visual Studio 14 2015 Win64" ../../public_cmakemaker/ 
cd ..\..\public_cmakemaker\ 
