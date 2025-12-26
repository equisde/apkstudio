# Game Modding Analysis Strategy
Este archivo define el protocolo de análisis para juegos detectados por APK Studio.

## Motor Detectado: Unity (IL2CPP/Mono)
- **Vectores de Ataque**:
  - Dump de metadatos IL2CPP.
  - Modificación de archivos `.assets` y `globalgamemanagers`.
  - Parches en librerías nativas (`libil2cpp.so`).
- **IA Mod Suggestions**: 
  - Analizar llamadas a `checkLicense` y `isPremium`.
  - Buscar multiplicadores de monedas en constantes `0x`.

## Motor Detectado: Unreal Engine
- **Vectores de Ataque**:
  - Extracción de archivos `.pak`.
  - Modificación de Blueprints.
  - Ajustes en `DefaultEngine.ini`.
