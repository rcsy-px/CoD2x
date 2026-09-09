#include "../src/mss32/reforged_scoreboard_data.h"
#include <cassert>
#include <iostream>
using namespace reforged_sb;
int main(){
for(int rows=0;rows<=16;++rows){float end=160+176+16+(std::min)(330.f,rows*50.f)+166;float offset=centerOffset(30,end);assert(30+offset>=0&&end+offset<=900);assert((30+offset+end+offset)*.5f==450.f);}

Snapshot s;assert(header("RS1|1|2|british|5|3|90|100|LIVE|DEV EU-1|mp_toujane_fix|8|0|5|2|420|1",s));
std::string a="0|A|5|5|2|1|30|15|1|0|1|Player";
std::string b="1|G|9|9|1|0|12|40|1|1|0|bot1";
assert(complete(s,{a,b}));assert(s.rows[0].id==1);assert(!s.rows[0].verified&&s.rows[0].level==0&&s.rows[0].ping==-1);
assert(!complete(s,{a,a}));assert(!complete(s,{a}));Row r;assert(!row("0|A|0|0|0|0|0|0|0|0|1|bad;command",r));assert(!row("64|A|0|0|0|0|0|0|0|0|1|bad",r));
assert(!header("RS1|1|65|british|5|3|90|100|LIVE|DEV|map|64|0|0|0|0|0",s));
for(int a=0;a<=64;++a)for(int b=0;b<=64-a;++b){auto l=layout(a,b);assert(l.a+l.b<=16);assert(l.a*l.pages>=a&&l.b*l.pages>=b);assert(l.pages>=1);}
auto l=layout(5,3);assert(l.a==5&&l.b==3&&l.pages==1);l=layout(1,15);assert(l.a==1&&l.b==15&&l.pages==1);
std::cout<<"Scoreboard parser, invalid snapshots, bot labels and all 2145 team layouts passed.\n";
}
