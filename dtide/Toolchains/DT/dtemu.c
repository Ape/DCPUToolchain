#include "dtemu.h"
#include <GL/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bstring.h>
#include <argtable2.h>
#include <dcpu.h>
#include <osutil.h>
#include <version.h>
#include <ldata.h>
#include <debug.h>
#include <iio.h>

// hardware
#include <hwsped3.h>
#include <hwlem1802.h>
#include <hwlem1802mem.h>
#include <hwtimer.h>
#include <hwm35fd.h>
#include <hwlua.h>
#include <hw.h>

static bool has_initialized_glfw3 = false;
static vm_t* vm = NULL;

bool start_emulation(const char* filename)
{
    FILE* load;
    uint16_t flash[0x10000];
    char leading[0x100];
    unsigned int i;
    bstring ss, st;

    // Stop any existing emulation.
    if (vm != NULL)
        stop_emulation();
    
    // Zero out the flash space.
    for (i = 0; i < 0x10000; i++)
        flash[i] = 0x0;

    // Zero out the leading space.
    for (i = 0; i < 0x100; i++)
        leading[i] = 0x0;

    // Open file.
    load = fopen(filename, "rb");
    if (load == NULL)
    {
	    fprintf(stderr, "emulator: unable to load %s from disk.\n", filename);
        return false;
    }

    // Read up to 0x10000 words.
    for (i = 0; i < 0x10000 && !feof(load); i++)
        iread(&flash[i], load);
    fclose(load);

    // Check to see if the first X bytes matches the header
    // for intermediate code and stop if it does.
    ss = bfromcstr("");
    st = bfromcstr(ldata_objfmt);
    for (i = 0; i < strlen(ldata_objfmt); i++)
        bconchar(ss, leading[i]);
    if (biseq(ss, st))
    {
        fprintf(stderr, "emulator: it appears you passed intermediate code for execution.  link\n");
        fprintf(stderr, "	the input code with the toolchain linker to execute it.\n");
        return false;
    }

    // And then use the VM.
    vm = vm_create();
    vm_flash(vm, flash);

    // Init hardware.
    vm_hw_timer_init(vm);
    vm_hw_lem1802_init(vm);
    vm_hw_sped3_init(vm);
    vm_hw_m35fd_init(vm);
    vm_hw_lua_init(vm);
    return true;
}

void cycle_emulation()
{
    if (!has_initialized_glfw3)
    {
        glfwInit();
        has_initialized_glfw3 = true;
    }

    if (vm != NULL)
    {
	    vm_cycle(vm);

        if (vm->halted)
        {
            // Stop the emulation.
            stop_emulation();
        }
    }
}

void stop_emulation()
{
    vm_hw_lua_free(vm);
    vm_hw_free_all(vm);
    vm_free(vm);
    vm = NULL;
    glfwTerminate();
    has_initialized_glfw3 = false;
}