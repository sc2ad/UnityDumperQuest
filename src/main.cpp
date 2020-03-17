#include "../include/main.hpp"

#define PATH "/sdcard/Android/data/com.beatgames.beatsaber/files/logdump"
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
    fwrite(str.data(), str.length() + 1, 1, fp);
}

void DumpParents(FILE* fp, std::string prefix, Il2CppObject* parentTransform) {
    // Get children
    int childCount;
    RUN_METHOD0(&childCount, parentTransform, "get_childCount");
    Il2CppString* parentName;
    RUN_METHOD0(&parentName, parentTransform, "get_name");
    write_info(fp, prefix + to_utf8(csstrtostr(parentName)) + " Children: " + std::to_string(childCount));
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
    log(DEBUG, "Logging to path: %s", (PATH + name + EXT).data());

    Il2CppArray* arr;
    static auto typeObject = il2cpp_utils::GetSystemType("UnityEngine", "GameObject");
    RUN_METHOD1(&arr, il2cpp_utils::GetClassFromName("UnityEngine", "Resources"), "FindObjectsOfTypeAll", typeObject);
    Il2CppObject* transform;
    for (il2cpp_array_size_t i = 0; i < arr->bounds->length; i++) {
        auto go = il2cpp_array_get(arr, Il2CppObject*, 0);
        if (go != nullptr) {
            RUN_METHOD0(&transform, go, "get_transform");
            if (transform != nullptr) {
                RUN_METHOD0(&transform, transform, "get_parent");
                if (transform != nullptr) {
                    DumpParents(fp, "", transform);
                }
            } else {
                Il2CppString* goName;
                RUN_METHOD0(&goName, go, "get_name");
                write_info(fp, to_utf8(csstrtostr(goName)) + " has no transform!");
            }
        } else {
            write_info(fp, "GameObject is null!");
        }
    }

    fclose(fp);
}

MAKE_HOOK_OFFSETLESS(SceneManager_Internal_SceneLoaded, void, Il2CppObject* scene, int mode) {
    SceneManager_Internal_SceneLoaded(scene, mode);
    // Get name of scene
    Il2CppString* name;
    RUN_METHOD0(&name, scene, "get_name");
    DumpAll(to_utf8(csstrtostr(name)));
}

extern "C" void load() {
    log(DEBUG, "Installing Unity Dumper!");
    INSTALL_HOOK_OFFSETLESS(SceneManager_Internal_SceneLoaded, il2cpp_utils::FindMethod("UnityEngine", "SceneManager", "Internal_SceneLoaded", 2));
    log(DEBUG, "Installed Unity Dumper!");
}