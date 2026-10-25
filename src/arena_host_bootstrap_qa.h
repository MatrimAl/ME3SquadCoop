#ifndef ME3_ARENA_HOST_BOOTSTRAP_QA_H
#define ME3_ARENA_HOST_BOOTSTRAP_QA_H
/* Protected parallel arena only. Initialize both worlds as standalone so
 * normal DLC streaming selects the same package carriers, then reuse the
 * ordinary runtime's checked in-place Listen without reloading the host. */
#include "runtime_transport.h"
static Me3RuntimeConfig g_runtime_config;
static Me3RuntimeConfigStatus g_config_status=ME3_CONFIG_DISABLED;
static volatile LONG g_enable_requested=1,runtime_host_start_gate;
static int runtime_control_enabled;
#include "runtime_network_bootstrap.h"
static int arena_host_listen_requested,arena_host_listen_reported;
static void arena_host_bootstrap_tick(uintptr_t base,uintptr_t engine,SessionContext c,int ready,DWORD now) {
    if(!arena_pair_qa||!arena_pair_auto_qa||!arena_parallel_qa||test_action!=6||!has_marker(root))return;
    if(!arena_host_listen_requested) {
        if(!ready||session_active||!arena_pair_world(base)||!runtime_host_story_idle(base,c))return;
        CombatContextChain chain;combat_context_read_chain(base,&chain);
        if(!chain.info_valid||chain.info!=c.world||!runtime_join_travel_idle(chain.world,engine))return;
        g_runtime_config.role=ME3_ROLE_HOST;g_runtime_config.endpoint=direct_ip_config;
        g_config_status=ME3_CONFIG_READY;runtime_control_enabled=1;
        arena_host_listen_requested=1;
        identity_snapshot(base,"arena-host-standalone-prewarmed");
        if(!runtime_request_listen()) {
            line("{\"arena_auto_qa\":\"in-place-listen-request-refused\"}\r\n");return;
        }
    }
    runtime_network_start_tick(base,engine,now);
    if(!arena_host_listen_reported&&runtime_listen_attempted) {
        arena_host_listen_reported=1;
        host_map_attempted=1;
        host_map_refreshed=runtime_network_start_status==RUNTIME_START_LISTENING&&runtime_host_start_gate;
        if(host_map_refreshed) {
            CombatContextChain chain;combat_context_read_chain(base,&chain);
            identity_host_driver=read_ptr(chain.world+0x120);
            line("{\"arena_auto_qa\":\"in-place-listen-ready\",\"implicit_travel\":false,\"package_validation_modified\":false}\r\n");
        }else line("{\"arena_auto_qa\":\"in-place-listen-failed\",\"restart_required\":true}\r\n");
    }
}
#endif
