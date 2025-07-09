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
#include <langhooks.h>
#include <intl.h>
#include <iostream> 

int plugin_is_GPL_compatible;

// Helper function to get function name as string
static const char* get_function_name(tree decl) {
    if (DECL_NAME(decl)) {
        return IDENTIFIER_POINTER(DECL_NAME(decl));
    }
    return "<unnamed>";
}

// Enhanced signature comparison
static bool compare_foo_signatures(tree decl1, tree decl2) {
    if (TREE_CODE(decl1) != FUNCTION_DECL || TREE_CODE(decl2) != FUNCTION_DECL) {
        return false;
    }

    tree type1 = TREE_TYPE(decl1);
    tree type2 = TREE_TYPE(decl2);
    
    if (!lang_hooks.types_compatible_p(TREE_TYPE(type1), TREE_TYPE(type2))) {
        return false;
    }

    // Handle C++ member functions using DECL_CXX_METHOD_P
    if (FUNC_OR_METHOD_TYPE_P(decl1) || FUNC_OR_METHOD_TYPE_P(decl2)) {
        if (!same_type_p(DECL_CONTEXT(decl1), DECL_CONTEXT(decl2))) {
            return false;
        }
        if (TYPE_READONLY(type1) != TYPE_READONLY(type2) ||
            TYPE_VOLATILE(type1) != TYPE_VOLATILE(type2)) {
            return false;
        }
    }

    tree args1 = TYPE_ARG_TYPES(type1);
    tree args2 = TYPE_ARG_TYPES(type2);
    
    while (args1 && args2 && args1 != void_list_node && args2 != void_list_node) {
        if (!lang_hooks.types_compatible_p(TREE_VALUE(args1), TREE_VALUE(args2))) {
            return false;
        }
        args1 = TREE_CHAIN(args1);
        args2 = TREE_CHAIN(args2);
    }
    
    if ((args1 && args1 != void_list_node) || (args2 && args2 != void_list_node)) {
        return false;
    }

    if (flag_exceptions) {
        tree raises1 = TYPE_RAISES_EXCEPTIONS(type1);
        tree raises2 = TYPE_RAISES_EXCEPTIONS(type2);
        
        if (!comp_except_specs(raises1, raises2, /*exact=*/false)) {
            return false;
        }
    }

    return true;
}

void walk_functions(tree current_decl = NULL) {
    if (current_decl) 
        std::cerr << "We're at function: " << cxx_printable_name(current_decl, 31) << std::endl;
    else 
        std::cerr << "No current function, walking all functions." << std::endl;
    std::cerr << "\tAll functions in the code:" << std::endl;
    cgraph_node *node;
    FOR_EACH_FUNCTION(node) {
        std::cerr << "\t\t\t " << cxx_printable_name(node->decl, 31) << std::endl;
    }
}

// Our pass implementation
class foo_comparison_pass : public gimple_opt_pass {
public:
    foo_comparison_pass(gcc::context *ctxt)
        : gimple_opt_pass(pass_data, ctxt) {}
    
    virtual unsigned int execute(function *fun) override {
        tree current_decl = fun->decl;
        const char* name = get_function_name(current_decl);
        
        walk_functions();
        
        if (name && strcmp(name, "foo") == 0) {
            // Compare with all other functions
            cgraph_node *node;
            FOR_EACH_FUNCTION(node) {
                tree other_decl = node->decl;
                if (other_decl != current_decl && 
                    TREE_CODE(other_decl) == FUNCTION_DECL) {
                    const char* other_name = get_function_name(other_decl);
                    if (other_name && strcmp(other_name, "foo") == 0) {
                        if (compare_foo_signatures(current_decl, other_decl)) {
                            warning_at(DECL_SOURCE_LOCATION(current_decl), 0, 
                                     "Duplicate foo signature detected");
                            warning_at(DECL_SOURCE_LOCATION(other_decl), 0, 
                                     "Previous declaration here");
                        }
                    }
                }
            }
        }
        return 0;
    }
    
    /* opt_pass methods: */
    virtual foo_comparison_pass *clone() { return this; }
    
private:
    static const pass_data pass_data;
};

const pass_data foo_comparison_pass::pass_data = {
    GIMPLE_PASS,
    "foo-comparison",
    OPTGROUP_NONE,
    TV_NONE,
    PROP_gimple_any,
    0,
    0,
    0,
    0
};

void cb_finish_unit(void *gcc_data, void *user_data) {
    walk_functions();
    
}

// Plugin initialization
int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        error(G_("Plugin version mismatch"));
        return 1;
    }
    
    struct register_pass_info pass_info;
    pass_info.pass = new foo_comparison_pass(g);
    pass_info.reference_pass_name = "ssa";
    pass_info.ref_pass_instance_number = 1;
    pass_info.pos_op = PASS_POS_INSERT_AFTER;
    
    register_callback(plugin_info->base_name,
                     PLUGIN_PASS_MANAGER_SETUP,
                     NULL,
                     &pass_info);
    
    register_callback(plugin_info->base_name,
                     PLUGIN_FINISH_UNIT,
                     cb_finish_unit,
                     NULL);


    return 0;
}