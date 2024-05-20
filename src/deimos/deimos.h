#pragma once
#define DEIMOS_H

#include "mars.h"
#include "ir.h"
#include "phobos.h"
#include "pass.h"

char* random_string(int len);

IR_Module* ir_generate(mars_module* mod);

void deimos_run(mars_module* main_mod, IR_Module* passthrough);