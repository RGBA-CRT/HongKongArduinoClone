REM ffmpeg -i se\SE_OK_16bit.wav  -f wav -acodec adpcm_ms se\SE_OK.adpcm.wav
copy se\SE_OK_16bit.wav se\SE_OK.adpcm.wav
ResourceHacker.exe -open resource_bin.res -save tmp.res -action addoverwrite -res se\SE_OK.adpcm.wav -mask WAVE,SE_OK,
ResourceHacker.exe -open HongKongArduinoClone.exe -save HongKongArduinoClone.exe -action addoverwrite -res tmp.res
del tmp.res
del se\SE_OK.adpcm.wav
upx HongKongArduinoClone.exe