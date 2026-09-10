// Exercise the actual implementation without starting the game or networking.
#include "../src/mss32/reforged_avatar.cpp"
#include <cassert>
int main() {
    cached.ref="default/1";
    cached.pixels.assign(256*256*4, 123);
    requested=cached.ref;
    map="mp_carentan_fix";
    attempts=2;
    retryAt=12345;
    auto* pixels=cached.pixels.data();
    for(int restart=0;restart<3;++restart) {
        // Deliberately invalid addresses: reset must never dereference/release them.
        material=reinterpret_cast<materialHandle_t*>(1);
        uploadedTexture=reinterpret_cast<void*>(2);
        uploadedRef=cached.ref;
        checkedAt=999;
        reforged_avatar_renderer_reset();
        assert(!material && !uploadedTexture && uploadedRef.empty() && !checkedAt);
        assert(cached.ref=="default/1" && cached.pixels.data()==pixels);
        assert(cached.pixels.size()==256*256*4 && cached.pixels[0]==123);
        assert(requested==cached.ref && attempts==2 && retryAt==12345);
        assert(map=="mp_carentan_fix");
        // Same address on the new renderer still requires an upload.
        assert(uploadedTexture!=reinterpret_cast<void*>(2));
    }
    reforged_avatar_renderer_reset();
    assert(!material && !uploadedTexture);
}
