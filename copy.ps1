$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk
& adb push libs/arm64-v8a/libdumper.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libdumper.so
& adb push include/libs/libbeatsaber-hook_2019_2_1f1_0_1_1.so /sdcard/Android/data/com.beatgames.beatsaber/files/libs/libbeatsaber-hook_2019_2_1f1_0_1_1.so
& adb shell am force-stop com.beatgames.beatsaber
