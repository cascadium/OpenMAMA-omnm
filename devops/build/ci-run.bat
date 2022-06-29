set /p VERSION=<%GITHUB_WORKSPACE%\VERSION
set OPENMAMA_INSTALL_DIR=%GITHUB_WORKSPACE%\openmama-omnm.%VCVER%.%PLATFORM%

set PATH=C:\vcpkg\installed\%PLATFORM%-windows\bin;%OPENMAMA_INSTALL_DIR%\bin;%PATH%

choco install --no-progress -y unzip cmake || goto error
vcpkg install openmama:%PLATFORM%-windows gtest:%PLATFORM%-windows || goto error

mkdir %GITHUB_WORKSPACE%\build || goto error
cd %GITHUB_WORKSPACE%\build || goto error
cmake -G "%GENERATOR%" %EXTRA_ARGS% -DCMAKE_TOOLCHAIN_FILE=c:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=%OPENMAMA_INSTALL_DIR% -DGTEST_ROOT=C:/vcpkg/installed/%PLATFORM%-windows/ -DMAMA_ROOT=C:/vcpkg/installed/%PLATFORM%-windows/ .. || goto error
cmake --build . --config RelWithDebInfo --target install || goto error
ctest . -C RelWithDebInfo  -E MsgFieldVectorBoolTests.GetVectorBoolNullField --timeout 120 --output-on-failure || goto error

7z a openmama-omnm.%VCVER%.%PLATFORM%.zip "%OPENMAMA_INSTALL_DIR%" || goto error

goto end

:error
echo Failed with error #%errorlevel%.

:end
exit /b %errorlevel%
