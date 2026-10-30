/* Offline guard fixture. The runner includes the exact current production
 * helpers, not a reimplementation. State-name and reflected-field lookup are
 * explicit stand-ins; all pawn/controller/inventory data is fixture memory.
 * No game process, native weapon method or selected-weapon write is used. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct GameArray {uintptr_t data;int count,max;} GameArray;
typedef struct Object {_Alignas(8) unsigned char bytes[0x1200];} Object;
static Object pc,pawn,inventory,weapon;
static int32_t pending_fire[4];
static const char *weapon_state;
static uintptr_t denied_at;
static int deny_state,deny_field;
static unsigned cases;
static uintptr_t address(Object *object){return (uintptr_t)object->bytes;}
static void u32(uintptr_t at,uint32_t value){memcpy((void*)at,&value,4);}
static int read_at(uintptr_t at,void *out,SIZE_T size){
    SIZE_T got=0;
    if(!at||(denied_at&&at<=denied_at&&denied_at-at<size))return 0;
    return ReadProcessMemory(GetCurrentProcess(),(const void*)at,out,size,&got)&&got==size;
}
static int native_state_name(uintptr_t base,uintptr_t actor,char *out,size_t size){
    (void)base;
    if(deny_state||actor!=address(&weapon)||!weapon_state)return 0;
    return snprintf(out,size,"%s",weapon_state)>0;
}
static uintptr_t runtime_guest_team_field(uintptr_t base,uintptr_t actor,
        const char *owner,const char *name,const char *kind,int offset,int size,uint64_t flags){
    (void)base;
    return !deny_field&&actor==address(&inventory)&&!strcmp(owner,"InventoryManager")&&
        !strcmp(name,"PendingFire")&&!strcmp(kind,"ArrayProperty")&&offset==0x270&&size==16&&
        flags==UINT64_C(0x400000)?address(&inventory)+0x270:0;
}
static int scene_readiness_array(uintptr_t at,GameArray *out,int maximum){
    GameArray value={0};
    if(!read_at(at,&value,sizeof value)||value.count<0||value.max<value.count||
       value.max>maximum||(value.count&&!value.data))return 0;
    *out=value;return 1;
}

/* Generated at execution from the current production headers, byte-for-byte. */
#include "scene_weapon_return_helpers.h"

static void reset(void){
    memset(&pc,0,sizeof pc);memset(&pawn,0,sizeof pawn);
    memset(&inventory,0,sizeof inventory);memset(&weapon,0,sizeof weapon);
    memset(pending_fire,0,sizeof pending_fire);
    GameArray requests={(uintptr_t)pending_fire,4,4};
    memcpy(inventory.bytes+0x270,&requests,sizeof requests);
    weapon_state="Inactive";denied_at=0;deny_state=deny_field=0;
}
static int check(const char *name,int expected_return,int expected_strict){
    Object pc_before=pc,pawn_before=pawn,inventory_before=inventory,weapon_before=weapon;
    int32_t pending_before[4];memcpy(pending_before,pending_fire,sizeof pending_before);
    int result=runtime_scene_weapon_return_ready(0,address(&pc),address(&pawn),address(&inventory),address(&weapon));
    int strict=runtime_scene_weapon_item_idle(0,address(&pc),address(&pawn),address(&inventory),address(&weapon));
    ++cases;
    if(result!=expected_return||strict!=expected_strict){
        fprintf(stderr,"FAIL %s: return=%d/%d strict=%d/%d\n",name,result,expected_return,strict,expected_strict);return 0;
    }
    if(memcmp(&pc,&pc_before,sizeof pc)||memcmp(&pawn,&pawn_before,sizeof pawn)||
       memcmp(&inventory,&inventory_before,sizeof inventory)||memcmp(&weapon,&weapon_before,sizeof weapon)||
       memcmp(pending_fire,pending_before,sizeof pending_before)){
        fprintf(stderr,"FAIL %s: helper changed fixture game storage\n",name);return 0;
    }
    return 1;
}
#define CHECK(name,returned,strict) do{if(!check(name,returned,strict))return 1;}while(0)
int main(void){
    reset();CHECK("clean inactive",1,1);
    weapon.bytes[0x10c2]=2;CHECK("inactive stale pending mode",1,0);
    reset();weapon.bytes[0x3b0]=2;CHECK("inactive stale current mode",1,0);
    reset();u32(address(&weapon)+0x10b4,0x1c000);CHECK("inactive stale animation bits",1,0);
    weapon.bytes[0x10c2]=weapon.bytes[0x3b0]=2;CHECK("inactive all stale equipped fields",1,0);
    reset();weapon_state="Active";CHECK("active preserved by strict helper",0,1);
    weapon_state="WeaponFiring";CHECK("firing",0,0);
    weapon_state="Reloading";CHECK("reloading",0,0);
    weapon_state="WeaponEquipping";CHECK("equipping",0,0);
    reset();u32(address(&pawn)+0xc20,132);CHECK("pawn action",0,0);
    reset();u32(address(&pawn)+0xc28,23);CHECK("pawn power",0,0);
    reset();pc.bytes[0x480]=1;CHECK("controller fire",0,0);
    for(unsigned i=0;i<4;++i){reset();pending_fire[i]=1;CHECK("pending fire request",0,0);}
    reset();pending_fire[0]=-1;CHECK("negative pending request",0,0);
    reset();GameArray empty={0};memcpy(inventory.bytes+0x270,&empty,sizeof empty);CHECK("valid empty request list",1,1);
    reset();GameArray invalid={(uintptr_t)pending_fire,5,4};memcpy(inventory.bytes+0x270,&invalid,sizeof invalid);CHECK("invalid request count",0,0);
    reset();invalid=(GameArray){0,1,1};memcpy(inventory.bytes+0x270,&invalid,sizeof invalid);CHECK("null nonempty request list",0,0);
    reset();invalid=(GameArray){(uintptr_t)pending_fire,33,33};memcpy(inventory.bytes+0x270,&invalid,sizeof invalid);CHECK("oversized request list",0,0);
    reset();deny_state=1;CHECK("unreadable state",0,0);
    reset();deny_field=1;CHECK("unverified reflected field",0,0);
    const uintptr_t required[]={address(&pawn)+0xc20,address(&pawn)+0xc28,address(&pc)+0x480,
        address(&inventory)+0x270,(uintptr_t)&pending_fire[2]};
    for(unsigned i=0;i<sizeof required/sizeof *required;++i){reset();denied_at=required[i];CHECK("required read fails",0,0);}
    const uintptr_t equipped_only[]={address(&weapon)+0x10c2,address(&weapon)+0x3b0,address(&weapon)+0x10b4};
    for(unsigned i=0;i<sizeof equipped_only/sizeof *equipped_only;++i){reset();denied_at=equipped_only[i];CHECK("inactive does not require equipped-only fields",1,0);}
    printf("PASS %u current production scene weapon return/strict-idle guard cases; no game launched\n",cases);
    return 0;
}
