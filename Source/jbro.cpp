#include <jni.h>
#include <android/log.h>
#include <Substrate/CydiaSubstrate.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <bits/sysconf.h>
#include <sys/mman.h>
#include "Quaternion.hpp"
#include "Unity.h"
#include "Color.hpp"
#include "Vector2.hpp"
#include "Rect.hpp"

#define LOG_TAG "JbroMain"

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)


uintptr_t getLibraryBase(const char *libName) {
    uintptr_t retAddr = 0;

    char fileName[255];
    memset(fileName, 0, sizeof(fileName));

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    snprintf(fileName, sizeof(fileName), "/proc/%d/maps", getpid());
    FILE *fp = fopen(fileName, "rt");
    if (fp != NULL) {
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strstr(buffer, libName) != NULL) {
                retAddr = (uintptr_t) strtoul(buffer, NULL, 16);
                break;
            }
        }
        fclose(fp);
    }
    return retAddr;
}

uintptr_t getAbsoluteAddress(const char *libName, uintptr_t relativeAddr) {
    uintptr_t base = getLibraryBase(libName);
    if (base == 0)
        return 0;
    return (base + relativeAddr);
}

uintptr_t location;

/*
__attribute__((constructor))void initializer() {
    pthread_t ptid;
    pthread_create(&ptid, NULL, started, NULL);
}
*/

uintptr_t base(const char *soname)
{
    void *imagehandle = dlopen(soname, RTLD_LOCAL | RTLD_LAZY);
    if (soname == NULL)
        return NULL;
    if (imagehandle == NULL){
        return NULL;
    }
    uintptr_t * irc = NULL;
    FILE *f = NULL;
    char line[200] = {0};
    char *state = NULL;
    char *tok = NULL;
    char * baseAddr = NULL;
    if ((f = fopen("/proc/self/maps", "r")) == NULL)
        return NULL;
    while (fgets(line, 199, f) != NULL)
    {
        tok = strtok_r(line, "-", &state);
        baseAddr = tok;
        tok = strtok_r(NULL, "\t ", &state);
        tok = strtok_r(NULL, "\t ", &state); // "r-xp" field
        tok = strtok_r(NULL, "\t ", &state); // "0000000" field
        tok = strtok_r(NULL, "\t ", &state); // "01:02" field
        tok = strtok_r(NULL, "\t ", &state); // "133224" field
        tok = strtok_r(NULL, "\t ", &state); // path field

        if (tok != NULL) {
            int i;
            for (i = (int)strlen(tok)-1; i >= 0; --i) {
                if (!(tok[i] == ' ' || tok[i] == '\r' || tok[i] == '\n' || tok[i] == '\t'))
                    break;
                tok[i] = 0;
            }
            {
                size_t toklen = strlen(tok);
                size_t solen = strlen(soname);
                if (toklen > 0) {
                    if (toklen >= solen && strcmp(tok + (toklen - solen), soname) == 0) {
                        fclose(f);
                        return (uintptr_t)strtoll(baseAddr,NULL,16);
                    }
                }
            }
        }
    }
    fclose(f);
    return NULL;
}

uintptr_t getRealOffset(uintptr_t offset)
{
    if (location <= 0)
    {
        location = getLibraryBase("libil2cpp.so");
        /*
        location = base("/data/app/com.pixel.gun3d-1/lib/arm/libil2cpp.so");
        if (location == 0)
        {
            location = base("/data/app-lib/com.pixel.gun3d-1/libil2cpp.so");
        }
        */
    }
    //LOGD("Offset #1: %i", offset);
    //LOGD("Offset #2: %i", bkase);
    return location + offset;
}


void *(*Component_GetTransform)(void* component) = (void *(*)(void* ))getRealOffset(0x3015A24); // Component$$get_transform
void *(*Component_GetGameObject)(void* component) = (void *(*)(void* ))getRealOffset(0x3015AB4); // Component$$get_gameObject

Vector3 (*Transform_get_position)(void* transform) = (Vector3 (*)(void *))getRealOffset(0x321D614); // Transform$$get_position
void (*Transform_Set_Position)(void* transform, Vector3 pos) = (void (*)(void*, Vector3))getRealOffset(0x321D6DC); // Transform$$set_position
void (*Transform_Get_parent)(void* transform) = (void (*)(void*))getRealOffset(0x321E6B4); // Transform$$get_parent
//void (*Transform_Set_Rotation)(void* transform, Quaternion pos) = (void (*)(void*, Quaternion))getRealOffset(0x2159EC0);
//Quaternion (*Transform_Get_Rotation)(void* transform) = (Quaternion (*)(void*))getRealOffset(0x2159E30);
void (*Transform_LookAt)(void* transform, Vector3 pos) = (void (*)(void*, Vector3))getRealOffset(0x321F548); // public void LookAt(Vector3 worldPosition)
void (*Transform_Rotate)(void* transform, Vector3 pos) = (void (*)(void*, Vector3))getRealOffset(0x321EFA8); // public void Rotate(Vector3 eulers)
void (*Transform_set_localScale)(void* transform, Vector3 pos) = (void (*)(void*, Vector3))getRealOffset(0x321E614); // Transform$$set_localScale
Vector3 (*Transform_get_localScale)(void* transform) = (Vector3 (*)(void *))getRealOffset(0x321E54C); // Transform$$get_localScale

void (*GameObject_set_active)(void* obj, bool) = (void (*)(void* obj, bool))getRealOffset(0x2FE12A4); // GameObject$$set_active
//void *(*PhotonNetwork_InstantiateSceneObject)(void* _this,void* prefabName, Vector3 position, Quaternion rotation, int group, void*) = (void *(*)(void*, void*, Vector3, Quaternion, int, void*))getRealOffset(0x947E7C); // PhotonNetwork$$InstantiateSceneObject

