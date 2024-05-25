#pragma once
#define DEIMOS_H

#include "mars.h"
#include "ir.h"
#include "phobos.h"
#include "pass.h"

char* random_string(int len);

IR_Module* ir_generate(MarsModule* mod);

void deimos_run(MarsModule* main_mod);