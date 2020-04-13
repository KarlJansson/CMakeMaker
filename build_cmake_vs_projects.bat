cmakemaker

if not exist .\build_release\ (
  mkdir .\build_release\ 
)

cd .\build_release\ 
cmake -G "Visual Studio 16 2019" -A x64 .. 
cd .. 