void (*LoadScene)(void*, void* sceneName) = (void (*)(void*, void* sceneName))getRealOffset(0x38DABAC); // SceneManager$$LoadScene

void (*Debug_log)(void*, void* message) = (void (*)(void*, void*))getRealOffset(0x300867C); // Debug$$Log

void* (*get_WeaponManager)(void *me) = (void* (*)(void *))getRealOffset(0xEF3970); // Player_move_c$$get__weaponManager

bool shotgun;
bool price;
bool ninja;
bool xray;
bool levell;
bool aimbot;
bool telekilll;
bool damageall;
bool slowdownall;
bool isp;
bool spam;
bool spam2;
bool me_scale_plus;
bool me_scale_minus;
bool noclipp;
bool firerate;
bool god;
bool shott;
bool ammmo;
bool gui;
bool mass;
bool cheat;
bool scenecheat;
bool scenedev;
bool setloc;
bool getloc;
bool dmg;
bool playerid;
bool explodeshot;
bool getscale;
bool setscale;
bool NOTfirstscale;
bool spawngem;

const char *txt1;
const char *txt2;

bool toggle1;
bool toggle2;
bool toggle3;
bool toggle4;
bool toggle5;
bool toggle6;
bool toggle7;
bool toggle8;
bool toggle9;
bool toggle10;
bool toggle11;
bool toggle12;
bool toggle13;
bool toggle14;
bool toggle15;
bool toggle16;
bool toggle17;
bool toggle18;
bool toggle19;
bool toggle20;
bool toggle21;
bool toggle22;
bool toggle23;
bool toggle24;
bool toggle25;
bool toggle26;
bool toggle27;
bool toggle28;
bool toggle29;
bool toggle30;
bool toggle31;
bool toggle32;
bool toggle33;
bool toggle34;
bool toggle35;
bool toggle36;
bool toggle37;
bool toggle38;
bool toggle39;
bool toggle40;

float PlayerX;
float PlayerY;
float PlayerZ;

Vector3 PlayerScale;
Vector3 PlayerScaleMod;

Vector3 playerPos;

int curramount;
bool addcoinsclicked;
bool addgemsclicked;
bool addbpclicked;
bool addeventclicked;
bool addcraftclicked;
bool addcouponclicked;

std::string object;

void* il2cpp_string_new(const char* str)
{
    static void* (*il2cpp_string_new_pointer)(const char* str) =
            (void* (*)(const char* str))getRealOffset(0x6A095C); // il2cpp_string_new 0x2518910
    return il2cpp_string_new_pointer(str);
}

template <typename T> std::string tostr(const T& t) {
    std::ostringstream os;
    os<<t;
    return os.str();
}


void Crash()
{
    raise(SIGSEGV);
}

bool isEnemy(void* me, void* it)
{
    bool (*isEnemy)(void *me, void *it) =
            (bool(*)(void *, void *))getRealOffset(0x16D0B5C); // PlayerDamageable$$IsEnemyTo
    return isEnemy(me, it);
}
bool isDead(void* it)
{
    bool (*IsDead)(void *me) =
            (bool(*)(void *))getRealOffset(0x16D0DD0); // PlayerDamageable$$IsDead
    return IsDead(it);
}

monoList<void **>* get_PlayerList()
{
    monoList<void **> *playersList;//  = *(monoList<void **> **)((uint64_t)character + 0x8);
    //LOGD("1");
    void *off = *(void **) (getRealOffset(0x434C14C)); // Class$Initializer
    //LOGD("2");
    LOGD("Player List check");
    if (off) {
        //LOGD("5");
        void *off2 = *(void **) ((uint32_t) off + 0x5C);
        //LOGD("6");
        if (off2) {
            //LOGD("7");
            playersList = *(monoList<void **> **) ((uint32_t) off2 + 0x30);
            LOGD("Player List Good #1");
            return playersList;

        }
        else
        {
            LOGD("Player List Bad #2");
        }
    }
    else
    {
        LOGD("Player List Bad #1");
    }
}

void* get_storagerStatic()
{
    void *var = *(void **) getRealOffset(0x4D1D704); // Class$Storager
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0x0); // 0x4C
        return var1;
    }
}

void Storager_set_string(void* key, void* val)
{
    void* storager = get_storagerStatic();
    void (*Storager_set_stringggg)(void* _this, void*, void*) =
            (void (*)(void*, void*, void*))getRealOffset(0x1900DB8/*0x12F5330*/); // Storager$$setString
    Storager_set_stringggg(storager, key, val);
}

void Storager_set_string(const char * key, const char * val)
{
    Storager_set_string(il2cpp_string_new(key), il2cpp_string_new(val));
}

void* get_debugStatic()
{
    void *var = *(void **) getRealOffset(0x434A6D8); // Class$UnityEngine.Debug
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0x0); // 0x4C
        return var1;
    }
}

void Debug_Log(const char* txt)
{
    void* a = get_debugStatic();
    Debug_log(a, il2cpp_string_new(txt));
}

void Debug_Log(void* txt)
{
    void* a = get_debugStatic();
    Debug_log(a, txt);
}

void* get_weaponManagerStatic()
{
    void *var = *(void **) getRealOffset(0x434C070); // Class$WeaponManager
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0x50);
        return var1;
    }
}

void* sharedWeaponManager()
{
    void *var = get_weaponManagerStatic();
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0x10C);
        return var1;
    }
}

void* get_MyPlayer()
{
    void *var = sharedWeaponManager();
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0x24);
        return var1;
    }
}

std::string formatter(const std::string& s) {
    if (!s.size()) {
        return "";
    }
    std::stringstream ss;
    ss << s[0];
    for (int i = 1; i < s.size(); i++) {
        if (i%2==0) ss << "\\x";
        ss << s[i];
    }
    return  "\\x" + ss.str();
}


void* get_PlayerTransform(void* ply)
{
    void *var = ply;
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0x1B0);
        return var1;
    }
}


