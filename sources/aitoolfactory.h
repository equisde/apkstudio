#ifndef AITOOLFACTORY_H
#define AITOOLFACTORY_H

#include <QString>
#include <QStringList>
#include <QMap>

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
        
        // --- CATEGORÍA: SEGURIDAD DE RED ---
        tools << AITool{"net_cert_bypass", "Security: Network", "AI Cert Bypass", "Genera parches smali para ignorar validación de certificados.", "Analiza este código smali y genera un parche para que checkServerTrusted siempre retorne void."};
        tools << AITool{"net_proxy_detect", "Security: Network", "Proxy Detector", "Busca lógica de detección de proxies y VPNs.", "Busca patrones de System.getProperty('http.proxyHost') y sugiere bypass."};
        
        // --- CATEGORÍA: ANTI-TAMPERING ---
        tools << AITool{"tamp_sig_check", "Security: Tampering", "Sig-Check Killer", "Localiza y anula la verificación de firma del APK.", "Identifica el método que compara el Signature[] y haz que siempre devuelva true."};
        tools << AITool{"tamp_root_bypass", "Security: Tampering", "Root Cloak IA", "Inyecta código para ocultar el root a la app.", "Busca archivos como /system/app/Superuser.apk en el código y ofusca la búsqueda."};
        
        // --- CATEGORÍA: GAME MODDING ---
        tools << AITool{"game_unity_speed", "Game Modding", "Unity SpeedHack", "Inyecta un multiplicador de Time.timeScale.", "Busca la clase UnityEngine.Time y genera un hook para modificar timeScale."};
        tools << AITool{"game_val_search", "Game Modding", "AI Value Hunter", "Encuentra offsets de monedas y diamantes en archivos metadata.", "Analiza los símbolos de este dump y busca patrones de 'Coin', 'Gem', 'Gold'."};
        
        // --- CATEGORÍA: APK CLONING ---
        tools << AITool{"clone_pkg_rename", "Cloning", "Stellar Renamer", "Renombra el package name de forma recursiva e inteligente.", "Cambia todos los strings de package name evitando romper librerías nativas."};
        
        // ... (Hasta completar las 30 herramientas)
        return tools;
    }
};

#endif // AITOOLFACTORY_H
