cmakemaker

if not exist .\build_release\ (
  mkdir .\build_release\ 
)

cd .\build_release\ 
cmake -G "Visual Studio 15 2017 Win64" .. 
cd .. 