Vector3 get_PlayerPosition()
{
    void *transform = get_PlayerTransform(get_MyPlayer());
    if (transform)
    {
        return Transform_get_position(Component_GetTransform(transform));
    }
}

void* get_PlayerSkinName(void* ply)
{
    void *var = ply;
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0x320);
        return var1;
    }
}

void* get_SkinNameFirstPersonControlSharp(void* ply)
{
    void *var = ply;
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0xD8);
        return var1;
    }
}

void* get_FirstPersonControlSharpCharacterController(void* ply)
{
    void *var = ply;
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0xD8);
        return var1;
    }
}

void* get_PlayerDamageable(void* ply)
{
    void *var = ply;
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0x30C);
        return var1;
    }
}

void* get_bonusControllerStatic()
{
    void *var = *(void **) getRealOffset(0x434CF5C); // Class$BonusController
    LOGD("var: %p", var);
    if (var)
    {
        void* var1 = *(void **)((uint32_t)var + 0x0);
        LOGD("var: %p", var1);
        if (var1);
        {
            return var1;
        }
    }
}

void* get_bankControllerStatic(void *_this)
{
    void* (*inst)(void *) = (void* (*)(void *))getRealOffset(0x196F980); // BankController$$get_Instance Class$BankController
    return inst(_this);
}

monoList<void **>* get_WeaponList(void* _this) {
    monoList<void **> *weaponsList;//  = *(monoList<void **> **)((uint64_t)character + 0x8);
    //LOGD("1");

    monoList<void **>* (*playerWeapons)(void *me) =
            *(monoList<void **>* (*)(void *))getRealOffset(0xFEAEAC); // WeaponManager$$get_playerWeapons List`1<WeaponW> get_playerWeapons()

    weaponsList = playerWeapons(get_WeaponManager(_this));

    return  weaponsList;
}

void DoXray(void *_this)
{
//    if (xray)
    {
        void* (*ActivateXRay)(void *, bool) =
        (void* (*)(void *, bool))getRealOffset(0xF0EE00); // Player_move_c$$ActivateXRay
        if (xray)
        {
            ActivateXRay(get_MyPlayer(), true);
        }
        else if (!xray)
        {
            ActivateXRay(get_MyPlayer(), false);
        }
    }
}

void AimBot(void *character)
{
    //LOGD("1");
    if (aimbot)
    {
        //LOGD("1");
        {
            void* transform = get_PlayerTransform(get_MyPlayer());
            //void* cam = get_myCamera();

            monoList<void **> *playersList = get_PlayerList();
            //LOGD("3");
            //LOGD("4");
            for (int i = 0; i < playersList->getSize(); i++)
            {
                //LOGD("5");
                //void *player = players[i];
                void *player = playersList->getItems()[i];
                void* damagable = get_PlayerDamageable(player);
                void *themtransform = get_PlayerTransform(player);

                Vector3 them;

                Vector3 myPos = Transform_get_position(Component_GetTransform(transform));

                float distance = 1000;

                //LOGD("6");
                if (isEnemy(damagable, character))
                {
                    //LOGD("7");
                    them = Transform_get_position(Component_GetTransform(themtransform));
                    distance = Vector3::Distance(them, myPos);
                    //LOGD("8");
                }

                Vector3 them1 = Transform_get_position(Component_GetTransform(themtransform));

                //;LOGD("9");
                float potentionDistance = Vector3::Distance(them1, myPos);

                if (isEnemy(damagable, get_MyPlayer()) && potentionDistance < distance)
                {
                    //LOGD("10");
                    them = them1;
                    distance = potentionDistance;
                    //LOGD("11");
                }

                //LOGD("12");
                if (isEnemy(damagable, get_MyPlayer()) && distance < 30)
                {

                    //Quaternion rotation = Quaternion::LookRotation(them - myPos, Vector3::Up());
                    //LOGD("My Location: Vector3(%f,%f,%f)", myPos.X, myPos.Y, myPos.Z);
                    //LOGD("13");
                    //Transform_Set_Rotation(Component_GetTransform(transform), rotation);
                    Transform_LookAt(Component_GetTransform(transform), them);
                    //LOGD("14");
                }
            }
        }
    }
}

void telekill(void *character)
{
    if (telekilll)
    {
        void* myTransform = get_PlayerTransform(get_MyPlayer());

        monoList<void **> *playersList = get_PlayerList();
        for (int i = 0; i < playersList->getSize(); i++)
        {

            void *player = playersList->getItems()[i];
            void *themtransform = get_PlayerTransform(player);
            void* damagable = get_PlayerDamageable(player);

            if (isEnemy(damagable, get_MyPlayer()) && !isDead(damagable))
            {
                Vector3 themPos = Transform_get_position(Component_GetTransform(themtransform));
                //Vector3 myPlayerPos = Transform_get_position(Component_GetTransform(myTransform));

                Vector3 myPos = themPos;
                myPos.Z += 3;
                Transform_Set_Position(Component_GetTransform(myTransform), myPos);
            }
        }
    }
}

void MassKill(void *character)
{
    if (mass)
    {
        void* me = get_MyPlayer();
        void* transform = get_PlayerTransform(me);
        //void* cam = *(void**)((uint32_t)me + 0x1C4);
        //LOGD("1");
        monoList<void **> *playersList = get_PlayerList();
        for (int i = 0; i < playersList->getSize(); i++)
        {
            void *player = playersList->getItems()[i];
            void *themtransform = get_PlayerTransform(player);
            void* themdamagable = get_PlayerDamageable(player);
            //LOGD("2");
            Vector3 myPos = Transform_get_position(Component_GetTransform(transform));
            //Vector3 myPos = Transform_get_position(Component_GetTransform(cam));
            myPos.Y =+ 2;
            myPos.Z =+ 3;

            //LOGD("3");
            if (isEnemy(themdamagable, me) && !isDead(themdamagable))
            {
                //LOGD("4");
                Transform_Set_Position(Component_GetTransform(themtransform), myPos);
                //LOGD("5");
                //Transform_Set_Position(Component_GetTransform(transform), me);
            }
        }
    }
}

