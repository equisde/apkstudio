#ifndef AITOOLFACTORY_H
#define AITOOLFACTORY_H

#include <QString>
#include <QList>

struct AITool {
    QString id;
    QString category;
    QString name;
    QString description;
    QString aiPrompt;
};

class AIToolFactory {
public:
    static QList<AITool> getAllTools() {
        QList<AITool> tools;
        
        // --- CATEGORÍA 1: UNITY C# & DLL (10) ---
        tools << AITool{"dll_iap_bypass", "Unity C#", "IAP Cracker IA", "Genera código C# para saltar el pago de Google Play.", "Analiza métodos de compra y devuelve el código modificado para forzar 'true'."};
        tools << AITool{"dll_ads_kill", "Unity C#", "Ad-Killer Injector", "Elimina anuncios de UnityAds/AdMob.", "Localiza proveedores de anuncios y sugiere cómo anular los callbacks de carga."};
        tools << AITool{"dll_anti_cheat", "Unity C#", "Unity AC Killer", "Detecta y anula sistemas de detección de memoria.", "Busca lógicas de checksum o hash de memoria en el juego y genera el bypass."};
        tools << AITool{"dll_player_stats", "Unity C#", "Stats Master", "Modifica HP, Daño, Velocidad y Salto.", "Localiza la clase Player y sugiere modificaciones en los atributos de movimiento."};
        tools << AITool{"dll_inventory", "Unity C#", "Inventory Hacker", "Desbloquea todos los items del inventario.", "Busca lógicas de posesión de items y sugiere forzar la cantidad a 999."};
        tools << AITool{"dll_network", "Unity C#", "Network Decryptor", "Analiza protocolos de red propietarios.", "Busca lógicas de serialización y explica cómo se cifran los paquetes."};
        tools << AITool{"dll_gacha", "Unity C#", "Gacha Odds IA", "Analiza probabilidades de loot boxes.", "Escanea tablas de probabilidad y sugiere cómo forzar el drop raro."};
        tools << AITool{"dll_obfus_detect", "Unity C#", "De-obfuscator", "Limpia nombres de métodos ofuscados.", "Analiza el contexto del código y renombra funciones como 'a()' a nombres descriptivos."};
        tools << AITool{"dll_level_unlock", "Unity C#", "Level Architect", "Desbloquea todos los mundos y niveles.", "Identifica banderas de progreso y sugiere cómo setearlas al máximo."};
        tools << AITool{"dll_asset_logic", "Unity C#", "Asset Logic IA", "Analiza cómo se cargan los AssetBundles.", "Explica el flujo de descarga de recursos externos del juego."};

        // --- CATEGORÍA 2: NATIVE & IL2CPP (10) ---
        tools << AITool{"native_elf_dissect", "Native", "ELF Dissector IA", "Analiza símbolos y secciones ELF.", "Escanea la tabla de símbolos nativos en busca de funciones de protección."};
        tools << AITool{"native_frida_gen", "Native", "Frida Hook Gen", "Genera scripts de Frida automáticos.", "Crea un script .js para interceptar la función nativa seleccionada."};
        tools << AITool{"native_offset_map", "Native", "Offset Master", "Calcula RVA a Offset de archivo.", "Toma el VA del dump y da la dirección exacta para parchear el .so."};
        tools << AITool{"native_syscall", "Native", "Syscall Tracer IA", "Analiza interacciones con el kernel.", "Identifica dónde el binario lee archivos o abre conexiones de red."};
        tools << AITool{"native_anti_debug", "Native", "Anti-Debug Bypass", "Anula ptrace y chequeos de debugger.", "Busca llamadas a ptrace o isDebuggerConnected nativos y ofrece el parche."};
        tools << AITool{"native_integrity", "Native", "Integrity Killer", "Anula chequeos de hash del APK.", "Identifica dónde el código nativo valida la firma del APK y sugiere el bypass."};
        tools << AITool{"native_crypto", "Native", "Native Crypto IA", "Busca llaves AES/XOR ocultas.", "Analiza el segmento de datos del binario en busca de patrones de llaves criptográficas."};
        tools << AITool{"native_modmenu", "Native", "ModMenu C++ Gen", "Genera el código C++ del menú flotante.", "Crea el source modmenu.cpp basado en los offsets encontrados."};
        tools << AITool{"native_vtable", "Native", "VTable Explorer", "Analiza tablas de métodos virtuales.", "Explica la jerarquía de clases nativas de C++ en el binario."};
        tools << AITool{"native_root_cloak", "Native", "Root Cloak IA", "Oculta el root a nivel nativo.", "Busca binarios como 'su' o 'magisk' en el código nativo y genera el bypass."};

        // --- CATEGORÍA 3: JAVA & SECURITY (10) ---
        tools << AITool{"sec_ssl_unpin", "Security", "SSL Unpinner Pro", "Elimina el pinning de certificados.", "Analiza TrustManagers personalizados y genera un parche Smali universal."};
        tools << AITool{"sec_cert_bypass", "Security", "AI Cert Bypass", "Ignora validaciones de certificado.", "Modifica el checkServerTrusted para que no lance excepciones nunca."};
        tools << AITool{"sec_proxy_detect", "Security", "VPN/Proxy Detector", "Anula detección de proxies.", "Busca lógicas de System.getProperty('http.proxyHost') y sugiere el bypass."};
        tools << AITool{"sec_emulator", "Security", "Emulator Cloaker", "Oculta el emulador a la app.", "Identifica checks de hardware (Build.MODEL, Build.PRODUCT) y los ofusca."};
        tools << AITool{"sec_tamper", "Security", "Anti-Tamper IA", "Busca lógicas de protección de archivos.", "Detecta si la app chequea sus propios archivos y anula la validación."};
        tools << AITool{"sec_api_map", "Security", "API Usage Analyzer", "Explica qué permisos usa la app.", "Analiza llamadas a la API de Android y deduce el comportamiento de la app."};
        tools << AITool{"sec_log_cleanup", "Security", "Log Stripper IA", "Elimina logs de depuración.", "Busca llamadas a Log.d/v/i y las comenta automáticamente para mayor sigilo."};
        tools << AITool{"sec_overlay", "Security", "Overlay Defender", "Busca protecciones contra overlays.", "Detecta si la app impide ventanas flotantes y sugiere el bypass."};
        tools << AITool{"sec_safety_net", "Security", "SafetyNet Bypass", "Analiza lógicas de integridad de Google.", "Busca integraciones de Play Integrity y sugiere cómo responder siempre exitoso."};
        tools << AITool{"sec_perm_escalate", "Security", "Privilege Analyzer", "Busca vectores de escalada.", "Analiza el AndroidManifest.xml en busca de intents o servicios expuestos."};

        // --- CATEGORÍA 4: AUTOMATION & CLONING (10) ---
        tools << AITool{"auto_pkg_rename", "Automation", "Recursive Renamer", "Cambia el ID de la app en todo el proyecto.", "Renombra el package name de forma segura sin romper librerías nativas."};
        tools << AITool{"auto_brand_gen", "Automation", "AI Rebranding", "Cambia nombres e iconos por IA.", "Genera nuevos strings y sugiere paletas de colores para la app."};
        tools << AITool{"auto_translation", "Automation", "AI Multi-Lang", "Traduce la app a 20+ idiomas.", "Traduce strings.xml manteniendo el formato XML intacto."};
        tools << AITool{"auto_smali_opt", "Automation", "Smali Optimizer", "Limpia y optimiza el código Smali.", "Elimina código muerto y optimiza registros en archivos .smali."};
        tools << AITool{"auto_final_report", "Automation", "Full Mod Report", "Documentación técnica final.", "Genera un reporte profesional en Markdown de todos los cambios realizados."};
        tools << AITool{"auto_manifest_fix", "Automation", "Manifest Repair IA", "Corrige errores en el manifest tras la clonación.", "Busca conflictos de providers y llaves de API en el manifest y sugiere la corrección."};
        tools << AITool{"auto_resource_opt", "Automation", "Resource Optimizer", "Optimiza y reduce el tamaño de los recursos.", "Analiza carpetas drawable y res en busca de archivos redundantes o no usados."};
        tools << AITool{"auto_lib_patch", "Automation", "Library Auto-Patch", "Parchea librerías comunes (OkHttp, Retrofit).", "Busca lógicas de interceptores en librerías de red y genera bypasses estándar."};
        tools << AITool{"auto_signature_kill", "Automation", "Global Sig-Killer", "Anula chequeos de firma en todo el código.", "Realiza un escaneo masivo de lógicas de comparación de firmas y genera parches smali."};
        tools << AITool{"auto_firebase_map", "Automation", "Firebase Hijacker", "Identifica configuraciones de Firebase.", "Escanea google-services.json y strings.xml para mapear la infraestructura de Firebase."};

        // --- CATEGORÍA 5: DEEP ANALYSIS & UTILITY (10) ---
        tools << AITool{"deep_asset_mapper", "Deep Analysis", "Universal Asset Mapper", "Categoriza todos los assets del juego.", "Escanea carpetas assets/ y categoriza por tipo: Texturas, Modelos, Scripts, Sonidos."};
        tools << AITool{"deep_string_xref", "Deep Analysis", "String X-Ref IA", "Cruza strings con su ubicación en el código.", "Busca el uso de strings críticos (llaves, URLs) y apunta al método Smali/C# exacto."};
        tools << AITool{"deep_api_xref", "Deep Analysis", "API X-Ref Pro", "Cruza llamadas al sistema con la lógica de la app.", "Analiza el uso de APIs de Android (Telephony, SMS, GPS) y explica su propósito."};
        tools << AITool{"deep_crypto_hunter", "Deep Analysis", "Crypto Engine Hunter", "Localiza motores de cifrado propietarios.", "Busca implementaciones personalizadas de algoritmos de cifrado en código nativo."};
        tools << AITool{"deep_intent_mapper", "Deep Analysis", "Intent Flow Mapper", "Mapea todos los intents internos y externos.", "Analiza el flujo de comunicación entre actividades y servicios."};
        tools << AITool{"deep_broadcast_map", "Deep Analysis", "Broadcast Watcher", "Identifica receptores de broadcast y sus acciones.", "Explica qué eventos del sistema escucha la app y cómo responde."};
        tools << AITool{"deep_service_scan", "Deep Analysis", "Service Dissector", "Analiza todos los servicios en segundo plano.", "Explica la función de cada servicio y busca comportamientos de persistencia."};
        tools << AITool{"deep_ui_reconstruct", "Deep Analysis", "UI Reconstructor", "Reconstruye la UI desde XML y código.", "Analiza layouts y lógica de vistas para explicar cómo se arma la interfaz."};
        tools << AITool{"deep_network_flow", "Deep Analysis", "Network Request Flow", "Mapea el flujo de peticiones de red.", "Ordena cronológicamente las llamadas de red desde el inicio de la app."};
        tools << AITool{"deep_persistence", "Deep Analysis", "Persistence Map", "Identifica dónde la app guarda sus datos.", "Mapea el uso de bases de datos SQLite, SharedPreferences y almacenamiento externo."};

        return tools;
    }
};

#endif
