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

// Safe function name extraction
static const char* get_function_name(tree decl) {
    return (decl && DECL_NAME(decl)) ? IDENTIFIER_POINTER(DECL_NAME(decl)) : "<unnamed>";
}

// Check if type is complete and valid for inspection
static bool is_complete_type(tree type) {
    if (!type || !TYPE_P(type)) return false;
    if (CLASS_TYPE_P(type) && !COMPLETE_TYPE_P(type)) return false;
    return true;
}

// Check if a function matches our target pattern
static bool is_target_function(tree decl) {
    if (!decl || TREE_CODE(decl) != FUNCTION_DECL) return false;
    
    const char* name = get_function_name(decl);
    return name && strcmp(name, "AAAAAAA") == 0;
}

// Safely walk class hierarchy (handles templates and incomplete types)
static void walk_class_hierarchy(tree type, hash_set<tree>& found_functions) {
    if (!is_complete_type(type)) return;

    // Check current class methods
    for (tree method = TYPE_FIELDS(type); method; method = TREE_CHAIN(method)) {
        if (is_target_function(method)) {
            found_functions.add(method);
        }
    }

    // Handle template classes
    if (CLASSTYPE_TEMPLATE_INFO(type)) {
        tree template_decl = CLASSTYPE_TI_TEMPLATE(type);
        if (template_decl) {
            tree pattern = DECL_TEMPLATE_RESULT(template_decl);
            if (pattern && pattern != type) {
                walk_class_hierarchy(TREE_TYPE(pattern), found_functions);
            }
        }
    }

    // Walk base classes carefully
    if (TYPE_BINFO(type)) {
        for (tree binfo = TYPE_BINFO(type); binfo; binfo = TREE_CHAIN(binfo)) {
            tree base_type = BINFO_TYPE(binfo);
            if (base_type && base_type != type && is_complete_type(base_type)) {
                walk_class_hierarchy(base_type, found_functions);
            }
        }
    }
}

// Main pass implementation
class member_function_pass : public gimple_opt_pass {
public:
    member_function_pass(gcc::context *ctxt) : gimple_opt_pass(pass_data, ctxt) {}
    
    unsigned int execute(function *fun) override {
        hash_set<tree> target_functions;
        std::cerr << "WHAT THE FUCK" << std::endl;
        // Walk all types in compilation unit
        varpool_node *var;
        FOR_EACH_VARIABLE(var) {
            // if (TREE_CODE() == TYPE_DECL && is_complete_type(TREE_TYPE(var)))) {
            //     walk_class_hierarchy(TREE_TYPE(var), target_functions);
            // }
            if (var->type == SYMTAB_VARIABLE) {
                tree decl = var->decl;
                if (decl && TREE_CODE(decl) == TYPE_DECL) {
                    tree type = TREE_TYPE(decl);
                    if (is_complete_get_decltype(type)) {
                        walk_class_hierarchy(type, target_functions);
                    }
                }
            }
        }
        
        // Report found functions
        for (hash_set<tree>::iterator it = target_functions.begin();
             it != target_functions.end(); ++it) {
            tree fn = *it;
            warning_at(DECL_SOURCE_LOCATION(fn), 0, 
                     "Found target function: %qD in %qT",
                     fn, DECL_CONTEXT(fn));
            
            // Print full signature
            debug_tree(fn); // GCC internal function to dump tree structure
        }
        
        return 0;
    }
    
    member_function_pass *clone() override { return this; }
    
private:
    static const pass_data pass_data;
};

const pass_data member_function_pass::pass_data = {
    GIMPLE_PASS,
    "member-function-detector",
    OPTGROUP_NONE,
    TV_NONE,
    PROP_gimple_any,
    0, 0, 0, 0
};

// Plugin initialization
int plugin_init(struct plugin_name_args *plugin_info,
               struct plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        error(G_("Plugin version mismatch"));
        return 1;
    }
    
    register_pass_info pass_info = {
        new member_function_pass(g),
        "ssa",
        1,
        PASS_POS_INSERT_AFTER
    };
    
    register_callback(plugin_info->base_name, 
                    PLUGIN_PASS_MANAGER_SETUP, 
                    NULL, 
                    &pass_info);
    return 0;
}