/*
void (*_SetWeaponsSet)(void *_this);
void SetWeaponsSet(void *_this)
{
    if (allweap)
    {
        void* (*LoadWeaponSet)(void *, void *) = (void* (*)(void *, void *))getRealOffset(0x278D12C); // 0xB185DC
        void* (*get_MultiplayerWSSN)() = (void* (*)())getRealOffset(0x5C657C); // 0x19E92DC
        LoadWeaponSet(_this, get_MultiplayerWSSN());
    }
    else {
        _SetWeaponsSet(_this);
    }
}
*/

void Ammo(void *_this)
{
    if (ammmo)
    {
        monoList<void **> *weaponsList = get_WeaponList(_this);
        for (int i = 0; i < weaponsList->getSize(); i++)
        {
            void *weapons = weaponsList->getItems()[i];

            void* (*set_currentAmmoInClip)(void *, int) =
                    (void* (*)(void *, int))getRealOffset(0x25EDCBC); // WeaponW$$set_currentAmmoInClip

            void* (*set_currentAmmoInBackpack)(void *, int) =
                    (void* (*)(void *, int))getRealOffset(0x25EDC60); // WeaponW$$set_currentAmmoInBackpack

            set_currentAmmoInClip(weapons, 12);
            set_currentAmmoInBackpack(weapons, 84);
        }
    }
}

void GodMode(void *_this)
{
    if (god)
    {
        float (*get_MaxHealth)(void *) =
                (float (*)(void *))getRealOffset(0xF3B3E0); // Player_move_c$$get_MaxHealth

        void* (*set_CurHealth)(void *, float) =
                (void* (*)(void *, float))getRealOffset(0xF21EE0); // Player_move_c$$set_CurHealth

        set_CurHealth(_this, get_MaxHealth(_this));
    }
}

void ninjaa(void *_this)
{
    {
        void *off = get_PlayerSkinName(_this);
        if (off)
        {
            void *off2 = get_SkinNameFirstPersonControlSharp(off);
            if (off2)
            {
                if(ninja && toggle3)
                {
                    *(bool*) ((uint64_t) off2 + 0x224) = true; // secondJumpEnabled0x210
                    *(bool*) ((uint64_t) off2 + 0xD0) = false; // ninjaJumpUsed
                    *(bool*) ((uint64_t) off2 + 0x160) = true; // canJump
                }
                else if (!ninja && toggle3)
                {
                    *(bool*) ((uint64_t) off2 + 0x224) = false; // secondJumpEnabled
                    *(bool*) ((uint64_t) off2 + 0xD0) = true; // ninjaJumpUsed
                    *(bool*) ((uint64_t) off2 + 0x158) = false; // canJump
                }
            }
        }
    }
}

void NoClip(void *_this)
{
    if (toggle8)
    {
        void* off = get_PlayerSkinName(_this);
        if (off)
        {
            void* off2 = get_SkinNameFirstPersonControlSharp(off);
            if (off2)
            {
                void (*CharacterController_set_radius)(void* character, float radius) =
                        (void (*)(void *, float ))getRealOffset(0xEAB82C); // CharacterController$$set_radius
                if (noclipp)
                {
                    void* character = get_FirstPersonControlSharpCharacterController(off2);
                    CharacterController_set_radius(character, INFINITY);
                }
                if (!noclipp)
                {
                    void* character = get_FirstPersonControlSharpCharacterController(off2);
                    CharacterController_set_radius(character, 0.35f);
                }
            }
        }
    }
}

void MeScalePlus(void *_this)
{
    if (me_scale_plus)
    {
        void* myPlayer = get_MyPlayer();
        void* off = get_PlayerSkinName(myPlayer);
        if (off) {
            void *off2 = get_SkinNameFirstPersonControlSharp(off);
            if (off2)
            {
                void* character = get_FirstPersonControlSharpCharacterController(off2);
                Vector3 mysize = Transform_get_localScale(Component_GetTransform(character));
                Vector3 mysize2 = mysize + Vector3(0.5,0.5,0.5);

                Transform_set_localScale(Component_GetTransform(character), mysize2);
                me_scale_plus = false;
            }
        }
    }
}

void MeScaleMinus(void *_this)
{
    if (me_scale_minus)
    {
        void* myPlayer = get_MyPlayer();
        void* off = get_PlayerSkinName(myPlayer);
        if (off) {
            void *off2 = get_SkinNameFirstPersonControlSharp(off);
            if (off2)
            {
                void* character = get_FirstPersonControlSharpCharacterController(off2);
                Vector3 mysize = Transform_get_localScale(Component_GetTransform(character));
                Vector3 mysize2 = mysize + Vector3(-0.5f,-0.5f,-0.5f);

                Transform_set_localScale(Component_GetTransform(character), mysize2);
                me_scale_minus = false;
            }
        }
    }
}

void PlayerGUI(void* _this)
{
    void* player = get_MyPlayer();
    void* ingamegui = *(void **)((uint32_t)player + 0x288);
    //void* offGameGuiPanel = *(void **)((uint32_t)ingamegui + 0x40C);
    void (*SetInterfaceVisible)(void* _this, bool) =
            (void (*)(void* _this, bool))getRealOffset(0x1821F28); // InGameGUI$$SetInterfaceVisible

    SetInterfaceVisible(ingamegui, !gui);
}

void ChatSpam1(void* _this)
{
    if (spam2)
    {
        void (*SendChat)(void* _this, void*, bool, void*) =
                (void (*)(void* _this, void*, bool, void*))getRealOffset(0xF04DF8); // Player_move_c$$SendChat

        SendChat(_this, il2cpp_string_new(txt1), false, il2cpp_string_new(""));
    }
}

