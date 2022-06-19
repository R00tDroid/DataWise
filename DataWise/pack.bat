rmdir "%CD%\Packed" /s /q
mkdir "%CD%\Packed"

xcopy "%CD%\Template" "%CD%\Packed" /s /q /v /y
xcopy "%CD%\..\AnalyticsTestProject\Plugins" "%CD%\Packed" /s /q /v /y /exclude:excludedfiles.txt

7z -aoa a -tzip -r "%CD%\DataWise104.zip" "%CD%\Packed\*.*"
PAUSE
