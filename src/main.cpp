#include "../include/main.hpp"

#define PATH "/sdcard/Android/data/com.beatgames.beatsaber/files/logdump-"
#define EXT ".txt"

#define RUN_METHOD0(outp, inp, name) \
do { \
    if (!il2cpp_utils::RunMethod(outp, inp, name)) { \
        log(CRITICAL, "Failed to run method!"); \
        std::abort(); \
    } \
} while(0);

#define RUN_METHOD1(outp, inp, name, arg1) \
do { \
    if (!il2cpp_utils::RunMethod(outp, inp, name, arg1)) { \
        log(CRITICAL, "Failed to run method!"); \
        std::abort(); \
    } \
} while(0);

void write_info(FILE* fp, std::string str) {
    log(DEBUG, "%s", str.data());
    fwrite((str + "\n").data(), str.length() + 1, 1, fp);
}

void DumpParents(FILE* fp, std::string prefix, Il2CppObject* parentTransform) {
    // Get children
    int childCount;
    RUN_METHOD0(&childCount, parentTransform, "get_childCount");
    Il2CppString* parentName;
    RUN_METHOD0(&parentName, parentTransform, "get_name");
    write_info(fp, prefix + (parentName != nullptr ? to_utf8(csstrtostr(parentName)) : "<NULL NAME>") + " Children: " + std::to_string(childCount));
    Il2CppObject* child;

    for (int i = 0; i < childCount; i++) {
        RUN_METHOD1(&child, parentTransform, "GetChild", i);
        if (child) {
            DumpParents(fp, prefix + "-", child);
        }
    }
}

// Iterates over all GameObjects in the scene, dumps information about them to file
void DumpAll(std::string name) {
    FILE* fp = fopen((PATH + name + EXT).data(), "w");
    static auto typeObject = il2cpp_utils::GetSystemType("UnityEngine", "GameObject");
    log(DEBUG, "Logging to path: %s", (PATH + name + EXT).data());

    Array<Il2CppObject *>* arr;
    log(DEBUG, "Getting all GameObjects!");
    RUN_METHOD1(&arr, il2cpp_utils::GetClassFromName("UnityEngine", "Resources"), "FindObjectsOfTypeAll", typeObject);
    Il2CppObject* transform;
    Il2CppObject* parentTransform;
    for (il2cpp_array_size_t i = 0; i < arr->Length(); i++) {
        auto go = arr->values[i];
        if (go != nullptr) {
            RUN_METHOD0(&transform, go, "get_transform");
            Il2CppString* goName;
            RUN_METHOD0(&goName, go, "get_name");
            if (transform != nullptr) {
                RUN_METHOD0(&parentTransform, transform, "get_parent");
                if (parentTransform == nullptr) {
                    // Has no parent!
                    write_info(fp, "GameObject: " + (goName != nullptr ? to_utf8(csstrtostr(goName)) : "<NULL GAMEOBJECT NAME>"));
                    DumpParents(fp, "", transform);
                }
            } else {
                write_info(fp, (goName != nullptr ? to_utf8(csstrtostr(goName)) : "<NULL GAMEOBJECT NAME>") + " has no transform!");
            }
        } else {
            write_info(fp, "GameObject is null!");
        }
    }

    fclose(fp);
}

MAKE_HOOK_OFFSETLESS(SceneManager_Internal_SceneLoaded, void, Scene scene, int mode) {
    SceneManager_Internal_SceneLoaded(scene, mode);
    // Get name of scene
    Il2CppString* name;
    RUN_METHOD0(&name, &scene, il2cpp_utils::FindMethod("UnityEngine.SceneManagement", "Scene", "get_name", 0));
    if (name) {
        log(DEBUG, "DUMPING SCENE: %s", to_utf8(csstrtostr(name)).data());
        DumpAll(to_utf8(csstrtostr(name)));
    } else {
        log(DEBUG, "NAME NULL FOR SCENE WITH HANDLE: %d", scene.m_Handle);
    }
}

extern "C" void load() {
    log(DEBUG, "Installing Unity Dumper!");
    INSTALL_HOOK_OFFSETLESS(SceneManager_Internal_SceneLoaded, il2cpp_utils::FindMethod("UnityEngine.SceneManagement", "SceneManager", "Internal_SceneLoaded", 2));
    log(DEBUG, "Installed Unity Dumper!");
}