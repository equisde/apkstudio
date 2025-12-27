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
    QString targetExt; // .dll, .smali, .so, .xml
};

class AIToolFactory {
public:
    static QList<AITool> getAllTools() {
        QList<AITool> tools;
        
        // --- CATEGORÍA: DLL & C# REVERSING (Unity/C#) ---
        tools << AITool{"dll_store_mod", "Unity C#", "AI Store Cracker", "Descompila Assembly-CSharp.dll y modifica precios.", "Busca métodos de compra y resta de saldo en C# y genera el código modificado.", ".dll"};
        tools << AITool{"dll_gacha_odds", "Unity C#", "Gacha Odds Master", "Analiza probabilidades de drop en código C#.", "Busca tablas de probabilidad y sugiere parches para forzar drops raros.", ".dll"};
        tools << AITool{"dll_anti_cheat", "Unity C#", "Unity AC Killer", "Detecta y anula sistemas de anti-cheat en C#.", "Busca lógicas de detección de trampas/memoria y genera el bypass.", ".dll"};
        tools << AITool{"dll_player_stats", "Unity C#", "Player Object Editor", "Modifica HP, Daño y Velocidad en C#.", "Busca la clase Player y modifica sus atributos base.", ".dll"};

        // --- CATEGORÍA: SECURITY HUB (IA Powered) ---
        tools << AITool{"sec_ssl_bypass", "Security", "AI Network Unpinner", "Inyecta bypass de certificados en la capa de red.", "Analiza lógicas de validación SSL y genera un parche para confiar en todos.", ".smali"};
        tools << AITool{"sec_root_cloak", "Security", "Root Cloak IA", "Oculta el estado de root a la aplicación.", "Busca binarios de root y propiedades de sistema para ofuscarlas.", ".smali"};
        tools << AITool{"sec_debug_kill", "Security", "Anti-Debug Bypass", "Anula detecciones de debugger/emulator.", "Identifica checks de isDebuggerConnected y los anula.", ".smali"};

        // --- CATEGORÍA: CLONING & BRANDING ---
        tools << AITool{"clone_pkg_rand", "Cloning", "Smart Package Renamer", "Cambia el ID de la app sin romper dependencias.", "Renombra el package name de forma recursiva en todo el proyecto.", ".xml"};
        tools << AITool{"clone_str_trans", "Cloning", "AI App Translator", "Traduce toda la app a cualquier idioma.", "Traduce strings.xml manteniendo las llaves originales.", ".xml"};

        // --- CATEGORÍA: IL2CPP & NATIVE ---
        tools << AITool{"native_offset_fix", "Il2Cpp", "RVA Offset Fixer", "Corrige direcciones de memoria para parcheo manual.", "Calcula el offset real de archivo basado en el VA del dump.", ".so"};
        tools << AITool{"auto_mod_report", "Automation", "Final Mod Report", "Resumen de todos los cambios.", "Genera un archivo MD con la documentación técnica de todo el modding realizado."};
        
        // --- CATEGORÍA: QUANTUM REVERSING (New) ---
        tools << AITool{"rev_elf_dissect", "Native", "AI ELF Dissector", "Analiza la estructura del binario nativo.", "Escanea la tabla de símbolos y busca funciones de seguridad ocultas."};
        tools << AITool{"rev_syscall_map", "Native", "Syscall Tracer IA", "Mapea llamadas al sistema.", "Identifica dónde el código nativo interactúa con el kernel (archivos, red)."};
        tools << AITool{"rev_csharp_logic", "Unity", "C# Logic Architect", "Analiza el flujo de datos en C#.", "Busca el 'GameManager' y explica el ciclo de vida del juego."};
        tools << AITool{"rev_proto_reconstruct", "Network", "Protobuf Rebuilder", "Reconstruye mensajes Protobuf.", "Analiza los buffers serializados y genera la definición .proto."};

        return tools;
    }
};

#endif
