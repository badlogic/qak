
    out = (str) => { console.log(str); }
    err = (str) => { console.log(str); }
};

qak.init = function(onReady) {

    qak.wasmModule.onRuntimeInitialized = _ => {
        const Module = qak.wasmModule;
        const qak_print_struct_offsets = Module.cwrap("qak_print_struct_offsets", "void", []);
        const qak_compiler_new = Module.cwrap("qak_compiler_new", "ptr", []);
        const qak_compiler_delete = Module.cwrap("qak_compiler_delete", "void", ["ptr"]);
        const qak_compiler_print_memory_usage = Module.cwrap("qak_compiler_print_memory_usage", "void", ["ptr"]);
        const qak_compiler_compile_source = Module.cwrap("qak_compiler_compile_source", "ptr", ["ptr", "ptr", "ptr"]);
        const qak_module_delete = Module.cwrap("qak_module_delete", "void", ["ptr"]);
        const qak_module_get_source = Module.cwrap("qak_module_get_source", "void", ["ptr", "ptr"]);
        const qak_module_get_num_errors = Module.cwrap("qak_module_get_num_errors", "number", ["ptr"]);
        const qak_module_get_error = Module.cwrap("qak_module_get_error", "void", ["ptr", "number", "ptr"]);
        const qak_module_print_errors = Module.cwrap("qak_module_print_errors", "void", ["ptr"]);
        const qak_module_get_num_tokens = Module.cwrap("qak_module_get_num_tokens", "number", ["ptr"]);
        const qak_module_get_token = Module.cwrap("qak_module_get_token", "void", ["ptr", "number", "ptr"]);
        const qak_module_print_tokens = Module.cwrap("qak_module_print_tokens", "void", ["ptr"]);
        const qak_module_print_ast = Module.cwrap("qak_module_print_ast", "void", ["ptr"]);

        qak_print_struct_offsets();

        var convertQakString = (stringPtr) => {
            var dataPtr = Module.HEAPU32[stringPtr / 4];
            var dataLength = Module.HEAPU32[stringPtr / 4 + 1];
            return Module.UTF8ArrayToString(Module.HEAPU8, dataPtr, dataLength);
        }

        var convertQakSpan = (spanPtr) => {
            var data = convertQakString(spanPtr);
            spanPtrU32 = spanPtr / 4 + 2;
            var start = Module.HEAPU32[spanPtrU32];
            var end = Module.HEAPU32[spanPtrU32 + 1];
            var startLine = Module.HEAPU32[spanPtrU32 + 2];
            var endLine = Module.HEAPU32[spanPtrU32 + 3];
            return {
                data: data,
                start: start,
                end: end,
                startLine: startLine,
                endLine: endLine
            }
        }

        qak.compiler = qak_compiler_new();

        qak.printMemoryUsage = () => {
            qak_compiler_print_memory_usage(qak.compiler);
        }

        qak.compileSource = (sourceName, sourceData) => {
            var name = Module.allocateUTF8(sourceName);
            var data = Module.allocateUTF8(sourceData);
            var module = qak_compiler_compile_source(qak.compiler, name, data);
            return module;
        };

        qak.deleteModule = (module) => {
            qak_module_delete(module);
        }

        qak.getSource = (module) => {
            var nativeSource = Module._malloc(16);
            qak_module_get_source(module, nativeSource);
            var source = {
                data: convertQakString(nativeSource),
                fileName: convertQakString(nativeSource + 8)
            }
            Module._free(nativeSource);
            return source;
        }

        qak.hasErrors = (module) => {
            return qak_module_get_num_errors(module) != 0;
        }

        qak.getErrors = (module) => {
            var numErrors = qak_module_get_num_errors(module);
            var errors = new Array(numErrors);
            var nativeError = Module._malloc(32);
            for (var i = 0; i < numErrors; i++) {
                qak_module_get_error(module, i, nativeError);
                var errorMessage = convertQakString(nativeError);
                var span = convertQakSpan(nativeError + 8);
                errors[i] = {
                    errorMessage: errorMessage,
                    span: span
                }
            }
            Module._free(nativeError);
            return errors;
        }

        qak.printErrors = (module) => {
            qak_module_print_errors(module);
        }

        qak.getTokens = (module) => {
            var numTokens = qak_module_get_num_tokens(module);
            var tokens = new Array(numTokens);
            var nativeToken = Module._malloc(28);
            var nativeTokenU32 = nativeToken / 4;
            for (var i = 0; i < numTokens; i++) {
                qak_module_get_token(module, i, nativeToken);
                var type = Module.HEAPU32[nativeTokenU32];
                var span = convertQakSpan(nativeToken + 4);
                tokens[i] = {
                    type: type,
                    span: span
                }
            }
            Module._free(nativeToken);
            return tokens;
        }

        qak.printTokens = (module) => {
            qak_module_print_tokens(module);
        }

        qak.printAst = (module) => {
            qak_module_print_ast(module);
        }

        if (onReady) onReady();
    };

    qak.initWasm();
}