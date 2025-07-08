#include <gcc-plugin.h>
#include <plugin-version.h>
#include <tree-pass.h>
#include <context.h>
#include <function.h>
#include <tree.h>

int plugin_is_GPL_compatible = 1; // Required (GPL license)

static void callback_finish_decl(void *event_data, void *user_data) {
    tree decl = (tree)event_data;
    if (TREE_CODE(decl) == FUNCTION_DECL) {
        const char *func_name = IDENTIFIER_POINTER(DECL_NAME(decl));
        printf("Found function: %s\n", func_name);
    }
}

int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version))
        return 1;

    printf("Example GCC Plugin Loaded!\n");
    register_callback(info->base_name, PLUGIN_FINISH_DECL, callback_finish_decl, NULL);

    return 0;
}