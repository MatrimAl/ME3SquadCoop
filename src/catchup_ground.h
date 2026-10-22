#ifndef ME3_CATCHUP_GROUND_H
#define ME3_CATCHUP_GROUND_H
/* A supporting Actor.Base is optional on world geometry. Ground readiness
 * comes from the live floor normal and velocity, after the caller has already
 * checked walking physics, reciprocal ownership and story/world identities.
 */
static int catchup_ground_allowed(float health,const float velocity[3],const float floor[3]) {
    if(!isfinite(health)||health<=0)return 0;
    for(int i=0;i<3;++i)if(!isfinite(velocity[i])||!isfinite(floor[i]))return 0;
    return fabsf(velocity[2])<=20.0f&&floor[2]>=0.5f&&floor[2]<=1.01f;
}
static int catchup_ground_selftest(void) {
    float velocity[3]={0,0,0},floor[3]={0,0,1};
    /* Ordinary world-floor case: no supporting actor is needed. */
    if(!catchup_ground_allowed(1,velocity,floor))return 0;
    floor[2]=0;if(catchup_ground_allowed(1,velocity,floor))return 0;
    floor[2]=0.49f;if(catchup_ground_allowed(1,velocity,floor))return 0;
    floor[2]=2;if(catchup_ground_allowed(1,velocity,floor))return 0;
    floor[2]=1;floor[0]=NAN;if(catchup_ground_allowed(1,velocity,floor))return 0;
    floor[0]=0;velocity[2]=21;if(catchup_ground_allowed(1,velocity,floor))return 0;
    velocity[2]=-21;if(catchup_ground_allowed(1,velocity,floor))return 0;
    velocity[2]=0;velocity[0]=INFINITY;if(catchup_ground_allowed(1,velocity,floor))return 0;
    velocity[0]=0;
    return !catchup_ground_allowed(0,velocity,floor)&&!catchup_ground_allowed(NAN,velocity,floor);
}
#endif