void forceCheat(void *_this)
{
    if (cheat)
    {
        //void (*ShowAndClearProgress)(void*) = (void (*)(void*))getRealOffset(0x1967708);

        //ShowAndClearProgress(_this);
        cheat = false;
    }
}

void loadCheat(void *_this)
{
    if (scenecheat)
    {
        LoadScene(_this, il2cpp_string_new("Cheat"));

        scenecheat = false;
    }
}

void loadDev(void *_this)
{
    if (scenedev)
    {
        LoadScene(_this, il2cpp_string_new("DeveloperConsole"));

        //LoadScene(_this, il2cpp_string_new("Cheat"));

        scenedev = false;
    }
}

void TP_Player(void *_this)
{
    if (setloc)
    {
        Vector3 coord = Vector3(0, 0, 0);
        if (PlayerX > 0 && PlayerY > 0 && PlayerZ > 0)
        {
            coord.X = PlayerX;
            coord.Y = PlayerY;
            coord.Z = PlayerZ;

            if (coord.X > 0 && coord.Y > 0 && coord.Z > 0)
            {
                void* myPlayer = get_MyPlayer();

                void* myTransform = get_PlayerTransform(myPlayer);
                Transform_Set_Position(Component_GetTransform(myTransform), coord);

                setloc = false;
                PlayerX = 0;
                PlayerY = 0;
                PlayerZ = 0;
                toggle27 = false;
            }
            else
            {
                LOGD("coord2 (X: %f | Y: %f | Z: %f)", coord.X, coord.Y, coord.Z);
            }
        }
        else
        {
            LOGD("coord1 (X: %f | Y: %f | Z: %f)", PlayerX, PlayerY, PlayerZ);
            setloc = false;
            toggle27 = false;
        }
    }
}

void getCoordinates(void *_this)
{
    if (getloc)
    {
        void* myPlayer = get_MyPlayer();

        void* myTransform = get_PlayerTransform(myPlayer);

        Vector3 gotten = Transform_get_position(Component_GetTransform(myTransform));

        if (gotten.X > 0 && gotten.Y > 0 && gotten.Z > 0)
        {
            playerPos = gotten;

            getloc = false;
            toggle28 = false;
        }
    }
}


void getScale(void *_this)
{
    if (getloc)
    {
        void* myPlayer = get_MyPlayer();
        void* off = get_PlayerSkinName(myPlayer);
        if (off) {
            void *off2 = get_SkinNameFirstPersonControlSharp(off);
            if (off2)
            {
                void* character = get_FirstPersonControlSharpCharacterController(off2);
                Vector3 mysize = Transform_get_localScale(Component_GetTransform(character));

                if (!NOTfirstscale) {
                    PlayerScale = mysize;
                    LOGD("PlayerScale = Vector3(%f, %f, %f)", PlayerScale.X, PlayerScale.Y, PlayerScale.Z);
                    NOTfirstscale = true;
                }

                PlayerScaleMod = PlayerScale;

                getloc = false;

            }
        }

    }
}

void setScale(void *_this) {
    if (setscale)
    {
        void* myPlayer = get_MyPlayer();
        void* off = get_PlayerSkinName(myPlayer);
        if (off) {
            void *off2 = get_SkinNameFirstPersonControlSharp(off);
            if (off2)
            {
                void* character = get_FirstPersonControlSharpCharacterController(off2);
                Vector3 mysize = Transform_get_localScale(Component_GetTransform(character));
                Vector3 mysize2 = mysize + Vector3(mysize.X, PlayerScaleMod.Y ,mysize.Z);

                Transform_set_localScale(Component_GetTransform(character), mysize2);
                setscale = false;
            }
        }
    }
}

void changePlayerID(void *_this)
{
    if (playerid)
    {
        Storager_set_string("AccountCreated", txt2);

        LOGD("Player ID Changed: %s", txt2);
        playerid = false;
        Crash();
    }
}


void AddCoins(void* _this)
{
    void (*AddCoins)(void* _this, int, bool, int) = (void (*)(void* _this, int, bool, int))getRealOffset(0x19775D4); // BankController$$AddCoins

    if (addcoinsclicked && curramount > 0)
    {
        AddCoins(get_bankControllerStatic(_this), curramount, false, 1);
        addcoinsclicked = false;
        curramount = 0;
    }

}

void AddGems(void* _this)
{
    void (*AddGems)(void* _this, int, bool, int) = (void (*)(void* _this, int, bool, int))getRealOffset(0x1977830); // BankController$$AddGems

    if (addgemsclicked && curramount > 0)
    {
        AddGems(get_bankControllerStatic(_this), curramount, false, 1);
        addgemsclicked = false;
        curramount = 0;
    }
}

void AddEventCurrency(void* _this)
{
    void (*AddEventCurrency)(void* _this, int, bool, int) = (void (*)(void* _this, int, bool, int))getRealOffset(0x1977B6C); // BankController$$AddEventCurrency

    if (addeventclicked && curramount > 0)
    {
        AddEventCurrency(get_bankControllerStatic(_this), curramount, false, 1);
        addeventclicked = false;
        curramount = 0;
    }
}

void AddBattlePassCurrency(void* _this)
{
    void (*AddBattlePassCurrency)(void* _this, int, bool, int) = (void (*)(void* _this, int, bool, int))getRealOffset(0x1977DD0); // BankController$$AddBattlePassCurrency

    if (addbpclicked && curramount > 0)
    {
        AddBattlePassCurrency(get_bankControllerStatic(_this), curramount, false, 1);
        addbpclicked = false;
        curramount = 0;
    }
}

void AddCraftCurrency(void* _this)
{
    void (*AddCraftCurrency)(void* _this, int, bool, int) = (void (*)(void* _this, int, bool, int))getRealOffset(0x1977E90); // BankController$$AddCraftCurrency

    if (addcraftclicked && curramount > 0)
    {
        AddCraftCurrency(get_bankControllerStatic(_this), curramount, false, 1);
        addcraftclicked = false;
        curramount = 0;
    }
}

