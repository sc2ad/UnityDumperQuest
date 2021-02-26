#include <dlfcn.h>
#include "extern/beatsaber-hook/shared/utils/utils.h"
#include "extern/beatsaber-hook/shared/utils/logging.hpp"
#include "extern/beatsaber-hook/shared/utils/typedefs.h"
#include "extern/beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "extern/beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "extern/beatsaber-hook/shared/config/config-utils.hpp"
#include "modloader/shared/modloader.hpp"

#define PATH "/sdcard/Android/data/com.beatgames.beatsaber/files/logdump-"
#define EXT ".txt"

static ModInfo modInfo;

static Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

static Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

void write_info(FILE* fp, std::string str) {
    getLogger().debug("%s", str.data());
    fwrite((str + "\n").data(), str.length() + 1, 1, fp);
}

void DumpParents(FILE* fp, std::string prefix, Il2CppObject* parentTransform) {
    // Get children
    int childCount = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(parentTransform, "childCount"));
    Il2CppString* parentName = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Il2CppString*>(parentTransform, "name"));
    write_info(fp, prefix + (parentName != nullptr ? to_utf8(csstrtostr(parentName)) : "<NULL NAME>") + " Children: " + std::to_string(childCount));
    Il2CppObject* child;

    for (int i = 0; i < childCount; i++) {
        child = CRASH_UNLESS(il2cpp_utils::RunMethod<Il2CppObject*>(parentTransform, "GetChild", i));
        if (child) {
            DumpParents(fp, prefix + "-", child);
        }
    }
}

// Iterates over all GameObjects in the scene, dumps information about them to file
void DumpAll(std::string name) {
    FILE* fp = fopen((PATH + name + EXT).data(), "w");
    static auto typeObject = il2cpp_utils::GetSystemType("UnityEngine", "GameObject");
    getLogger().debug("Logging to path: %s", (PATH + name + EXT).data());

    getLogger().debug("Getting all GameObjects!");
    Array<Il2CppObject*>* arr = CRASH_UNLESS(il2cpp_utils::RunMethod<Array<Il2CppObject*>*>("UnityEngine", "Resources", "FindObjectsOfTypeAll", typeObject));
    Il2CppObject* transform;
    Il2CppObject* parentTransform;
    Il2CppString* goName;
    for (il2cpp_array_size_t i = 0; i < arr->Length(); i++) {
        auto go = arr->values[i];
        if (go) {
            transform = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(go, "transform"));
            goName = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Il2CppString*>(go, "name"));
            if (transform) {
                parentTransform = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(transform, "parent"));
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
    Il2CppString* name = CRASH_UNLESS(il2cpp_utils::RunMethod<Il2CppString*>(&scene, il2cpp_utils::FindMethod("UnityEngine.SceneManagement", "Scene", "get_name")));
    if (name) {
        getLogger().debug("DUMPING SCENE: %s", to_utf8(csstrtostr(name)).data());
        DumpAll(to_utf8(csstrtostr(name)));
    } else {
        getLogger().debug("NAME NULL FOR SCENE WITH HANDLE: %d", scene.m_Handle);
    }
}

// This function is called before a mod is constructed for the first time.
// Perform one time setup, as well as specify the mod ID and version here.
extern "C" void setup(ModInfo& info) {
    // This should be the first thing done
    modInfo = info;
    info.id = "UnityDumperQuest";
    info.version = "0.1.0";
    getLogger().info("Completed setup!");
    // We can even check information specific to the modloader!
    getLogger().info("Modloader name: %s", Modloader::getInfo().name.c_str());
    // Load config
    getConfig().Load();
    // Can use getConfig().config to modify the rapidjson document
    getConfig().Write();
}

// This function is called when the mod is loaded for the first time, immediately after il2cpp_init.
extern "C" void load() {
    getLogger().debug("Installing Unity Dumper!");
    INSTALL_HOOK_OFFSETLESS(getLogger(), SceneManager_Internal_SceneLoaded, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_SceneLoaded", 2));
    getLogger().debug("Installed Unity Dumper!");
    getLogger().info("initialized: %s", il2cpp_functions::initialized ? "true" : "false");
}