#include "reforged_scoreboard_data.h"
#include "reforged_scoreboard.h"
#include "drawing.h"
#include "shared.h"
#include "../shared/cod2_client.h"
#include "../shared/cod2_dvars.h"
#include "../shared/cod2_cmd.h"
#include <cstring>
namespace  {
    using namespace reforged_sb;
    Snapshot snapshot;
    std::string lastCommit, currentMap;
    DWORD updated=0,pulse=0;
    bool subscribed=false,valid=false,previousNext=false,previousBack=false;
    int page=0;
    float verticalOffset=0;
    bool pending=false,wasActive=false;
    materialHandle_t *skin=nullptr,*logo=nullptr,*flagA=nullptr,*flagG=nullptr,*skull=nullptr,*ranks[9]= {
    }
    ;
    vec4_t gold= {
        .83f,.66f,.31f,1
    }
    , ivory= {
        .89f,.88f,.81f,1
    }
    , muted= {
        .48f,.51f,.49f,1
    }
    , red= {
        .74f,.35f,.29f,1
    }
    ;
    void pic(float x,float y,float w,float h,vec4_t c,void* material) {
        UI_DrawHandlePic(x*.4f,(y+verticalOffset)*(480.f/900),w*.4f,h*(480.f/900),4,4,c,material);
    }
    void box(float x,float y,float w,float h,float r,float g,float b,float a) {
        vec4_t c= {
            r,g,b,a
        }
        ;
        pic(x,y,w,h,c,shaderWhite);
    }
    void line(float y) {
        box(112,y,1376,1,.61f,.47f,.21f,.65f);
    }
    void text(const std::string& s,float x,float y,float scale=.26f,vec4_t color=ivory,bool right=false) {
        float px=x*.4f;
        if(right)px-=UI_TextWidth(s.c_str(),s.size(),fontNormal,scale);
        UI_DrawText(s.c_str(),s.size(),fontNormal,px,(y+verticalOffset)*(480.f/900),HORIZONTAL_ALIGN_FULLSCREEN,VERTICAL_ALIGN_FULLSCREEN,scale,color,TEXT_STYLE_SHADOWED);
    }
    std::string fit(std::string s,float width,float scale=.26f) {
        if(UI_TextWidth(s.c_str(),s.size(),fontNormal,scale)<=width*.4f)return s;
        while(!s.empty()&&UI_TextWidth((s+"...").c_str(),s.size()+3,fontNormal,scale)>width*.4f)s.pop_back();
        return s+"...";
    }
    // Stock CG_ParseScores (0x004d02d0): 24-byte rows, client id +0, ping +8.
    // PAM already refreshes this table with ShowScoreBoard; no extra request needed.
    int nativePing(int id) {
        int count=*(int*)0x01518f80;
        if(count<0||count>64)return -1;
        for(int i=0;i<count;++i) {
            const int* row=(const int*)(0x01518fb4+i*24);
            if(row[0]==id)return row[2]>=0&&row[2]<=9999?row[2]:-1;
        }
        return -1;
    }
    std::string number(int n) {
        return n<0?"-":std::to_string(n);
    }
    void panel(float y,float h) {
        vec4_t c= {
            1,1,1,.94f
        }
        ;
        pic(112,y,1376,h,c,skin);
        box(112,y,1376,h,0,0,0,.68f);
        line(y);
        line(y+h);
        box(112,y,1,h,.61f,.47f,.21f,.65f);
        box(1487,y,1,h,.61f,.47f,.21f,.65f);
    }
    void materials() {
        if(skin)return;
        skin=CG_RegisterMaterial("rfg_s000",MATERIAL_TYPE_DEFAULT);
        logo=CG_RegisterMaterial("rfg_logo",MATERIAL_TYPE_DEFAULT);
        skull=CG_RegisterMaterial("rfg_kill",MATERIAL_TYPE_DEFAULT);
        flagG=CG_RegisterMaterial("hud_flag_german",MATERIAL_TYPE_DEFAULT);
        flagA=CG_RegisterMaterial(("hud_flag_"+(snapshot.nation.empty()?std::string("british"):snapshot.nation)).c_str(),MATERIAL_TYPE_DEFAULT);
        for(int i=0;i<9;++i) {
            char n[16];
            snprintf(n,sizeof(n),"rfg_rk%02d",i);
            ranks[i]=CG_RegisterMaterial(n,MATERIAL_TYPE_DEFAULT);
        }
    }
    float team(const std::vector<Row>& rows,bool axis,float y,int capacity,float height) {
        int begin=page*capacity,end=(std::min)((int)rows.size(),begin+capacity);
        int shown=(std::max)(0,end-begin);
        panel(y,88+shown*height);
        pic(142,y+10,40,40,ivory,axis?flagG:flagA);
        std::string nation=axis?"GERMAN ARMY":snapshot.nation=="british"?"BRITISH ARMY":snapshot.nation=="american"?"UNITED STATES ARMY":"RED ARMY";
        text(nation,212,y+37,.29f,axis?red:gold);
        text(std::to_string(rows.size())+" PLAYERS",1455,y+37,.23f,muted,true);
        text("RANK",166,y+74,.20f,muted);
        text("PLAYER",325,y+74,.20f,muted);
        const float cols[]= {
            900,1020,1140,1270,1455
        }
        ;
        const char* labels[]= {
            "SCORE","KILLS","DEATHS","ASSISTS","PING"
        }
        ;
        for(int i=0;i<5;++i)text(labels[i],cols[i],y+74,.20f,muted,true);
        for(int i=begin;i<end;++i) {
            const Row&r=rows[i];
            float ry=y+88+(i-begin)*height;
            bool own=r.id==snapshot.own;
            if(own) {
                box(113,ry,1374,height,.62f,.45f,.14f,.23f);
                box(113,ry,3,height,.86f,.67f,.29f,1);
            }
            else if((i-begin)%2==0)box(113,ry,1374,height,1,1,1,.025f);
            float scale=height<35?.21f:.27f;
            float baseline=ry+height*.67f;
            vec4_t c;
            memcpy(c,r.alive?ivory:muted,sizeof(c));
            if(!r.alive)pic(131,ry+height*.28f,16,16,muted,skull);
            if(r.level>0) {
                int rank=r.level>=40?8:r.level>=35?7:r.level>=30?6:r.level>=25?5:r.level>=20?4:r.level>=15?3:r.level>=10?2:r.level>=5?1:0;
                float size=(std::min)(height-6,36.f);
                pic(171,ry+(height-size)/2,size,size,ivory,ranks[rank]);
                text(std::to_string(r.level),230,baseline,scale,gold);
            }
            else text("-",230,baseline,scale,muted);
            text(fit(r.name,385,scale),325,baseline,scale,c);
            if(r.bot)text("BOT",747,baseline,.18f,muted);
            else if(r.verified)text("VERIFIED",747,baseline,.16f,gold);
            int values[]= {
                r.score,r.kills,r.deaths,r.assists,r.bot?-1:nativePing(r.id)
            }
            ;
            for(int n=0;n<5;++n)text(number(values[n]),cols[n],baseline,scale,own?gold:c,true);
        }
        return y+88+shown*height;
    }
    int original() {
        int result=0;
        ASM_CALL(RETURN(result),0x004dabf0);
        return result;
    }
    int render() {
        if(Dvar_GetInt("ui_rf_sb_version")!=1)return original();
        if(!*(int*)0x015195b4)return 0;
        dvar_t* paused=*(dvar_t**)0x0166e01c;
        if(paused&&paused->value.integer)return 0;
        materials();
        verticalOffset=0;
        if(!valid||GetTickCount()-updated>5000) {
            panel(394,112);
            text("REFORGED SCOREBOARD",145,440,.30f,gold);
            text(valid?"Updating match data...":"Loading match data...",145,477,.24f,muted);
            return 1;
        }
        std::vector<Row>a,b;
        std::string spectators;
        for(const Row&r:snapshot.rows) {
            if(r.team=='A')a.push_back(r);
            else if(r.team=='G')b.push_back(r);
            else {
                if(!spectators.empty())spectators+=", ";
                spectators+=r.name;
            }
        }
        Layout l=layout(a.size(),b.size());
        page=(std::min)(page,l.pages-1);
        float h=(std::min)(50.f,330.f/(std::max)(1,l.a+l.b));
        int visibleA=(std::max)(0,(std::min)(l.a,(int)a.size()-page*l.a));
        int visibleB=(std::max)(0,(std::min)(l.b,(int)b.size()-page*l.b));
        float contentBottom=160+88+88+16+(visibleA+visibleB)*h+166;
        verticalOffset=centerOffset(30,contentBottom);

        panel(30,112);
        pic(130,43,175,85,ivory,logo);
        text(snapshot.server,337,70,.30f,gold);
        std::string map=snapshot.map;
        if(map=="mp_toujane_fix"||map=="mp_toujane")map="TOUJANE";
        else if(map=="mp_carentan")map="CARENTAN";
        else if(map=="mp_downtown")map="STALINGRAD";
        text(fit(map,300),337,111,.24f);
        text(number(snapshot.allies),748,102,.52f,gold,true);
        text(":",797,100,.40f,muted);
        text(number(snapshot.axis),844,102,.52f,red);
        text("TEAM DEATHMATCH",1030,67,.24f,gold);
        text(std::to_string(snapshot.count)+" / "+std::to_string(snapshot.capacity)+" PLAYERS",1455,67,.21f,muted,true);
        std::string clock=snapshot.phase;
        if(snapshot.phase=="LIVE"&&snapshot.remaining>=0) {
            char t[24];
            snprintf(t,sizeof(t),"%d:%02d",snapshot.remaining/60,snapshot.remaining%60);
            clock=t;
        }
        text(clock,1030,108,.29f);
        text(snapshot.limit>0?"LIMIT "+number(snapshot.limit):"NO SCORE LIMIT",1455,108,.21f,muted,true);
        float bottom=team(a,false,160,l.a,h);
        bottom=team(b,true,bottom+16,l.b,h);
        panel(bottom+16,94);
        text("SPECTATORS",136,bottom+45,.20f,muted);
        text(fit(spectators.empty()?"None":spectators,1080,.21f),330,bottom+45,.21f);
        char kd[24];
        if(snapshot.kills<0||snapshot.deaths<0)snprintf(kd,sizeof(kd),"-");
        else snprintf(kd,sizeof(kd),"%.2f",snapshot.kills/(float)(std::max)(1,snapshot.deaths));
        text(std::string("YOUR K/D  ")+kd,136,bottom+85,.23f,gold);
        text("DAMAGE  "+number(snapshot.damage),625,bottom+85,.23f);
        text("HEADSHOTS  "+number(snapshot.headshots),1455,bottom+85,.23f,ivory,true);
        box(112,bottom+122,1376,44,0,0,0,.9f);
        text("HOLD TAB TO VIEW SCOREBOARD",132,bottom+151,.19f,muted);
        if(l.pages>1)text("PAGE "+std::to_string(page+1)+" / "+std::to_string(l.pages)+"    PGUP / PGDN",1468,bottom+151,.20f,gold,true);
        else text("CLASSIC COMBAT. REFORGED.",1468,bottom+151,.19f,muted,true);
        return 1;
    }
    int draw() {
        pending=false;
        if(Dvar_GetInt("ui_rf_sb_version")!=1)return original();
        dvar_t* paused=*(dvar_t**)0x0166e01c;
        if(!*(int*)0x015195b4||(paused&&paused->value.integer))return 0;
        pending=true;
        return 1;
    }
}
void reforged_scoreboard_end() {
    if(pending) {
        render();
        pending=false;
    }
}
void reforged_scoreboard_init() {
}
void reforged_scoreboard_frame() {
    pending=false;
    bool active=clientState==CLIENT_STATE_ACTIVE;
    if(wasActive&&!active) {
        dvar_t* version=Dvar_GetDvarByName("ui_rf_sb_version");
        if(version)Dvar_SetString(version,"0");
    }
    wasActive=active;
    std::string map=active?(char*)0x0196ffa0:"";
    if(!active||map!=currentMap) {
        valid=false;
        subscribed=false;
        lastCommit.clear();
        skin=nullptr;
        currentMap=map;
        page=0;
    }
    if(!active||Dvar_GetInt("ui_rf_sb_version")!=1)return;
    DWORD now=GetTickCount();
    bool held=*(int*)0x015195b4!=0;
    if(held&&(!subscribed||now-pulse>=1500)) {
        Cbuf_AddText("openscriptmenu ingame rf_scoreboard_pulse\n");
        pulse=now;
        subscribed=true;
    }
    else if(!held&&subscribed) {
        Cbuf_AddText("openscriptmenu ingame rf_scoreboard_close\n");
        subscribed=false;
    }
    if(!held)return;
    std::string commit=Dvar_GetString("ui_rf_sb_commit");
    if(!commit.empty()&&commit!=lastCommit) {
        lastCommit=commit;
        Snapshot candidate;
        if(header(commit,candidate)&&candidate.map==map) {
            std::vector<std::string> rows;
            for(int i=0;i<candidate.count;++i)rows.push_back(Dvar_GetString(("ui_rf_sb_r"+std::to_string(i)).c_str()));
            if(complete(candidate,rows)) {
                if(snapshot.nation!=candidate.nation)skin=nullptr;
                snapshot=std::move(candidate);
                updated=now;
                valid=true;
            }
        }
    }
    bool next=(GetAsyncKeyState(VK_NEXT)&0x8000)!=0,back=(GetAsyncKeyState(VK_PRIOR)&0x8000)!=0;
    if(next&&!previousNext)++page;
    if(back&&!previousBack)page=(std::max)(0,page-1);
    previousNext=next;
    previousBack=back;
}
void reforged_scoreboard_patch() {
    const unsigned char expected[]= {
        0xe8,0x44,0xee,0x00,0x00
    }
    ;
    if(memcmp((void*)0x004cbda7,expected,sizeof(expected))==0)patch_call(0x004cbda7,(unsigned int)draw);
}
