#pragma once
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>
namespace reforged_sb  {
inline float centerOffset(float top,float bottom) { return 450.f-(top+bottom)*.5f; }
    struct Row  {
        int id=0,score=0,kills=0,deaths=0,assists=0,ping=0,level=0;
        bool verified=false,bot=false,alive=false;
        char team='S';
        std::string name;
    }
    ;
    struct Snapshot  {
        int revision=0,count=0,allies=0,axis=0,remaining=-1,limit=0,capacity=0,own=0,kills=-1,deaths=-1,damage=-1,headshots=-1;
        std::string nation,phase,server,map;
        std::vector<Row> rows;
    }
    ;
    inline bool split(const std::string& s,size_t count,std::vector<std::string>& out)  {
        if(s.size()>1024)return false;
        out.clear();
        size_t begin=0;
        for(size_t i=0;i<=s.size();++i)if(i==s.size()||s[i]=='|') {
            out.push_back(s.substr(begin,i-begin));
            begin=i+1;
        }
        return out.size()==count;
    }
    inline bool integer(const std::string& s,int& out,int lo=-9999999,int hi=9999999)  {
        if(s.empty()||s.size()>9)return false;
        size_t i=s[0]=='-'?1:0;
        if(i==s.size())return false;
        for(;i<s.size();++i)if(s[i]<'0'||s[i]>'9')return false;
        long n=std::strtol(s.c_str(),nullptr,10);
        if(n<lo||n>hi)return false;
        out=(int)n;
        return true;
    }
    inline bool clean(const std::string& s,size_t max)  {
        if(s.empty()||s.size()>max)return false;
        for(unsigned char c:s)if(c<32||c=='"'||c=='\\'||c==';'||c=='^')return false;
        return true;
    }
    inline bool header(const std::string& raw,Snapshot& s)  {
        std::vector<std::string> f;
        if(!split(raw,17,f)||f[0]!="RS1")return false;
        if(!integer(f[1],s.revision,1)||!integer(f[2],s.count,0,64)||!integer(f[4],s.allies)||!integer(f[5],s.axis)||!integer(f[6],s.remaining,-1)||!integer(f[7],s.limit,0)||!integer(f[11],s.capacity,1,64)||!integer(f[12],s.own,0,63)||!integer(f[13],s.kills,-1)||!integer(f[14],s.deaths,-1)||!integer(f[15],s.damage,-1)||!integer(f[16],s.headshots,-1))return false;
        if(f[3]!="american"&&f[3]!="british"&&f[3]!="russian")return false;
        if(f[8]!="LIVE"&&f[8]!="WAITING"&&f[8]!="FINISHED"&&f[8]!="TIMEOUT")return false;
        if(!clean(f[9],48)||!clean(f[10],32)||s.count>s.capacity)return false;
        s.nation=f[3];
        s.phase=f[8];
        s.server=f[9];
        s.map=f[10];
        return true;
    }
    inline bool row(const std::string& raw,Row& r)  {
        std::vector<std::string> f;
        int v,b,a;
        if(!split(raw,12,f))return false;
        if(!integer(f[0],r.id,0,63)||!integer(f[2],r.score)||!integer(f[3],r.kills,-1)||!integer(f[4],r.deaths,-1)||!integer(f[5],r.assists,-1)||!integer(f[6],r.ping,-1,9999)||!integer(f[7],r.level,0,1000)||!integer(f[8],v,0,1)||!integer(f[9],b,0,1)||!integer(f[10],a,0,1))return false;
        if((f[1]!="A"&&f[1]!="G"&&f[1]!="S")||!clean(f[11],32))return false;
        r.team=f[1][0];
        r.name=f[11];
        r.bot=b;
        r.verified=v&&!b;
        r.level=r.verified?r.level:0;
        r.alive=a;
        r.ping=b?-1:r.ping;
        return true;
    }
    inline bool complete(Snapshot& s,const std::vector<std::string>& raw)  {
        if(raw.size()!=(size_t)s.count)return false;
        bool seen[64]= {
        }
        ;
        s.rows.clear();
        for(const auto& text:raw) {
            Row r;
            if(!row(text,r)||seen[r.id])return false;
            seen[r.id]=true;
            s.rows.push_back(r);
        }
        std::stable_sort(s.rows.begin(),s.rows.end(),[](const Row&a,const Row&b) {
            if(a.score!=b.score) return a.score>b.score;
            if(a.kills!=b.kills) return a.kills>b.kills;
            if(a.deaths!=b.deaths) return a.deaths<b.deaths;
            return a.id<b.id;
        }
        );
        return true;
    }
    struct Layout  {
        int a=0,b=0,pages=1;
    }
    ;
    inline Layout layout(int a,int b)  {
        Layout l;
        l.a=(std::min)(a,8);
        l.b=(std::min)(b,8);
        int free=16-l.a-l.b;
        int add=(std::min)(a-l.a,free);
        l.a+=add;
        free-=add;
        l.b+=(std::min)(b-l.b,free);
        l.pages=(std::max)(1,(std::max)(l.a?(a+l.a-1)/l.a:0,l.b?(b+l.b-1)/l.b:0));
        return l;
    }
}
