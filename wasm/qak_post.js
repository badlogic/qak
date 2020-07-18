
    out = (str) => { console.log(str); }
    err = (str) => { console.log(str); }
    qak.Module = Module;
})(qak);

qak.init = function(onReady) {
    qak.Module.onRuntimeInitialized = _ => {
        const Module = qak.Module;
        const qak_version = Module.cwrap("qak_version", "number", []);
        const qak_compiler_new = Module.cwrap("qak_compiler_new", "ptr", []);
        const qak_compiler_delete = Module.cwrap("qak_compiler_delete", "void", ["ptr"]);
        const qak_compile_source = Module.cwrap("qak_compile_source", "ptr", ["ptr", "ptr", "ptr"]);
        const qak_module_delete = Module.cwrap("qak_module_delete", "void", ["ptr"]);
        const qak_module_has_errors = Module.cwrap("qak_module_has_errors", "number", ["ptr"]);
        const qak_module_print_errors = Module.cwrap("qak_module_print_errors", "void", ["ptr"]);
        const qak_module_print_tokens = Module.cwrap("qak_module_print_tokens", "void", ["ptr"]);
        const qak_module_print_ast = Module.cwrap("qak_module_print_ast", "void", ["ptr"]);

        qak.compiler = qak_compiler_new();

        qak.compileSource = (sourceName, sourceData) => {
            var name = Module.allocateUTF8(sourceName);
            var data = Module.allocateUTF8(sourceData);
            var module = qak_compile_source(qak.compiler, name, data);
            return module;
        };

        qak.deleteModule = (module) => {
            qak_module_delete(module);
        }

        qak.hasErrors = (module) => {
            return qak_module_has_errors(module) != 0;
        }

        qak.printErrors = (module) => {
            qak_module_print_errors(module);
        }

        qak.printTokens = (module) => {
            qak_module_print_tokens(module);
        }

        qak.printAst = (module) => {
            qak_module_print_ast(module);
        }

        if (onReady) onReady();
    };
}