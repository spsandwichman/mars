#pragma once
#define PHOBOS_DOT_H

#include "orbit.h"
#include "ast.h"

void emit_dot(string path, da(Ast) nodes);
void recurse_dot(Ast node, fs_file* file, int n, int uID);