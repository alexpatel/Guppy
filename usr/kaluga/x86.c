/**
 * \file
 * \brief Device manager for Barrelfish.
 *
 * Interacts with the SKB / PCI to start cores, drivers etc.
 *
 * x86 specific startup code
 *
 */

/*
 * Copyright (c) 2007-2010, 2016 ETH Zurich.
 * Copyright (c) 2015, Hewlett Packard Enterprise Development LP.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <barrelfish/barrelfish.h>
#include <skb/skb.h>
#include "kaluga.h"

static errval_t start_serial(void){
    errval_t err;
    struct module_info * mi = find_module("serial_pc16550d");
    if (mi != NULL) {
        // Get internal int number. COM1 uses ISA nr. 4
        struct driver_argument arg;
        err = skb_execute("isa_irq_to_int(4,N), writeln(N).");
        if(err_is_fail(err)){
            DEBUG_SKB_ERR(err, "skb_execute");
            return err;
        }
        int int_nr;
        err = skb_read_output("%d", &int_nr);
        if(err_is_fail(err)){
            DEBUG_ERR(err, "skb_read_output");
            return err;
        }
        printf("Found internal int number (%d) for serial.\n", int_nr);
        arg.int_arg.int_range_start = int_nr;
        arg.int_arg.int_range_end = int_nr;
        arg.int_arg.model = INT_MODEL_LEGACY;

        struct cnoderef argnode_ref;
        err = cnode_create_l2(&arg.arg_caps, &argnode_ref);
        if(err_is_fail(err)){
            DEBUG_ERR(err, "cnode_create_l2");
            return err;
        }

        // Build irq src cap
        struct capref cap;
        cap.cnode = argnode_ref;
        cap.slot = 0;
        err = sys_debug_create_irq_src_cap(cap, int_nr);

        if(err_is_fail(err)){
            DEBUG_ERR(err, "Could not create int_src cap");
            return err;
        }
        err = mi->start_function(0, mi, "hw.legacy.uart.1 {}", &arg);
        if(err_is_fail(err)){
            USER_PANIC_ERR(err, "serial->start_function");
        }
    } else {
        KALUGA_DEBUG("Kaluga: Not starting serial, binary not found\n");
    }
    return SYS_ERR_OK;
}

errval_t arch_startup(char * add_device_db_file)
{
    errval_t err = SYS_ERR_OK;
    // We need to run on core 0
    // (we are responsible for booting all the other cores)
    assert(my_core_id == BSP_CORE_ID);
    KALUGA_DEBUG("Kaluga running on x86.\n");

    err = skb_client_connect();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Connect to SKB.");
    }

    // Make sure the driver db is loaded
    err = skb_execute("[device_db].");
    if (err_is_fail(err)) {
        USER_PANIC_SKB_ERR(err, "Device DB not loaded.");
    }
    if(add_device_db_file != NULL){
        err = skb_execute_query("[%s].", add_device_db_file);
        if(err_is_fail(err)){
            USER_PANIC_SKB_ERR(err,"Additional device db file %s not loaded.", add_device_db_file);
        }
    }

    // The current boot protocol needs us to have
    // knowledge about how many CPUs are available at boot
    // time in order to start-up properly.
    char* record = NULL;
    err = oct_barrier_enter("barrier.acpi", &record, 2);

    KALUGA_DEBUG("Kaluga: watch_for_cores\n");

    err = watch_for_cores();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Watching cores.");
    }

    KALUGA_DEBUG("Kaluga: pci_root_bridge\n");

    err = watch_for_pci_root_bridge();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Watching PCI root bridges.");
    }

    KALUGA_DEBUG("Kaluga: pci_devices\n");

    err = watch_for_pci_devices();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Watching PCI devices.");
    }

    KALUGA_DEBUG("Kaluga: int_controller_devices\n");

    err = watch_for_int_controller();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Watching interrupt controllers.");
    }

    KALUGA_DEBUG("Kaluga: wait_for_all_spawnds\n");

    err = wait_for_all_spawnds();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Unable to wait for spawnds failed.");
    }

    KALUGA_DEBUG("Kaluga: Starting serial...\n");
    err = start_serial();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "start_serial");
    }

    return err;
}