void AddCouponsCurrency(void* _this)
{
    void (*AddCouponsCurrency)(void* _this, int, bool, int) = (void (*)(void* _this, int, bool, int))getRealOffset(0x1977F50); // BankController$$AddCouponsCurrency

    if (addbpclicked && curramount > 0)
    {
        AddCouponsCurrency(get_bankControllerStatic(_this), curramount, false, 1);
        addcouponclicked = false;
        curramount = 0;
    }
}


void SummonGem(void *_this)
{
    void* bonus = get_bonusControllerStatic();
    if (bonus && spawngem)
    {
        void (*AddBonusAfterKillPlayer)(void* _this, Vector3 pos) =
                (void (*)(void* _this, Vector3 pos))getRealOffset(0x1C63C84); // BonusController$$AddBonusAfterKillPlayer

        Vector3 mypos = get_PlayerPosition();
        Vector3 gempos = Vector3(mypos.X, mypos.Y, mypos.Z - 2);
        AddBonusAfterKillPlayer(bonus, gempos);
        spawngem = false;
        return;
    }
    LOGD("BonusController.sharedController = %p", bonus);
}

void damageEveryone(void *_this)
{
    if (dmg)
    {
        void* me = get_MyPlayer();

        monoList<void **> *playersList = get_PlayerList();

        for (int i = 0; i < playersList->getSize(); i++)
        {
            void *player = playersList->getItems()[i];

            void* damagable = get_PlayerDamageable(player);

            void *themtransform = get_PlayerTransform(player);

            void* themobject = Component_GetGameObject(themtransform);

            if (isEnemy(damagable, me))
            {
                void (*DamageTarget)(void* _this, void*, float, void*, int, int) =
                        (void (*)(void*, void*, float, void*, int, int))getRealOffset(0xF3C7A0); // Player_move_c$$DamageTarget

                DamageTarget(_this, themobject, 25, il2cpp_string_new("gadget_dragonwhistle"), 0, 3);
            }
        }
    }
}

/*
void (*_SetWeaponsSet)(void* _this);
void SetWeaponsSet(void* _this)
{
    if (_this && allweap)
    {
        void (*LoadWeaponSet)(void* _this, void* set) = (void (*)(void* _this, void*))getRealOffset(0xA97750);
        LoadWeaponSet(_this, il2cpp_string_new("MultiplayerWSSN"));
    }
}
*/

bool (*orig_haskey)(void* _this, void* key);
bool enc_haskey(void* _this, void* key)
{
    bool (*PlayerPrefs_HasKey)(void* __this, void* key) =
            (bool (*)(void*, void*))getRealOffset(0x2FF6A54); // PlayerPrefs$$HasKey
    Debug_Log(key);
    return PlayerPrefs_HasKey(_this, key);
}

void* (*orig_getstring)(void* _this, void* key);
void* enc_getstring(void* _this, void* key)
{
    void* (*PlayerPrefs_GetString)(void* __this, void* key) =
            (void *(*)(void*, void*))getRealOffset(0x2FF6940); // PlayerPrefs$$GetString
    Debug_Log(key);
    return PlayerPrefs_GetString(_this, key);
}

void (*orig_setstring)(void* _this, void* key, void* val);
void enc_setstring(void* _this, void* key, void* val)
{
    void (*PlayerPrefs_SetString)(void* __this, void* key, void* val) =
            (void (*)(void*, void*, void*))getRealOffset(0x2FF688C); // PlayerPrefs$$SetString
    Debug_Log(key);
    Debug_Log(val);
    PlayerPrefs_SetString(_this, key, val);
}

void (*_WeapSounds)(void* pThis);
void WeapSounds(void* player)
{
    if (player)
    {
        void* off = player;
        if (toggle1 && shotgun) // shotgun
        {
            *(bool*) ((uint64_t) off + 0xE6) = true; // isShotGun
        }
        if (toggle1 && !shotgun) // shotgun
        {
            *(bool*) ((uint64_t) off + 0xE6) = false; // isShotGun
        }
        if (toggle4 && xray) // Zoom XRay
        {
            *(bool*) ((uint64_t) off + 0x69) = false; // zoomXray
        }
        if (toggle4 && !xray) // Zoom XRay
        {
            *(bool*) ((uint64_t) off + 0x69) = true; // zoomXray
        }
        if (toggle7 && explodeshot) // explode shot
        {
            *(bool*) ((uint64_t) off + 0xE5) = true; // bulletExplode
        }
        if (toggle7 && !explodeshot) // explode shot
        {
            *(bool*) ((uint64_t) off + 0xE5) = false; // bulletExplode
        }

    }
    //_WeapSounds(player);
}


bool (*orig_CanSpawnGemBonus)(void* pThis);
bool mod_CanSpawnGemBonus(void* _this)
{
    return true;
}

int (*orig_IndexBonusOnKill)(void* pThis);
int mod_IndexBonusOnKill(void* _this)
{
    return 9;
}

