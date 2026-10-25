#ifndef ME3_NETWORK_ACTOR_IDENTITY_SELFTEST_H
#define ME3_NETWORK_ACTOR_IDENTITY_SELFTEST_H
/* Synthetic memory only. No game starts and no engine object is changed. */
static int guest_player_context_channel(uintptr_t base,uintptr_t pawn,NetworkActorIdentityEntry *out);
static void network_fixture_pointer(uintptr_t at,uintptr_t value) { memcpy((void*)at,&value,8); }
static void network_fixture_array(uintptr_t at,uintptr_t data,int count,int max) {
    GameArray a={data,count,max};memcpy((void*)at,&a,sizeof a);
}
static int network_actor_identity_selftest(void) {
    unsigned char *image=VirtualAlloc(NULL,0x1900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    unsigned char *memory=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,20*0x6100);
    if(!image||!memory){if(image)VirtualFree(image,0,MEM_RELEASE);if(memory)HeapFree(GetProcessHeap(),0,memory);return 0;}
    uintptr_t base=(uintptr_t)image,o[20];unsigned char names[20*128]={0};
    const char *labels[]={"FixtureWorld","FixtureLevel","FixtureInfo","FixtureDriver","FixtureConnection","FixturePawn","FixtureChannel","OtherConnection","OtherChannel","World","Level","WorldInfo","NetDriver","NetConnection","Pawn","ActorChannel","Actor","SFXWeapon_NativeBase","FixtureWeapon","WeaponChannel"};
    const int classes[]={9,10,11,12,13,14,15,13,15,9,10,11,12,13,14,15,16,17,17,15};
    for(int i=0;i<20;++i){o[i]=(uintptr_t)(memory+i*0x6100);uint32_t name=i*128;
        memcpy((void*)(o[i]+8),&i,4);memcpy((void*)(o[i]+0x48),&name,4);strcpy((char*)names+i*128+12,labels[i]);}
    for(int i=0;i<20;++i)network_fixture_pointer(o[i]+0x50,o[classes[i]]);
    network_fixture_pointer(o[14]+0x60,o[16]);network_fixture_pointer(o[17]+0x60,o[16]);
    network_fixture_pointer(base+NAME_POOLS_RVA,(uintptr_t)names);
    network_fixture_array(base+OBJECTS_RVA,(uintptr_t)o,20,20);
    network_fixture_pointer(base+0x18b6638,o[0]);network_fixture_pointer(o[0]+0x88,o[1]);
    uintptr_t actors[]={o[2]},connections[]={o[4],o[7]},channels[]={o[6],o[19],o[8]};
    network_fixture_array(o[1]+0x60,(uintptr_t)actors,1,1);
    network_fixture_pointer(o[0]+0x120,o[3]);network_fixture_pointer(o[5]+0x198,o[2]);network_fixture_pointer(o[18]+0x198,o[2]);
    network_fixture_pointer(o[3]+0x80,o[4]);network_fixture_pointer(o[4]+0x8c,o[3]);network_fixture_pointer(o[7]+0x8c,o[3]);
    network_fixture_array(o[3]+0x70,(uintptr_t)connections,1,2);
    network_fixture_array(o[4]+0x5f90,(uintptr_t)channels,2,3);
    network_fixture_array(o[7]+0x5f90,(uintptr_t)(channels+2),1,1);
    const int actors_for_channel[]={5,5,18},channel_objects[]={6,8,19};
    for(int i=0;i<3;++i){uintptr_t ch=o[channel_objects[i]];int32_t index=i+7,type=2;
        network_fixture_pointer(ch,base+0x11a2150);network_fixture_pointer(ch+0xa0,o[actors_for_channel[i]]);
        network_fixture_pointer(ch+0x60,o[i==1?7:4]);memcpy((void*)(ch+0x6c),&index,4);memcpy((void*)(ch+0x78),&type,4);
        network_fixture_pointer(o[i==1?7:4]+0xf84+index*8,ch);}
    network_fixture_pointer(base+0x11a2150+0x2b0,base+0x4840a0);network_fixture_pointer(base+0x11a2150+0x2d0,base+0x488700);
    const unsigned char init[]={0x48,0x89,0x47,0x60,0x89,0x6f,0x6c,0x89,0x77,0x70},type[]={0xc7,0x41,0x78,0x02,0x00,0x00,0x00};
    const unsigned char table[]={0x48,0x63,0xcb,0x48,0x8d,0x9e,0x90,0x5f,0x00,0x00,0x48,0x89,0xbc,0xce,0x84,0x0f,0x00,0x00},actor[]={0x49,0x8b,0x85,0xa0,0x00,0x00,0x00,0xc6,0x80,0x6a,0x02,0x00,0x00,0x03};
    memcpy(image+0x4840dd,init,sizeof init);memcpy(image+0x484f32,type,sizeof type);memcpy(image+0x480254,table,sizeof table);memcpy(image+0x489ce5,actor,sizeof actor);
    const unsigned char global[]={0x4c,0x8b,0x05,0x55,0xc6,0xfe,0x00},driver[]={0x49,0x8b,0x98,0x20,0x01,0x00,0x00};
    const unsigned char mode[]={0x48,0x8b,0x81,0x88,0x00,0x00,0x00,0x48,0x8b,0x48,0x60,0x48,0x8b,0x01,0x0f,0xb6,0x80,0xc8,0x07,0x00,0x00,0xc3};
    memcpy(image+0x8c9fdc,global,sizeof global);memcpy(image+0x8ca012,driver,sizeof driver);memcpy(image+0x8c1e90,mode,sizeof mode);
    int action=test_action;uintptr_t host=identity_host_driver;NetworkActorIdentityEntry saved[4];
    memcpy(saved,network_actor_identity_cache,sizeof saved);unsigned int next=network_actor_identity_next;
    test_action=5;*(unsigned char*)(o[2]+0x7c8)=3;
    int ok=network_actor_channel_id(base,o[5])==7&&network_weapon_channel_id(base,o[18])==9&&network_actor_channel_id(base,o[18])<0&&network_weapon_channel_id(base,o[5])<0;
    /* Target enumeration may replace every discovery hint while the owned
     * pawn and reciprocal channel remain live. Context must rediscover it. */
    memset(network_actor_identity_cache,0,sizeof network_actor_identity_cache);
    NetworkActorIdentityEntry context_channel={0};
    ok=ok&&guest_player_context_channel(base,o[5],&context_channel)&&context_channel.channel==o[6]&&context_channel.index==7;
    /* Existing cache entries must not conceal a duplicate or broken scope. */
    network_fixture_array(o[4]+0x5f90,(uintptr_t)channels,3,3);network_fixture_pointer(o[8]+0x60,o[4]);network_fixture_pointer(o[4]+0xf84+8*8,o[8]);
    ok=ok&&network_actor_channel_id(base,o[5])<0;
    ok=ok&&!guest_player_context_channel(base,o[5],&context_channel);
    network_fixture_array(o[4]+0x5f90,(uintptr_t)channels,2,3);network_fixture_pointer(o[8]+0x60,o[7]);
    ok=ok&&network_actor_channel_id(base,o[5])==7;
    network_fixture_pointer(o[4]+0x8c,0);ok=ok&&network_actor_channel_id(base,o[5])<0;network_fixture_pointer(o[4]+0x8c,o[3]);
    network_fixture_pointer(o[4]+0xf84+7*8,0);ok=ok&&network_actor_channel_id(base,o[5])<0;network_fixture_pointer(o[4]+0xf84+7*8,o[6]);
    network_fixture_pointer(o[5]+0x198,o[0]);ok=ok&&network_actor_channel_id(base,o[5])<0;network_fixture_pointer(o[5]+0x198,o[2]);
    network_fixture_pointer(o[0]+0x120,o[4]);ok=ok&&network_actor_channel_id(base,o[5])<0;network_fixture_pointer(o[0]+0x120,o[3]);
    network_fixture_array(o[4]+0x5f90,(uintptr_t)channels,2,1);ok=ok&&network_actor_channel_id(base,o[5])<0;network_fixture_array(o[4]+0x5f90,(uintptr_t)channels,2,3);
    image[0x8ca012]^=1;ok=ok&&network_actor_channel_id(base,o[5])<0;image[0x8ca012]^=1;
    uintptr_t saved_actor=o[5];o[5]=0;ok=ok&&network_actor_channel_id(base,saved_actor)<0;o[5]=saved_actor;
    test_action=6;identity_host_driver=o[3];*(unsigned char*)(o[2]+0x7c8)=2;network_fixture_pointer(o[3]+0x80,0);
    ok=ok&&network_actor_channel_id(base,o[5])==7;
    /* The host must examine all peers, including a second matching channel. */
    network_fixture_array(o[3]+0x70,(uintptr_t)connections,2,2);ok=ok&&network_actor_channel_id(base,o[5])<0;
    network_fixture_array(o[3]+0x70,(uintptr_t)connections,1,2);identity_host_driver=0;ok=ok&&network_actor_channel_id(base,o[5])<0;
    test_action=action;identity_host_driver=host;memcpy(network_actor_identity_cache,saved,sizeof saved);network_actor_identity_next=next;
    network_actor_layout_matches(0);VirtualFree(image,0,MEM_RELEASE);HeapFree(GetProcessHeap(),0,memory);return ok;
}
#endif
