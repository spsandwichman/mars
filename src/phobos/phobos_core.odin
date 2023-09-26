package phobos

import co "../common"
import "core:os"
import "core:mem"
import "core:strings"
import "core:fmt"
import "core:path/filepath"

// mars compiler frontend - lexer, parser
// produces abstract syntax tree to be passed to deimos backend

//         lexer            parser         validator
// files --------> tokens ---------> AST ------------> AST

phobos_build_state : co.build_state

// build the complete AST of the program, spanning multiple files and modules.
construct_complete_AST :: proc() -> ^program_tree {

    // lots of checks
    if !os.exists(phobos_build_state.compile_directory) || 
       !os.is_dir(phobos_build_state.compile_directory) {
            fmt.printf("ERROR Directory \"%s\" does not exist or is not a directory.\n", phobos_build_state.compile_directory)
            os.exit(1)
    }
    compile_directory_handle, open_dir_ok := os.open(phobos_build_state.compile_directory)
    if open_dir_ok != os.ERROR_NONE {
        fmt.printf("ERROR Directory \"%s\" cannot be opened.\n", phobos_build_state.compile_directory)
        os.exit(1)
    }
    compile_directory_files, _ := os.read_dir(compile_directory_handle, 100)
    if len(compile_directory_files) < 1 {
        fmt.printf("ERROR Directory \"%s\" is empty.\n", phobos_build_state.compile_directory)
        os.exit(1)
    }
    {
        mars_files := 0
        for file in compile_directory_files {
            mars_files += int(filepath.ext(file.fullpath) == ".mars")
        }
        if mars_files < 1 {
            fmt.printf("ERROR Directory \"%s\" has no .mars files.\n", phobos_build_state.compile_directory)
            os.exit(1)
        }
    }

    // lex all the module files
    lexers := make([dynamic]^lexer)
    for file in compile_directory_files {
        if file.is_dir || filepath.ext(file.fullpath) != ".mars" {
            continue
        }

        file_source, _ := os.read_entire_file(file.fullpath)

        this_lexer := new(lexer)

        relative_path, _ := filepath.rel(phobos_build_state.compile_directory, file.fullpath)

        #no_bounds_check {
            lexer_init(this_lexer, relative_path, cast(string) file_source)
            construct_token_buffer(this_lexer)
        }
        append(&lexers, this_lexer)
    }


    // parse all the module files
    main_module_ast := new(module)
    for file_lexer in lexers {

        // this_file_ast := parse_file(file_lexer, main_module_ast)

        // append(&(main_module_ast.files), this_file_ast)
    }

    return nil
}

construct_token_buffer :: proc(ctx: ^lexer) {

    int_token : lexer_token
    for int_token.kind != .EOF {
        int_token = lex_next_token(ctx)
        append(&ctx.buffer, int_token)
    }

    shrink(&ctx.buffer)
}

TODO :: co.TODO