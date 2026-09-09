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
    int page=0,minimumTeamRows=4;
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
    void text(const std::string& s,float x,float y,float scale=.26f,vec4_t color=ivory,bool right=false,fontHandle_t* font=nullptr) {
        if(!font)font=fontNormal;
        float px=x*.4f;
        if(right)px-=UI_TextWidth(s.c_str(),s.size(),font,scale);
        UI_DrawText(s.c_str(),s.size(),font,px,(y+verticalOffset)*(480.f/900),HORIZONTAL_ALIGN_FULLSCREEN,VERTICAL_ALIGN_FULLSCREEN,scale,color,TEXT_STYLE_SHADOWED);
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
            1,1,1,.90f
        }
        ;
        pic(112,y,1376,h,c,skin);
        box(112,y,1376,h,0,0,0,.52f);
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
        int slots=(std::max)(shown,minimumTeamRows);
        panel(y,100+slots*height);
        box(113,y+59,1374,1,.66f,.55f,.34f,.65f);
        box(130,y+99,1340,1,.66f,.55f,.34f,.65f);
        pic(137,y+8,90,46,ivory,axis?flagG:flagA);
        std::string nation=axis?"GERMAN":snapshot.nation=="british"?"BRITISH":snapshot.nation=="american"?"AMERICAN":"SOVIET";
        text(nation,244,y+43,.42f,ivory);
        text(std::to_string(rows.size())+(rows.size()==1?" PLAYER":" PLAYERS"),485,y+43,.20f,ivory);
        text("RANK",177,y+87,.20f,muted);
        text("PLAYER",340,y+87,.20f,muted);
        const float cols[]= {
            900,1020,1140,1270,1455
        }
        ;
        const char* labels[]= {
            "SCORE","KILLS","DEATHS","ASSISTS","PING"
        }
        ;
        for(int i=0;i<5;++i)text(labels[i],cols[i],y+87,.20f,muted,true);
        for(int i=begin;i<end;++i) {
            const Row&r=rows[i];
            float ry=y+100+(i-begin)*height;
            bool own=r.id==snapshot.own;
            if(own) {
                box(122,ry,1356,height,.62f,.45f,.14f,.24f);
                box(122,ry,1356,1,.9f,.71f,.36f,.95f);
                box(122,ry+height-1,1356,1,.9f,.71f,.36f,.95f);
                box(1477,ry,1,height,.9f,.71f,.36f,.95f);
                box(122,ry,1,height,.86f,.67f,.29f,1);
            }
            else if((i-begin)%2==0)box(113,ry,1374,height,1,1,1,.025f);
            box(130,ry+height-1,1340,1,.7f,.68f,.57f,.16f);
            float scale=height<35?.23f:.30f;
            float baseline=ry+height*.67f;
            vec4_t c;
            memcpy(c,r.alive?ivory:muted,sizeof(c));
            if(!r.alive)pic(131,ry+height*.28f,16,16,muted,skull);
            if(r.level>0) {
                int rank=r.level>=40?8:r.level>=35?7:r.level>=30?6:r.level>=25?5:r.level>=20?4:r.level>=15?3:r.level>=10?2:r.level>=5?1:0;
                float size=(std::min)(height-6,36.f);
                pic(171,ry+(height-size)/2,size,size,ivory,ranks[rank]);
                text("LVL "+std::to_string(r.level),223,baseline,.21f,ivory);
            }
            else text("-",223,baseline,scale,muted);
            std::string displayName=fit(r.name,365,scale);
            text(displayName,340,baseline,scale,c);
            float badgeX=340+UI_TextWidth(displayName.c_str(),displayName.size(),fontNormal,scale)/.4f+14;
            if(r.bot) {
                box(badgeX-3,baseline-19,49,22,.65f,.65f,.59f,.3f);
                text("BOT",badgeX,baseline,.18f,ivory);
            } else if(r.verified) {
                vec4_t checkColor={own?.9f:.48f,own?.72f:.67f,own?.34f:.84f,1};
                CG_DrawRotatedPic((badgeX+2)*.4f,(baseline-8)*(480.f/900),5*.4f,2*(480.f/900),HORIZONTAL_ALIGN_FULLSCREEN,VERTICAL_ALIGN_FULLSCREEN,45,checkColor,shaderWhite);
                CG_DrawRotatedPic((badgeX+5)*.4f,(baseline-11)*(480.f/900),11*.4f,2*(480.f/900),HORIZONTAL_ALIGN_FULLSCREEN,VERTICAL_ALIGN_FULLSCREEN,-45,checkColor,shaderWhite);
            }
            int values[]= {
                r.score,r.kills,r.deaths,r.assists,r.bot?-1:nativePing(r.id)
            }
            ;
            for(int n=0;n<5;++n)text(number(values[n]),cols[n],baseline,scale,own?gold:c,true);
        }
        if(rows.empty()&&slots>0)text("No players on this team",340,y+100+height*1.7f,.23f,muted);
        return y+100+slots*height;
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
        minimumTeamRows=l.a+l.b<=8?4:0;
        int rowSlots=(std::max)(minimumTeamRows,l.a)+(std::max)(minimumTeamRows,l.b);
        float h=(std::min)(44.f,336.f/(std::max)(1,rowSlots));
        verticalOffset=0;
        panel(30,112);
        pic(127,36,235,99,ivory,logo);
        const float dividers[]={374,518,678,884,1160,1320};
        for(float x:dividers)box(x,48,1,73,.7f,.62f,.43f,.45f);
        auto centered=[](const std::string& value,float x,float y,float scale,vec4_t color) {
            float width=UI_TextWidth(value.c_str(),value.size(),fontNormal,scale)/.4f;
            text(value,x-width/2,y,scale,color);
        };
        centered(snapshot.server,446,89,.25f,ivory);
        centered("SERVER",446,115,.16f,muted);
        std::string map=snapshot.map;
        if(map=="mp_toujane_fix"||map=="mp_toujane")map="TOUJANE";
        else {if(map.substr(0,3)=="mp_")map=map.substr(3);for(char& ch:map){if(ch=='_')ch=' ';else if(ch>='a'&&ch<='z')ch-=32;}}
        centered(fit(map,143,.23f),598,89,.23f,ivory);
        centered("MAP",598,115,.16f,muted);
        float scoreScale=.9f;
        int scoreWidth=(std::max)(UI_TextWidth(number(snapshot.allies).c_str(),number(snapshot.allies).size(),fontNormalBold,scoreScale),UI_TextWidth(number(snapshot.axis).c_str(),number(snapshot.axis).size(),fontNormalBold,scoreScale));
        if(scoreWidth>32)scoreScale*=32.f/scoreWidth;
        text(number(snapshot.allies),770,111,scoreScale,gold,true,fontNormalBold);
        text(":",781,103,.56f,muted);
        text(number(snapshot.axis),817,111,scoreScale,red,false,fontNormalBold);
        centered("TEAM DEATHMATCH",1022,89,.23f,ivory);
        centered("GAME MODE",1022,115,.16f,muted);
        centered(std::to_string(snapshot.count)+" / "+std::to_string(snapshot.capacity),1240,89,.29f,ivory);
        centered("PLAYERS",1240,115,.16f,muted);
        std::string clock=snapshot.phase;
        if(snapshot.phase=="LIVE"&&snapshot.remaining>=0) {
            char t[24];snprintf(t,sizeof(t),"%02d:%02d",snapshot.remaining/60,snapshot.remaining%60);clock=t;
        }
        centered(clock,1403,87,.33f,gold);
        centered(snapshot.limit>0?"LIMIT: "+number(snapshot.limit):"NO SCORE LIMIT",1403,115,.16f,muted);
        float bottom=team(a,false,160,l.a,h);
        bottom=team(b,true,bottom+16,l.b,h);
        panel(bottom+16,57);
        box(795,bottom+17,1,55,.7f,.62f,.43f,.45f);
        text("SPECTATORS",136,bottom+52,.19f,ivory);
        text(fit(spectators.empty()?"None":spectators,450,.21f),305,bottom+52,.21f);
        char kd[24];
        if(snapshot.kills<0||snapshot.deaths<0)snprintf(kd,sizeof(kd),"-");
        else snprintf(kd,sizeof(kd),"%.2f",snapshot.kills/(float)(std::max)(1,snapshot.deaths));
        text("YOUR MATCH",817,bottom+52,.18f,gold);
        text(std::string("K/D: ")+kd,987,bottom+52,.20f,ivory);
        text("DAMAGE: "+number(snapshot.damage),1153,bottom+52,.20f,ivory);
        text("HEADSHOTS: "+number(snapshot.headshots),1464,bottom+52,.20f,ivory,true);
        if(l.pages>1)text("PAGE "+std::to_string(page+1)+" / "+std::to_string(l.pages)+"   PGUP / PGDN",132,bottom+101,.18f,gold);
        text("HOLD TAB - SCOREBOARD",1488,bottom+101,.18f,ivory,true);
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