void (*_Update)(void* pThis);
void Update(void* pThis)
{
    void* player = pThis; //get_LocalPlayer();
    if (player)
    {
        if (toggle1) {
            //std::thread bot(shotgunn, player); bot.join();
        }
        if (toggle3) {
            std::thread bot(ninjaa, player); bot.join();
        }
        if (toggle4) {
            std::thread bot(DoXray, player); bot.join();
        }
        if (toggle6) {
            std::thread bot(AimBot, player); bot.join();
        }
        if (toggle8) {
            std::thread bot(NoClip, player); bot.join();
        }
        if (toggle10) {
            //std::thread bot(GodMode, player); bot.join();
        }
        if (toggle12) {
            std::thread bot(Ammo, player); bot.join();
        }
        if (toggle13) {
            std::thread bot(telekill, player); bot.join();
        }
        if (toggle14) {
            //std::thread bot(thirdperson, player); bot.join();
        }
        if (toggle15) {
            //std::thread bot(DamageAll, player); bot.join();
        }
        if (toggle16) {
            std::thread bot(MeScalePlus, player); bot.join();
        }
        if (toggle17) {
            //std::thread bot(SlowdownTarget, player); bot.join();
        }
        if (toggle18) {
            //std::thread bot(InfoESP, player); bot.join();
        }
        if (toggle19) {
            std::thread bot(ChatSpam1, player); bot.join();
        }
        if (toggle20) {
            std::thread bot(MeScaleMinus, player); bot.join();
        }
        if (toggle21) {
            std::thread bot(PlayerGUI, player); bot.join();
        }
        if (toggle22) {
            std::thread bot(MassKill, player); bot.join();
        }
        if (toggle23) {
            std::thread bot(ChatSpam1, player); bot.join();
        }
        if (toggle24) {
            //std::thread bot(forceCheat, player); bot.join();
        }
        if (toggle25) {
            std::thread bot(loadCheat, player); bot.join();
        }
        if (toggle26) {
            std::thread bot(loadDev, player); bot.join();
        }
        if (toggle27) {
            std::thread bot(TP_Player, player); bot.join();
        }
        if (toggle28)
        {
            std::thread bot(getCoordinates, player); bot.join();
        }
        if (toggle29) {
            std::thread bot(damageEveryone, player); bot.join();
        }
        if (toggle30) {
            std::thread bot(getScale, player); bot.join();
        }
        if (toggle31) {
            std::thread bot(setScale, player); bot.join();
        }
        if (toggle32) {
            std::thread bot(changePlayerID, player); bot.join();
        }/*
        if (toggle33) {
            //std::thread bot(, player); bot.join();
        }
        if (toggle34) {
            std::thread bot(SummonGem, player); bot.join();
        }

        if (addcoinsclicked) {
            std::thread bot(AddCoins, player); bot.join();
        }
        if (addgemsclicked) {
            std::thread bot(AddGems, player); bot.join();
        }
        if (addeventclicked) {
            std::thread bot(AddEventCurrency, player); bot.join();
        }
        if (addbpclicked) {
            std::thread bot(AddBattlePassCurrency, player); bot.join();
        }
        if (addcraftclicked) {
            std::thread bot(AddCraftCurrency, player); bot.join();
        }
        if (addcouponclicked) {
            std::thread bot(AddCouponsCurrency, player); bot.join();
        }*/

        //std::thread bot(bBanks, player); bot.join();
    }
    _Update(pThis);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_jbro129_FloatingActivity_invoke(JNIEnv *env, jclass type, jint which, jstring text, jboolean is) {
    int select = (jint) which;
    bool set = (bool) is;
    const char* gtxt = env->GetStringUTFChars(text, 0);
    LOGD("invoke(%i, %s, %i)",select, gtxt, set);
    if (select == 0)
    {
        LOGD("invoke 0 before start true");
        bool start = true;

        LOGD("Trying to find libil2cpp.so in maps...");

        if (location <= 0)
        {
            location = getRealOffset(0);
            LOGD("found at %u", getRealOffset(0));
            start = false;
        }

        if (!start)
        {
            LOGD("Hooked!");
        }
    }
    if (select == 1)// shotgun
    {
        shotgun = set;
        toggle1 = true;
    }
    if (select == 2)// price
    {
        price = set;
        toggle2 = true;
    }
    if (select == 3)// ninja
    {
        ninja = set;
        toggle3 = true;
    }
    if (select == 4)// xray
    {
        xray = set;
        toggle4 = true;
    }
    if (select == 5)// level
    {
        levell = set;
        toggle5 = true;
    }
    if (select == 6)// aimbot
    {
        aimbot = set;
        toggle6 = true;
    }
    if (select == 7)// explode shot
    {
        explodeshot = set;
        toggle7 = true;
    }
    if (select == 8)// noclip
    {
        noclipp = set;
        toggle8 = true;
    }
    if (select == 9)// firerate
    {
        firerate = set;
        toggle9 = true;
    }
    if (select == 10)// godmode
    {
        god = set;
        toggle10 = true;
    }
    if (select == 11)// damage
    {
        shott = set;
        toggle11 = true;
    }
    if (select == 12)// ammo
    {
        ammmo = set;
        toggle12 = true;
    }
    if (select == 13)// telekill
    {
        telekilll = set;
        toggle13 = true;
    }
    if (select == 14)// third person
    {
        //spawngem = set;
        //gems = set;
        //thirdpersonn = set;
        toggle14 = true;
    }
    if (select == 15)// DamageAll
    {
        damageall = set;
        toggle15 = true;
    }
    if (select == 16)// me scale ++
    {
        me_scale_plus = set;
        toggle16 = true;
    }
    if (select == 17)// Slowdown
    {
        slowdownall = set;
        toggle17 = true;
    }
    if (select == 18)// Info ESP
    {
        isp = set;
        toggle18 = true;
    }
    if (select == 19)
    {
        spam = set;
        toggle19 = true;
    }
    if (select == 20)// me scale --
    {
        me_scale_minus = set;
        toggle20 = true;
    }
    if (select == 21)// InGameGUI
    {
        gui = set;
        toggle21 = true;
    }
    if (select == 22)// MassKill
    {
        mass = set;
        toggle22 = true;
    }
    if (select == 23)
    {
        spam2 = set;

        txt1 = env->GetStringUTFChars(text, 0);

        toggle23 = true;
    }
    if (select == 24)// Cheat Detected
    {
        cheat = set;
        toggle24 = true;
    }
    if (select == 25)// Scene Cheat
    {
        scenecheat = set;
        toggle25 = true;
    }
    if (select == 26)// Scene Dev
    {
        scenedev = set;
        toggle26 = true;
    }
    if (select == 27)// Send Location
    {
        setloc = set;
        toggle27 = true;
    }
    if (select == 28)// Grab Location
    {
        getloc = set;
        toggle28 = true;
    }
    if (select == 29)// Damage everyone
    {
        dmg = set;
        toggle29 = true;
    }
    if (select == 30)// Get Scale
    {
        getscale = set;
        toggle30 = true;
    }
    if (select == 31)// Set Scale
    {
        setscale = set;
        toggle31 = true;
    }
    if (select == 32)// Player ID
    {
        playerid = set;

        txt2 = env->GetStringUTFChars(text, 0);

        toggle32 = true;
    }
    if (select == 33)// Any Weapon Allowed
    {
        toggle33 = true;
        //MSHookFunction((void *) getRealOffset(0x27908D4), (void *) &SetWeaponsSet, (void **) &_SetWeaponsSet);
    }
    if (select == 34)// Debug
    {
        spawngem = set;
        toggle34 = true;
        MSHookFunction((void *) getRealOffset(0x1C6581C), (void *) &mod_CanSpawnGemBonus, (void **) &orig_CanSpawnGemBonus); // BonusController$$CanSpawnGemBonus
        MSHookFunction((void *) getRealOffset(0x1C64074), (void *) &mod_IndexBonusOnKill, (void **) &orig_IndexBonusOnKill); // BonusController$$IndexBonusOnKill
    }
	
    MSHookFunction((void *) getRealOffset(0x25E0B30), (void *) &Update, (void **) &_Update); // Player_move_c$$UpdateEffects
    //MSHookFunction((void *) getRealOffset(0x20C4B24), (void *) &Bonus_Awake, (void **) &_Bonus_Awake);
    //MSHookFunction((void *) getRealOffset(0x15DD6AC), (void *) &WeapSounds, (void **) &_WeapSounds); // WeaponSounds$$CheckPlayDefaultAnimInMulti


    //MSHookFunction((void *) getRealOffset(0x2621DF8), (void *) &enc_setstring, (void **) &orig_setstring); // EncryptedPlayerPrefs$$SetString
    //MSHookFunction((void *) getRealOffset(0x2621854), (void *) &enc_getstring, (void **) &orig_getstring); // EncryptedPlayerPrefs$$GetString
    //MSHookFunction((void *) getRealOffset(0x26223F0), (void *) &enc_haskey, (void **) &orig_haskey); // EncryptedPlayerPrefs$$HasKey

    //MSHookFunction((void *) getRealOffset(0x1CD450C), (void *) &EventUrl, (void **) &_EventUrl);
    //MSHookFunction((void *) getRealOffset(0x23C1AD4), (void *) &MapListUrl, (void **) &_MapListUrl);
    //MSHookFunction((void *) getRealOffset(0x72BC54), (void *) &PixelBookSettingsUrl, (void **) &_PixelBookSettingsUrl);
    //MSHookFunction((void *) getRealOffset(0xE26B60), (void *) &BalanceUrl, (void **) &_BalanceUrl);
    //MSHookFunction((void *) getRealOffset(0x1DA9AF4), (void *) &SignaturesUrl, (void **) &_SignaturesUrl);
    //env->ReleaseStringUTFChars(text, nativeString);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jbro129_FloatingActivity_sendLocation(JNIEnv *env, jclass type, jfloat X, jfloat Y, jfloat Z) {

    PlayerX = (float)X;
    PlayerY = (float)Y;
    PlayerZ = (float)Z;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_jbro129_FloatingActivity_grabLocation(JNIEnv *env, jclass type) {

    std::string X = tostr(playerPos.X);
    std::string Y = tostr(playerPos.Y);
    std::string Z = tostr(playerPos.Z);

    std::string done = X + "," + Y + "," + Z;

    const char* ret = done.c_str();

    return env->NewStringUTF(ret);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_jbro129_FloatingActivity_sendScale(JNIEnv *env, jclass type, jfloat y)
{
    PlayerScaleMod.Y = y;
}

extern "C"
JNIEXPORT jfloat JNICALL
Java_com_jbro129_FloatingActivity_grabScale(JNIEnv *env, jclass type) {

    return PlayerScale.X;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jbro129_FloatingActivity_mprotect(JNIEnv *env, jclass type, jlong addr, jlong len) {

    return mprotect((void *)(uintptr_t) addr, (size_t)len, PROT_WRITE | PROT_READ | PROT_EXEC);

}
extern "C"
JNIEXPORT jlong JNICALL
Java_com_jbro129_FloatingActivity_sysconf(JNIEnv *env, jclass type, jint name) {

    return sysconf(name);
}extern "C"
JNIEXPORT void JNICALL
Java_com_jbro129_FloatingActivity_testFormatter(JNIEnv *env, jclass type, jstring hex)
{
    const char* chr = env->GetStringUTFChars(hex, 0);
    std::string str = formatter(chr);
    LOGD("%s -> %s", chr, str.c_str());
    env->ReleaseStringUTFChars(hex, chr);

}extern "C"
JNIEXPORT void JNICALL
Java_com_jbro129_FloatingActivity_addCurrency(JNIEnv *env, jclass type, jint whichh, jint amountt) {

    int which = (int)whichh;
    int amount = (int)amountt;
    if (which == 0) // Coins
    {
        addcoinsclicked = true;
        curramount = amount;
    }
    else if (which == 1) // Gems
    {
        addgemsclicked = true;
        curramount = amount;
    }
    else if (which == 2) // BattlePass
    {
        addbpclicked = true;
        curramount = amount;
    }
    else if (which == 3) // Event
    {
        addeventclicked = true;
        curramount = amount;
    }
    else if (which == 4) // Craft
    {
        addcraftclicked = true;
        curramount = amount;
    }
    else if (which == 5) // Coupon
    {
        addcouponclicked = true;
        curramount = amount;
    }

}