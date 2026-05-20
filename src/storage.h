#pragma once
#include "config.h"

void storage_init();

// Machine config (NVS)
void storage_load_config(MachineConfig &cfg);
void storage_save_config(const MachineConfig &cfg);

// Winding programs (NVS, up to 8 slots)
#define MAX_PROGRAMS 8
int  storage_program_count();
bool storage_load_program(int slot, WindingProgram &prog);
bool storage_save_program(int slot, const WindingProgram &prog);
bool storage_delete_program(int slot);
void storage_get_program_names(char names[][32], int &count);

// WiFi config (NVS)
void storage_load_wifi(WiFiConfig &cfg);
void storage_save_wifi(const WiFiConfig &cfg);
