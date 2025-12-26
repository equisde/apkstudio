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
        
        // --- ECONOMY & STORES (10) ---
        tools << AITool{"eco_iap_kill", "Economy", "IAP Global Bypass", "Anula validación de Google Play Billing.", "Localiza 'isPurchased' o 'get_IsUnlocked' y fuerza el retorno a true."};
        tools << AITool{"eco_money_inf", "Economy", "Infinite Currency", "Busca setters de oro, gemas o monedas.", "Busca métodos 'AddCurrency' o campos '_money' y sugiere el parche para valor máximo."};
        tools << AITool{"eco_shop_free", "Economy", "Free in-game Store", "Pone los precios de la tienda en 0.", "Analiza clases 'StoreItem' o 'Price' y cambia la lógica de resta de saldo."};
        tools << AITool{"eco_energy_god", "Economy", "God Energy System", "Energía infinita sin esperas.", "Busca 'ConsumeEnergy' o 'm_CurrentEnergy' y anula el decremento."};
        tools << AITool{"eco_ads_kill", "Economy", "AI Ad-Blocker", "Elimina anuncios por código.", "Busca 'ShowInterstitial' o 'AdManager' y devuelve false en los checks."};
        tools << AITool{"eco_reward_mult", "Economy", "Reward Multiplier", "Multiplica recompensas x100.", "Busca 'CalculateReward' y multiplica el retorno por una constante masiva."};
        tools << AITool{"eco_level_unlock", "Economy", "Instant Level Unlock", "Desbloquea todos los niveles.", "Busca 'IsLevelLocked' y haz que siempre retorne false."};
        
        // --- PLAYER & COMBAT (10) ---
        tools << AITool{"ply_god_mode", "Combat", "God Mode (HP)", "Invulnerabilidad total.", "Localiza 'TakeDamage' o 'm_Health' y haz que el daño recibido sea 0."};
        tools << AITool{"ply_one_hit", "Combat", "One-Hit Kill", "Daño masivo a enemigos.", "Busca 'GetAttackPower' y multiplica el resultado por 999999."};
        tools << AITool{"ply_speed_hack", "Combat", "AI Speed Engine", "Velocidad de movimiento x2.", "Busca 'moveSpeed', 'walkSpeed' o 'velocity' y genera un offset de incremento."};
        tools << AITool{"ply_no_recoil", "Combat", "Zero Recoil", "Elimina el retroceso de armas.", "Busca 'RecoilAmount' o 'ShakeCamera' y anula la función."};
        tools << AITool{"ply_inf_ammo", "Combat", "Infinite Ammo", "Munición que nunca baja.", "Busca 'ConsumeAmmo' o 'DecrementClip' y 'nopea' la resta."};
        tools << AITool{"ply_range_ext", "Combat", "Range Extender", "Aumenta el rango de ataque.", "Busca variables de 'AttackRange' o 'Distance' y duplica su valor."};

        // --- IL2CPP REVERSING (10) ---
        tools << AITool{"rev_offset_map", "Reversing", "Offset Mapper Pro", "Calcula RVA a Offset real.", "Toma la dirección del dump y el VA del binario para dar el offset de parcheo exacto."};
        tools << AITool{"rev_class_dissect", "Reversing", "Class Dissector", "Analiza variables del Jugador.", "Extrae todos los campos de la clase Player y deduce cuáles son las stats locales."};
        tools << AITool{"rev_anti_cheat", "Reversing", "Anti-Cheat Killer", "Anula SafetyNet/Integrity.", "Busca 'IsRooted', 'isEmulator' o 'checkIntegrity' y genera el bypass."};
        tools << AITool{"rev_string_pool", "Reversing", "String Finder", "Busca strings sensibles.", "Escanea el string pool en busca de llaves de API o URLs de servidores."};

        // --- AUTOMATION & MOD MENU (5) ---
        tools << AITool{"auto_modmenu_cpp", "Automation", "C++ ModMenu Gen", "Genera código para menú nativo.", "Crea un archivo modmenu.cpp profesional con toggles para los offsets encontrados."};
        tools << AITool{"auto_java_ui", "Automation", "Java UI Architect", "Crea UI flotante para Android.", "Genera una clase FloatingWindowService con switches funcionales."};
        
        return tools;
    }
};

#endif // AITOOLFACTORY_H
