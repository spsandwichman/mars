#include "orbit.h"
#include "term.h"
#include "ast.h"
#include "dot.h"

void emit_dot(string path, AST node) {
	char* path_cstr = clone_to_cstring(path);
	char buffer[1050];
	sprintf(buffer, "%.1024s.dot", path_cstr); //capped at 1024 characters for file name
	free(path_cstr);

	string filename = to_string(buffer);
	if (fs_exists(filename)) {
		//we delete the file and recreate a new one
		fs_file file;
		fs_get(filename, &file);
		if (fs_delete(&file) == 0) general_error("Failed to delete file: \"%s\"", filename);
	}
	//the file does not exist
	fs_file file;
	fs_create(filename, oft_regular, &file);
	fs_open(&file, "w");
	printf("emitting dot file: \"%s\"\n", filename.raw);
	char* first_line = "digraph G {\n";
	fs_write(&file, first_line, strlen(first_line));
	recurse_dot(node, &file, 1, 0);

	fs_write(&file, "}", 1);
	fs_drop(&file);
}

int _dot_uID = 0;

int dot_uID() {
	return ++_dot_uID;
}

void recurse_dot(AST node, fs_file* file, int n, int uid) {
	//this code keeps track of depth with n, in which case it prints that many tabs.
	//all nodes have label sugar to allow them to change names to un-mutated names.
	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	char buffer[4096];

	if (is_null_AST(node)) {
		printf("encountered null node at uid %d\n", uid);
		sprintf(buffer, "\"%s_%d\" [shape=diamond, style=filled, label=\"%s\", color=\"1.0 .7 .7\"]\n", 
				ast_type_str[node.type], uid, 
				"NULL\\nAST"); //invalid sugar
	    fs_write(file, buffer, strlen(buffer));
		return;
	}

	
	switch (node.type) {
		case astype_invalid:
			return;
		case astype_identifier_expr: {
	        sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=\".7 .3 1.0\", label=\"%s\"]\n", 
	        		clone_to_cstring(node.as_identifier_expr->tok->text), 
	        		_dot_uID + 1, 
	        		clone_to_cstring(node.as_identifier_expr->tok->text)); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        sprintf(buffer, "\"%s_%d\" [label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier with sugared name
	    	fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"", 
	        		ast_type_str[node.type], uid, 
	        		clone_to_cstring(node.as_identifier_expr->tok->text), _dot_uID + 1); //print node link
	        fs_write(file, buffer, strlen(buffer));
	        break;
		}

		case astype_decl_stmt: {
	    	char* decl_type = "";
	        if (node.as_decl_stmt->is_mut) 
	        	decl_type = "mut decl";
	        else 
	        	decl_type = "let decl"; //determine name

	        sprintf(buffer, "\"%s_%d\" [label=\"%s\"]\n", "declaration", uid, decl_type); //decl sugar
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        FOR_URANGE(i, 0, node.as_decl_stmt->lhs.len) { //iterate over function itself
	        	int int_uid = dot_uID();
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", "declaration", uid, ast_type_str[node.as_decl_stmt->lhs.at[i].type], int_uid);
	            fs_write(file, buffer, strlen(buffer));
	            recurse_dot(node.as_decl_stmt->lhs.at[i], file, n+1, int_uid);
	        }
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		"declaration", uid, 
	        		ast_type_str[node.as_decl_stmt->type.type], _dot_uID + 1); //link
	        fs_write(file, buffer, strlen(buffer));
	        recurse_dot(node.as_decl_stmt->type, file, n+1, dot_uID()); //print type info

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		"declaration", uid, 
	        		ast_type_str[node.as_decl_stmt->rhs.type], _dot_uID + 1); //print links
	        fs_write(file, buffer, strlen(buffer));
	        recurse_dot(node.as_decl_stmt->rhs, file, n+1, dot_uID());
			break;
		}

		case astype_func_literal_expr: {
	        sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=green, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs			

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_func_literal_expr->type.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	    	recurse_dot(node.as_func_literal_expr->type, file, n+1, dot_uID());
	    	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_func_literal_expr->code_block.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	    	recurse_dot(node.as_func_literal_expr->code_block, file, n+1, dot_uID());
	    	break;
	    }

		case astype_fn_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=green, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer));
			for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1); //fix tabs	
	        
	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "label=parameters\n");
	        fs_write(file, buffer, strlen(buffer));

	        FOR_URANGE(i, 0, node.as_fn_type_expr->parameters.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n+1); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_fn_type_expr->parameters.at[i].type.type], int_uid);
	            fs_write(file, buffer, strlen(buffer));
	        	recurse_dot(node.as_fn_type_expr->parameters.at[i].type, file, n+2, int_uid);
	        }



	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}\n", 2);		
	    	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer));
			for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1); //fix tabs	
	        
	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "label=returns\n");
	        fs_write(file, buffer, strlen(buffer));


	    	FOR_URANGE(i, 0, node.as_fn_type_expr->returns.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n+1); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_fn_type_expr->returns.at[i].type.type], int_uid);
	            fs_write(file, buffer, strlen(buffer));
	        	recurse_dot(node.as_fn_type_expr->returns.at[i].type, file, n+2, int_uid);
	        }

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}", 1);	
			break;
		}

		case astype_basic_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=pink, label=\"%s\"]", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

			sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"", 
					ast_type_str[node.type], uid, 
					clone_to_cstring(node.as_basic_type_expr->lit->text), _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" [shape=house,label=\"%s\"]", 
	        		clone_to_cstring(node.as_basic_type_expr->lit->text), _dot_uID + 1,
	        		clone_to_cstring(node.as_basic_type_expr->lit->text));
	        fs_write(file, buffer, strlen(buffer));
	    	
	    	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	    	break;
		}

		case astype_block_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        FOR_URANGE(i, 0, node.as_block_stmt->stmts.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_block_stmt->stmts.at[i].type], int_uid);
	            fs_write(file, buffer, strlen(buffer)); 
	        	recurse_dot(node.as_block_stmt->stmts.at[i], file, n+1, int_uid);
	        }
	        break;
		}

		case astype_cast_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	
	    	
	        int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_cast_expr->type.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_cast_expr->rhs.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_cast_expr->type, file, n+1, int_uid);
	    	recurse_dot(node.as_cast_expr->rhs, file, n+1, int_uid + 1);

			break;
		}

		case astype_pointer_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=pink, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_pointer_type_expr->subexpr.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_pointer_type_expr->subexpr, file, n+1, int_uid);

	        break;
		}

		case astype_binary_op_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"binary op %s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		clone_to_cstring(node.as_binary_op_expr->op->text)); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_binary_op_expr->lhs.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_binary_op_expr->rhs.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_binary_op_expr->lhs, file, n+1, int_uid);
	    	recurse_dot(node.as_binary_op_expr->rhs, file, n+1, int_uid + 1);

			break;
		}

		case astype_while_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_while_stmt->condition.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_while_stmt->block.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "label=condition\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "color=red\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer));         	

	        recurse_dot(node.as_while_stmt->condition, file, n+1, int_uid);

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}\n", 2);

	    	recurse_dot(node.as_while_stmt->block, file, n+1, int_uid + 1);

	        break;
		}

		case astype_assign_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        FOR_URANGE(i, 0, node.as_block_stmt->stmts.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_assign_stmt->lhs.at[i].type], int_uid);
	            fs_write(file, buffer, strlen(buffer)); 
	        	recurse_dot(node.as_assign_stmt->lhs.at[i], file, n+1, int_uid);
	        }

	        int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_assign_stmt->rhs.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 

	        recurse_dot(node.as_assign_stmt->rhs, file, n+1, int_uid);

	        break;
		}

		case astype_comp_assign_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"assignment op %s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		clone_to_cstring(node.as_comp_assign_stmt->op->text)); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_comp_assign_stmt->lhs.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_comp_assign_stmt->rhs.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_comp_assign_stmt->lhs, file, n+1, int_uid);
	    	recurse_dot(node.as_comp_assign_stmt->rhs, file, n+1, int_uid + 1);

	    	break;
		}

		case astype_unary_op_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"unary op %s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		clone_to_cstring(node.as_unary_op_expr->op->text)); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	       	int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_unary_op_expr->inside.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_unary_op_expr->inside, file, n+1, int_uid);

	        break;
		}

		case astype_literal_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        char* literal_name = "invalid literal type";

	    	char literal_buffer[2048];

	        //determine what type of literal, and then emit the correct node.
	        switch (node.as_literal_expr->value.kind) {
	        	case ev_bool: {
	        		literal_name = "bool";
	        		sprintf(literal_buffer, "%s", node.as_literal_expr->value.as_bool == true ? "true" : "false");
	        		break;
	        	}
	        	case ev_string: {
	        		literal_name = "string";
	        		sprintf(literal_buffer, "%.2047s", clone_to_cstring(node.as_literal_expr->value.as_string));
	        		break;
	        	}
	        	case ev_int: {
	        		literal_name = "int";
	        		sprintf(literal_buffer, "%d", node.as_literal_expr->value.as_int);
	        		break;
	        	}
	        	case ev_float: {
	        		literal_name = "float";
	        		sprintf(literal_buffer, "%lf", node.as_literal_expr->value.as_float);
	        		break;
	        	}
	        	case ev_pointer: {
	        		literal_name = "pointer";
	        		sprintf(literal_buffer, "null");
	        		break;
	        	}
	        	default:
	        }

	        int int_uid = dot_uID();

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		literal_name, int_uid);
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1); 
	       	sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue,label=\"%s\\n%s\"]\n", 
	        		literal_name, int_uid, 
	        		literal_name, literal_buffer);
	        fs_write(file, buffer, strlen(buffer));

	       	break;
		}

		default: {
			sprintf(buffer, "\"unimpl_%d\"->\"unimpl_%d\" unimpl_%d [shape=star,style=filled,color=yellow]", uid, uid, uid);
			fs_write(file, buffer, strlen(buffer));
			
			if (node.type < astype_COUNT) {
				printf("encountered unimplemented node: %s\n", ast_type_str[node.type]);
			} else {
				printf("unimpl: %d\n", node.type);
			}
			
			break;
		}

	}

	fs_write(file, "\n", 1);
}
/*
void recurse_dot(AST node, fs_file* file, int n, int uid) {
	//we need to generate all the edges for THIS node and then move on
	//this has no cycle detection, so will halt and catch fire if there is a cyclical graph passed in
	
	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	if (is_null_AST(node)) {
        //char* str = "null -> null\n";
        //fs_write(file, str, strlen(str));
        return;
    }

    char* str = "\n";

    char buffer[4096];

    switch (node.type) {
	    case astype_invalid:
	    	return;
	    case astype_identifier_expr:
	        {
	        	char* identifier_box_text = "[shape=box,style=filled,color=\".7 .3 1.0\"]";
	        	sprintf(buffer, "\"%s_%d\" %s\n", clone_to_cstring(node.as_identifier_expr->tok->text), _dot_uID + 1, identifier_box_text);
	        	fs_write(file, buffer, strlen(buffer));
	        	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, clone_to_cstring(node.as_identifier_expr->tok->text), _dot_uID + 1);
	        	fs_write(file, buffer, strlen(buffer));
	        	break;
	    	}
	    case astype_literal_expr:
	        switch (node.as_literal_expr->value.kind) {
	        //case ev_bool: printf("bool literal\n"); break;
	        //case ev_float: printf("float literal\n"); break;
	        //case ev_int: printf("int literal\n"); break;
	        //case ev_pointer: printf("pointer literal\n"); break;
	        //case ev_string: printf("string literal\n"); break;
	        //default: printf("[invalid] literal\n"); break;
	        }
	        break;
	    case astype_comp_literal_expr:
	        //printf("compound literal\n");
	        {
	        	char buffer[1024];

	        	recurse_dot(node.as_comp_literal_expr->type, file, n+1, dot_uID());
	        	FOR_URANGE(i, 0, node.as_comp_literal_expr->elems.len) {
	        	    recurse_dot(node.as_comp_literal_expr->elems.at[i], file, n+1, dot_uID());
	        	}
	    	}
	        break;
	    case astype_paren_expr:
	        //printf("()\n");
	        recurse_dot(node.as_paren_expr->subexpr, file, n+1, dot_uID());
	        break;
	    case astype_cast_expr:
	        //if (node.as_cast_expr->is_bitcast) printf("bitcast\n");
	        //else printf("cast\n");
	        recurse_dot(node.as_cast_expr->type, file, n+1, dot_uID());
	        recurse_dot(node.as_cast_expr->rhs, file, n+1, dot_uID());
	        break;
	    case astype_unary_op_expr:
	        //printf("unary %s\n", token_type_str[node.as_unary_op_expr->op->type]);
	        recurse_dot(node.as_unary_op_expr->inside, file, n+1, dot_uID());
	        break;
	    case astype_binary_op_expr:
	        //printf("binary %s\n", token_type_str[node.as_binary_op_expr->op->type]);
	        recurse_dot(node.as_binary_op_expr->lhs, file, n+1, dot_uID());
	        recurse_dot(node.as_binary_op_expr->rhs, file, n+1, dot_uID());
	        break;
	    case astype_entity_selector_expr:
	        //printf("entity selector\n");
	        recurse_dot(node.as_entity_selector_expr->lhs, file, n+1, dot_uID());
	        recurse_dot(node.as_entity_selector_expr->rhs, file, n+1, dot_uID());
	        break;
	    case astype_selector_expr:
	        //printf("selector\n");
	        recurse_dot(node.as_selector_expr->lhs, file, n+1, dot_uID());
	        recurse_dot(node.as_selector_expr->rhs, file, n+1, dot_uID());
	        break;
	    case astype_impl_selector_expr:
	        //printf("implicit selector\n");
	        recurse_dot(node.as_impl_selector_expr->rhs, file, n+1, dot_uID());
	        break;
	    case astype_index_expr:
	        //printf("index\n");
	        recurse_dot(node.as_index_expr->lhs, file, n+1, dot_uID());
	        recurse_dot(node.as_index_expr->inside, file, n+1, dot_uID());
	        break;
	    case astype_slice_expr:
	        //printf("slice\n");
	        recurse_dot(node.as_slice_expr->lhs, file, n+1, dot_uID());
	        recurse_dot(node.as_slice_expr->inside_left, file, n+1, dot_uID());
	        recurse_dot(node.as_slice_expr->inside_right, file, n+1, dot_uID());
	        break;
	    case astype_call_expr:
	        //printf("function call\n");
	        recurse_dot(node.as_call_expr->lhs, file, n+1, dot_uID());
	        FOR_URANGE(i, 0, node.as_call_expr->params.len) {
	            recurse_dot(node.as_call_expr->params.at[i], file, n+1, dot_uID());
	        }
	        break;
	    
	    case astype_module_decl:
	        //printf("module %s\n", clone_to_cstring(node.as_module_decl->name->text));
	        break;
	    case astype_import_stmt:
	        //printf("import\n");
	        recurse_dot(node.as_import_stmt->name, file, n+1, dot_uID());
	        recurse_dot(node.as_import_stmt->path, file, n+1, dot_uID());
	        break;
	    case astype_block_stmt:
	        //printf("{;}\n");
	        FOR_URANGE(i, 0, node.as_block_stmt->stmts.len) {
	            recurse_dot(node.as_block_stmt->stmts.at[i], file, n+1, dot_uID());
	        }
	        break;
	    case astype_decl_stmt:
	    	{
	    		char* decl_type = "";
	        	if (node.as_decl_stmt->is_mut) 
	        		decl_type = "mut decl";
	        	else 
	        	    decl_type = "let decl";
	        	FOR_URANGE(i, 0, node.as_decl_stmt->lhs.len) {
	        		int int_uid = dot_uID();
	        		sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", decl_type, uid, ast_type_str[node.as_decl_stmt->lhs.at[i].type], int_uid);
	        	    fs_write(file, buffer, strlen(buffer));
	        	    recurse_dot(node.as_decl_stmt->lhs.at[i], file, n+1, int_uid);
	        	}
	        	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", decl_type, uid, ast_type_str[node.as_decl_stmt->type.type], _dot_uID + 1);
	        	fs_write(file, buffer, strlen(buffer));
	        	recurse_dot(node.as_decl_stmt->type, file, n+1, dot_uID());
	        	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", decl_type, uid, ast_type_str[node.as_decl_stmt->rhs.type], _dot_uID + 1);
	        	fs_write(file, buffer, strlen(buffer));
	        	recurse_dot(node.as_decl_stmt->rhs, file, n+1, dot_uID());
	    	}
	        return;
	    case astype_type_decl_stmt:
	        //printf("type decl\n");
	        recurse_dot(node.as_type_decl_stmt->lhs, file, n+1, dot_uID());
	        recurse_dot(node.as_type_decl_stmt->rhs, file, n+1, dot_uID());
	        break;
	    case astype_struct_type_expr:
	        //printf("struct type expr\n");
	        FOR_URANGE(i, 0, node.as_struct_type_expr->fields.len) {
	            recurse_dot(node.as_struct_type_expr->fields.at[i].field, file, n+1, dot_uID());
	            recurse_dot(node.as_struct_type_expr->fields.at[i].type, file, n+1, dot_uID());
	        }
	        break;
	    case astype_union_type_expr:
	        //printf("union type expr\n");
	        FOR_URANGE(i, 0, node.as_struct_type_expr->fields.len) {
	            recurse_dot(node.as_struct_type_expr->fields.at[i].field, file, n+1, dot_uID());
	            recurse_dot(node.as_struct_type_expr->fields.at[i].type, file, n+1, dot_uID());
	        }
	        break;
	    case astype_enum_type_expr:
	        //printf("enum type expr\n");
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_enum_type_expr->backing_type.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	        recurse_dot(node.as_enum_type_expr->backing_type, file, n+1, dot_uID());
	        FOR_URANGE(i, 0, node.as_enum_type_expr->variants.len) {
	            sprintf(buffer, "\"%s_%d\" -> \"%s = %ld\"\n", ast_type_str[node.type], uid, clone_to_cstring((node.as_enum_type_expr->variants.at[i].ident.as_identifier_expr->tok->text)), node.as_enum_type_expr->variants.at[i].value);
	            fs_write(file, buffer, strlen(buffer));
	        }
	        break;
	    case astype_pointer_type_expr:
	        //printf("^ type expr\n");
	       	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_pointer_type_expr->subexpr.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	        recurse_dot(node.as_pointer_type_expr->subexpr, file, n+1, dot_uID());
	        break;
	    case astype_slice_type_expr:
	        //printf("[] type expr\n");
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_slice_type_expr->subexpr.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	        recurse_dot(node.as_slice_type_expr->subexpr, file, n+1, dot_uID());
	        break;
	    case astype_basic_type_expr:
	        //printstr(node.as_basic_type_expr->lit->text);
	        //printf("\n");
	        break;
	    case astype_func_literal_expr:
	    	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_func_literal_expr->type.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	    	recurse_dot(node.as_func_literal_expr->type, file, n+1, dot_uID());

	    	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_func_literal_expr->code_block.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	    	recurse_dot(node.as_func_literal_expr->code_block, file, n+1, dot_uID());
	    	break;

	    default:
	        str = "unimpl -> unimpl\n";
			printf("encountered unimplemented node: %s\n", ast_type_str[node.type]);
			break;
	}

	//fs_write(file, str, strlen(str));
}*/