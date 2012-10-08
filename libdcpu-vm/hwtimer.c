/**

	File:		hwio.c

	Project:	DCPU-16 Tools
	Component:	LibDCPU-vm

	Authors:	James Rhodes
			Jose Manuel Diez

	Description:	Hosts the virtual screen and keyboard for the
			emulator.

**/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <debug.h>
#include "hw.h"
#include "hwtimer.h"
#include "dcpu.h"
#include "dcpubase.h"
#include "dcpuhook.h"
#include "dcpuops.h"

void vm_hw_timer_cycle(vm_t* vm, uint16_t pos, void* ud)
{
	struct timer_hardware* hw = (struct timer_hardware*) ud;

	if (hw->message != 0)
	{
		if(hw->clock_ticks >= hw->clock_target)
		{
			vm_interrupt(vm, hw->message);
			hw->clock_ticks = 0;
		}
		else 
			hw->clock_ticks++;
	}
}

void vm_hw_timer_interrupt(vm_t* vm, void* ud)
{
	struct timer_hardware* hw = (struct timer_hardware*) ud;

	switch (vm->registers[REG_A])
	{
		case TIMER_SET_ENABLED:
			if (vm->registers[REG_B] == 0x0)
				break;
			else
			{
				if (vm->registers[REG_B] > 60) break;
				hw->clock_target = (DCPU_TICKS_KHZ * 1000 * vm->registers[REG_B]) / 60;
			}

			break;

		case TIMER_GET_ELAPSED:
			vm->registers[REG_C] = hw->clock_ticks;
			hw->clock_ticks = 0;
			break;

		case TIMER_SET_INTERRUPT:
			hw->message = vm->registers[REG_B];
			break;
	}
}

void vm_hw_timer_init(vm_t* vm)
{
	struct timer_hardware* hw;

	hw = malloc(sizeof(struct timer_hardware));
	hw->clock_target = 0;
	hw->clock_ticks = 0;
	hw->message = 0;

	hw->device.id = 0x12D0B402;
	hw->device.version = 0x0001;
	hw->device.manufacturer = 0x00000000;
	hw->device.handler = &vm_hw_timer_interrupt;
	hw->device.userdata = hw;

	hw->hook_id = vm_hook_register(vm, &vm_hw_timer_cycle, HOOK_ON_PRE_CYCLE, hw);
	hw->hw_id = vm_hw_register(vm, hw->device);
}	

void vm_hw_timer_free(void* ud)
{
	struct timer_hardware* hw = (struct timer_hardware*) ud;

	vm_hook_unregister(hw->vm, hw->hook_id);
	vm_hw_unregister(hw->vm, hw->hw_id);
}
