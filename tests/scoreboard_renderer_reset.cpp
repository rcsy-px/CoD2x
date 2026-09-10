#include "../src/mss32/reforged_scoreboard.cpp"
#include <cassert>
void* patch_call(unsigned int, unsigned int) { std::abort(); } // Never patch a test process.
int main() {
    valid=true; snapshot.own=7; snapshot.nation="american";
    for(int i=0;i<3;++i) {
        skin=logo=flagA=flagG=skull=reinterpret_cast<materialHandle_t*>(1);
        for(auto& rank:ranks)rank=reinterpret_cast<materialHandle_t*>(2);
        pending=subscribed=true; pulse=123;
        reforged_scoreboard_renderer_reset();
        assert(!skin&&!logo&&!flagA&&!flagG&&!skull);
        for(auto rank:ranks)assert(!rank);
        assert(!pending&&!subscribed&&!pulse);
        assert(valid&&snapshot.own==7&&snapshot.nation=="american");
    }
}
