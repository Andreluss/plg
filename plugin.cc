#include <gcc-plugin.h>
#include <plugin-version.h>
#include <tree.h>
#include <tree-pass.h>
#include <context.h>
#include <stringpool.h>
#include <toplev.h>
#include <cp/cp-tree.h> // For C++ specific features
#include <hash-set.h>   // For hash_set
#include <cstring>      // For strcmp
#include <diagnostic.h> // For warning/inform
#include <cgraph.h>
#include <intl.h>
#include <langhooks.h>

int plugin_is_GPL_compatible; // Required for GCC plugins

// Helper function to get function name as string
static const char* get_function_name(tree decl) {
    if (DECL_NAME(decl)) {
        return IDENTIFIER_POINTER(DECL_NAME(decl));
    }
    return "<unnamed>";
}

// Enhanced signature comparison that handles C++ features
static bool compare_foo_signatures(tree decl1, tree decl2) {
    // Basic function check
    if (TREE_CODE(decl1) != FUNCTION_DECL || TREE_CODE(decl2) != FUNCTION_DECL) {
        return false;
    }

    // Compare return types
    tree type1 = TREE_TYPE(decl1);
    tree type2 = TREE_TYPE(decl2);
    
    if (!lang_hooks.types_compatible_p(TREE_TYPE(type1), TREE_TYPE(type2))) {
        return false;
    }

    // Handle C++ cv-qualifiers for member functions
    if (FUNC_OR_METHOD_TYPE_P(decl1) || 
        FUNC_OR_METHOD_TYPE_P(decl2)) {
        if (!same_type_p(DECL_CONTEXT(decl1), DECL_CONTEXT(decl2))) {
            return false;
        }
        if (TYPE_READONLY(type1) != TYPE_READONLY(type2) ||
            TYPE_VOLATILE(type1) != TYPE_VOLATILE(type2)) {
            return false;
        }
    }

    // Compare parameters
    tree args1 = TYPE_ARG_TYPES(type1);
    tree args2 = TYPE_ARG_TYPES(type2);
    
    while (args1 && args2 && args1 != void_list_node && args2 != void_list_node) {
        if (!lang_hooks.types_compatible_p(TREE_VALUE(args1), TREE_VALUE(args2))) {
            return false;
        }
        args1 = TREE_CHAIN(args1);
        args2 = TREE_CHAIN(args2);
    }
    
    // Check for variadic functions or parameter count mismatch
    if ((args1 && args1 != void_list_node) || (args2 && args2 != void_list_node)) {
        return false;
    }

    // Handle C++ exception specifications
    if (flag_exceptions) {
        tree raises1 = TYPE_RAISES_EXCEPTIONS(type1);
        tree raises2 = TYPE_RAISES_EXCEPTIONS(type2);
        
        if (!comp_except_specs(raises1, raises2, /*exact=*/false)) {
            return false;
        }
    }

    return true;
}

// Our pass that finds and compares foo functions
class foo_comparison_pass : public ipa_opt_pass_d {
public:
    foo_comparison_pass(gcc::context *ctxt)
        : ipa_opt_pass_d(pass_data, ctxt, 
                        NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL) {}
    
    virtual unsigned int execute(void) override {
        hash_set<tree> processed_foos;
        bool found_duplicate = false;
        
        // Walk all functions in compilation unit
        tree var;
        FOR_EACH_FUNCTION(var) {
            if (TREE_CODE(var) == FUNCTION_DECL) {
                const char* name = get_function_name(var);
                
                if (name && strcmp(name, "foo") == 0) {
                    // Compare with all previously seen foo functions
                    for (hash_set<tree>::iterator it = processed_foos.begin();
                         it != processed_foos.end(); ++it) {
                        tree other = *it;
                        if (compare_foo_signatures(var, other)) {
                            warning_at(DECL_SOURCE_LOCATION(var), 0, 
                                     "Duplicate foo signature detected:");
                            warning_at(DECL_SOURCE_LOCATION(other), 0, 
                                     "  Previous declaration here");
                            found_duplicate = true;
                        }
                    }
                    processed_foos.add(var);
                }
            }
        }
        
        if (found_duplicate) {
            inform(UNKNOWN_LOCATION, "Note: Includes template instantiations");
        }
        
        return 0;
    }
    
private:
    static const pass_data pass_data;
};

const pass_data foo_comparison_pass::pass_data = {
    IPA_PASS,                   // type
    "foo-comparison",           // name
    OPTGROUP_NONE,              // optinfo_flags
    TV_NONE,                    // tv_id
    0,                          // properties_required
    0,                          // properties_provided
    0,                          // properties_destroyed
    0,                          // todo_flags_start
    0                           // todo_flags_finish
};

// Plugin initialization
int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
    // Verify GCC version matches
    if (!plugin_default_version_check(version, &gcc_version)) {
        error(G_("Plugin version mismatch"));
        return 1;
    }
    
    // Register our pass
    struct register_pass_info pass_info;
    pass_info.pass = new foo_comparison_pass(g);
    
    // Insert after early optimizations but before IPA
    pass_info.reference_pass_name = "early_optimizations";
    pass_info.ref_pass_instance_number = 1;
    pass_info.pos_op = PASS_POS_INSERT_AFTER;
    
    register_callback(plugin_info->base_name,
                     PLUGIN_PASS_MANAGER_SETUP,
                     NULL,
                     &pass_info);
    
    return 0;
}