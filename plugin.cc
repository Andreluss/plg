#include <gcc-plugin.h>
#include <plugin-version.h>
#include <tree.h>
#include <tree-pass.h>
#include <context.h>
#include <stringpool.h>
#include <toplev.h>
#include <cp/cp-tree.h>
#include <hash-set.h>
#include <cstring>
#include <diagnostic-core.h>
#include <cgraph.h>
#include <function.h>
#include <gimple.h>
#include <cp/type-utils.h>
#include <cp/cp-tree.h>
#include <intl.h>
#include <print-tree.h>
#include <iostream>
int plugin_is_GPL_compatible;

// Plugin initialization
int plugin_init(struct plugin_name_args *plugin_info,
               struct plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        error(G_("Plugin version mismatch"));
        return 1;
    }
    
    register_callback(plugin_info->base_name, PLUGIN_PRE_GENERICIZE, 
                      [](void *gcc_data, void *user_data) {
                          // This is where you can add your plugin logic
                          std::cout << "Plugin callback executed." << std::endl;
                      }, nullptr);

    return 0;
}