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
        
        // --- CATEGORÍA: ECONOMY & STORES (10 herramientas) ---
        tools << AITool{"eco_iap_kill", "Economy", "IAP Global Bypass", "Anula validación de Google Play Billing.", "Localiza 'isPurchased' o 'getPurchaseState' y genera un parche para retornar true."};
        tools << AITool{"eco_money_inf", "Economy", "Infinite Currency", "Busca setters de oro, gemas o monedas.", "Busca métodos 'AddCurrency' o campos '_money' y sugiere el parche para valor máximo."};
        tools << AITool{"eco_shop_free", "Economy", "Free in-game Store", "Pone los precios de la tienda en 0.", "Analiza clases 'StoreItem' o 'Price' y cambia la lógica de resta de saldo."};
        tools << AITool{"eco_energy_god", "Economy", "God Energy System", "Energía infinita sin esperas.", "Busca 'ConsumeEnergy' o 'm_CurrentEnergy' y anula el decremento."};
        tools << AITool{"eco_ads_kill", "Economy", "AI Ad-Blocker", "Elimina anuncios por código.", "Busca 'ShowInterstitial' o 'AdManager' y devuelve false en los checks."};
        tools << AITool{"eco_reward_mult", "Economy", "Reward Multiplier", "Multiplica recompensas x100.", "Busca 'CalculateReward' y multiplica el retorno por una constante masiva."};
        tools << AITool{"eco_level_unlock", "Economy", "Instant Level Unlock", "Desbloquea todos los niveles.", "Busca 'IsLevelLocked' y haz que siempre retorne false."};
        tools << AITool{"eco_skin_unlock", "Economy", "All Skins Unlocked", "Desbloquea cosméticos.", "Busca 'HasSkin' o 'IsItemOwned' y fuerza el retorno a true."};
        tools << AITool{"eco_vip_god", "Economy", "VIP / Premium IA", "Activa el estado VIP.", "Identifica 'IsVIPUser' o 'GetAccountType' y fuerza el rango máximo."};
        tools << AITool{"eco_battlepass", "Economy", "BattlePass Architect", "Hackea el pase de batalla.", "Busca 'GetBattlePassExp' o 'IsPremiumPass' y genera el bypass."};

        // --- CATEGORÍA: PLAYER & COMBAT (10 herramientas) ---
        tools << AITool{"ply_god_mode", "Combat", "God Mode (HP)", "Invulnerabilidad total.", "Localiza 'TakeDamage' o 'm_Health' y haz que el daño recibido sea 0."};
        tools << AITool{"ply_one_hit", "Combat", "One-Hit Kill", "Daño masivo a enemigos.", "Busca 'GetAttackPower' y multiplica el resultado por 999999."};
        tools << AITool{"ply_speed_hack", "Combat", "AI Speed Engine", "Velocidad de movimiento x2.", "Busca 'moveSpeed', 'walkSpeed' o 'velocity' y genera un offset de incremento."};
        tools << AITool{"ply_no_recoil", "Combat", "Zero Recoil", "Elimina el retroceso de armas.", "Busca 'RecoilAmount' o 'ShakeCamera' y anula la función."};
        tools << AITool{"ply_inf_ammo", "Combat", "Infinite Ammo", "Munición que nunca baja.", "Busca 'ConsumeAmmo' o 'DecrementClip' y 'nopea' la resta."};
        tools << AITool{"ply_range_ext", "Combat", "Range Extender", "Aumenta el rango de ataque.", "Busca variables de 'AttackRange' o 'Distance' y duplica su valor."};
        tools << AITool{"ply_wall_hack", "Combat", "ESP / WallHack IA", "Detecta entidades tras muros.", "Busca 'IsVisible' o lógicas de renderizado Raycast y sugiere modificaciones."};
        tools << AITool{"ply_aim_assist", "Combat", "AimBot Helper", "Mejora la precisión de disparo.", "Analiza 'CalculateAim' o 'TargetAcquisition' y reduce el spread a cero."};
        tools << AITool{"ply_skill_cd", "Combat", "No Skill Cooldown", "Habilidades sin tiempo de espera.", "Busca 'GetSkillCooldown' y haz que retorne un float de 0.0."};
        tools << AITool{"ply_fly_hack", "Combat", "Fly/Gravity Hack", "Permite volar o saltar infinito.", "Busca 'GravityScale' o 'JumpForce' y sugiere valores para flotar."};

        // --- CATEGORÍA: IL2CPP REVERSING (10 herramientas) ---
        tools << AITool{"rev_offset_map", "Reversing", "Offset Mapper Pro", "Calcula RVA a Offset real.", "Toma la dirección del dump y el VA del binario para dar el offset de parcheo exacto."};
        tools << AITool{"rev_class_dissect", "Reversing", "Class Dissector", "Analiza variables del Jugador.", "Extrae todos los campos de la clase Player y deduce cuáles son las stats locales."};
        tools << AITool{"rev_anti_cheat", "Reversing", "Anti-Cheat Killer", "Anula SafetyNet/Integrity.", "Busca 'IsRooted', 'isEmulator' o 'checkIntegrity' y genera el bypass."};
        tools << AITool{"rev_string_pool", "Reversing", "String Finder", "Busca strings sensibles.", "Escanea el string pool en busca de llaves de API o URLs de servidores."};
        tools << AITool{"rev_meta_analyser", "Reversing", "Metadata Analyzer", "Busca debilidades en metadata.", "Analiza los símbolos de global-metadata.dat para encontrar funciones de servidor."};
        tools << AITool{"rev_network_sniff", "Reversing", "Network Logic IA", "Analiza comunicaciones de red.", "Busca 'SendRequest' o 'PacketEncrypt' y sugiere cómo interceptar datos."};
        tools << AITool{"rev_unity_events", "Reversing", "Event Hook Mapper", "Detecta triggers de Unity.", "Busca 'OnTriggerEnter' o 'OnCollision' y sugiere hooks para interceptarlos."};
        tools << AITool{"rev_encryption", "Reversing", "Crypto Decoder", "Busca lógicas de cifrado.", "Busca 'AES', 'Base64' o 'XOR' en el código smali/cs y ofrece decodificación."};
        tools << AITool{"rev_lib_scanner", "Reversing", "Native Lib Scanner", "Busca otras .so interesantes.", "Analiza el uso de libmain.so o libgame.so y su interacción con il2cpp."};
        tools << AITool{"rev_signature", "Reversing", "Signature Bypass Pro", "Anula chequeos de firma.", "Identifica lógicas de PackageManager.getSignature y genera el parche de bypass."};

        // --- CATEGORÍA: AUTOMATION & MOD MENU (5 herramientas) ---
        tools << AITool{"auto_modmenu_cpp", "Automation", "C++ ModMenu Gen", "Genera código para menú nativo.", "Crea un archivo modmenu.cpp profesional con toggles para los offsets encontrados."};
        tools << AITool{"auto_java_ui", "Automation", "Java UI Architect", "Crea UI flotante para Android.", "Genera una clase FloatingWindowService con switches funcionales."};
        tools << AITool{"auto_smali_patcher", "Automation", "Smali Auto-Patch", "Aplica parches smali automáticos.", "Toma los hallazgos de IA y genera el archivo .smali corregido listo para build."};
        tools << AITool{"auto_binary_patch", "Automation", "Binary Patcher IA", "Genera parches hexadecimales.", "Crea el JSON de parches binarios para aplicar a libil2cpp.so directamente."};
        tools << AITool{"auto_mod_report", "Automation", "Final Mod Report", "Resumen de todos los cambios.", "Genera un archivo MD con la documentación técnica de todo el modding realizado."};
        
        return tools;
    }
};

#endif // AITOOLFACTORY_H