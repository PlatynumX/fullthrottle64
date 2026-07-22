#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "osys_n64.h"
#include "backends/saves/default/default-saves.h"
#include "backends/fs/posix/posix-fs-factory.h"
#include "common/textconsole.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static const OSystem::GraphicsMode kModes[] = {
    { "320x240", "320x240", 0 },
    { 0, 0, 0 }
};

OSystem_N64::OSystem_N64()
    : _mixer(0), _screen8(0), _overlay16(0), _cursor16(0), _cursor8(0),
      _cursorW(0), _cursorH(0), _cursorHotX(0), _cursorHotY(0), _cursorKey(0),
      _cursorUsesOwnPalette(false), _mouseVisible(false), _mouseX(160), _mouseY(120),
      _screenW(320), _screenH(240), _gameW(320), _gameH(200), _shake(0),
      _overlayVisible(false), _dirty(true), _gfxMode(0), _prevButtons(0) {

    memset(_palette, 0, sizeof(_palette));
    memset(_exactPalette, 0, sizeof(_exactPalette));
    memset(_cursorPalette, 0, sizeof(_cursorPalette));
}

OSystem_N64::~OSystem_N64() {
    delete _mixer;
    free(_screen8);
    free(_overlay16);
    free(_cursor16);
    free(_cursor8);
}

void OSystem_N64::initBackend() {
    debug_init_isviewer();
    debug_init_usblog();

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS);
    joypad_init();

    int rc = dfs_init(DFS_DEFAULT_LOCATION);
    if (rc != DFS_ESUCCESS)
        debugf("[FT64] DragonFS init failed: %s (%d)\n", dfs_strerror(rc), rc);

    _screen8 = (uint8 *)memalign(16, 320 * 240);
    _overlay16 = (uint16 *)memalign(16, 320 * 240 * sizeof(uint16));
    memset(_screen8, 0, 320 * 240);
    memset(_overlay16, 0, 320 * 240 * sizeof(uint16));

    audio_init(DEFAULT_SOUND_SAMPLE_RATE, 4);
    mixer_init(32);

    _mixer = new Audio::MixerImpl(this, DEFAULT_SOUND_SAMPLE_RATE);
    _mixer->setReady(true);

    _timerManager = new DefaultTimerManager();
    _savefileManager = new DefaultSaveFileManager("rom:/");
    _fsFactory = new POSIXFilesystemFactory();

    ConfMan.setInt("autosave_period", 0);
    ConfMan.setBool("subtitles", true);
    ConfMan.setBool("speech_mute", false);

    debugf("[FT64] backend init\n");
    debugf("[FT64] RDRAM=%lu bytes\n", (unsigned long)get_memory_size());
    debugf("[FT64] DFS FT.000=%d\n", dfs_rom_size("/game/FT.000"));

    EventsBaseBackend::initBackend();
}

bool OSystem_N64::hasFeature(Feature f) {
    return f == kFeatureCursorPalette;
}
void OSystem_N64::setFeatureState(Feature f, bool enable) {
    if (f == kFeatureCursorPalette)
        _cursorUsesOwnPalette = enable;
}
bool OSystem_N64::getFeatureState(Feature f) {
    return f == kFeatureCursorPalette ? _cursorUsesOwnPalette : false;
}

const OSystem::GraphicsMode *OSystem_N64::getSupportedGraphicsModes() const { return kModes; }
int OSystem_N64::getDefaultGraphicsMode() const { return 0; }
bool OSystem_N64::setGraphicsMode(const char *) { _gfxMode = 0; return true; }
bool OSystem_N64::setGraphicsMode(int mode) { _gfxMode = mode; return true; }
int OSystem_N64::getGraphicsMode() const { return _gfxMode; }

void OSystem_N64::initSize(uint width, uint height, const Graphics::PixelFormat *) {
    _gameW = width > 320 ? 320 : width;
    _gameH = height > 240 ? 240 : height;
    _screenSurface.pixels = _screen8;
    _screenSurface.w = _gameW;
    _screenSurface.h = _gameH;
    _screenSurface.pitch = 320;
    _screenSurface.format = Graphics::PixelFormat::createFormatCLUT8();
    _dirty = true;
    debugf("[FT64] initSize %ux%u -> %dx%d\n", width, height, _gameW, _gameH);
}
int16 OSystem_N64::getHeight() { return _gameH; }
int16 OSystem_N64::getWidth() { return _gameW; }

uint16 OSystem_N64::rgb555(byte r, byte g, byte b) const {
    return (uint16)(((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | 1);
}

void OSystem_N64::setPalette(const byte *colors, uint start, uint num) {
    memcpy(_exactPalette + start * 3, colors, num * 3);
    for (uint i = 0; i < num; ++i) {
        _palette[start+i] = rgb555(colors[0], colors[1], colors[2]);
        colors += 3;
    }
    _dirty = true;
}
void OSystem_N64::grabPalette(byte *colors, uint start, uint num) {
    memcpy(colors, _exactPalette + start * 3, num * 3);
}

void OSystem_N64::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
    if (!buf || w <= 0 || h <= 0) return;
    const byte *src = (const byte *)buf;
    if (x < 0) { src -= x; w += x; x = 0; }
    if (y < 0) { src -= y * pitch; h += y; y = 0; }
    if (x + w > _gameW) w = _gameW - x;
    if (y + h > _gameH) h = _gameH - y;
    if (w <= 0 || h <= 0) return;

    for (int row = 0; row < h; ++row)
        memcpy(_screen8 + (y + row) * 320 + x, src + row * pitch, w);
    _dirty = true;
}

void OSystem_N64::convertGameBuffer(surface_t *dst) {
    int yoff = (240 - _gameH) / 2;
    int xoff = (320 - _gameW) / 2;
    graphics_fill_screen(dst, 0);

    uint16 *fb = (uint16 *)dst->buffer;
    for (int y = 0; y < _gameH; ++y) {
        int sy = y + _shake;
        if (sy < 0 || sy >= _gameH) continue;
        uint16 *d = fb + (y + yoff) * dst->stride / 2 + xoff;
        const uint8 *s = _screen8 + sy * 320;
        for (int x = 0; x < _gameW; ++x)
            d[x] = _palette[s[x]];
    }
}

void OSystem_N64::pumpAudio() {
    if (!_mixer || !audio_can_write()) return;
    short *out = audio_write_begin();
    if (!out) return;
    const int frames = audio_get_buffer_length();
    _mixer->mixCallback((byte *)out, frames * 4);
    audio_write_end();
}

void OSystem_N64::updateScreen() {
    pumpAudio();
    if (!_dirty) return;

    surface_t *disp = display_get();
    if (_overlayVisible) {
        uint16 *dst = (uint16 *)disp->buffer;
        for (int y=0; y<240; ++y)
            memcpy((byte *)dst + y * disp->stride, _overlay16 + y*320, 320*2);
    } else {
        convertGameBuffer(disp);
    }

    if (_mouseVisible && _cursor16 && _cursorW && _cursorH) {
        uint16 *dst = (uint16 *)disp->buffer;
        for (uint cy=0; cy<_cursorH; ++cy) {
            int dy = _mouseY - _cursorHotY + (int)cy;
            if (dy < 0 || dy >= 240) continue;
            for (uint cx=0; cx<_cursorW; ++cx) {
                int dx = _mouseX - _cursorHotX + (int)cx;
                if (dx < 0 || dx >= 320) continue;
                uint16 p = _cursor16[cy*_cursorW+cx];
                if (p != 0)
                    *(uint16 *)((byte *)dst + dy*disp->stride + dx*2) = p;
            }
        }
    }

    display_show(disp);
    _dirty = false;
}

Graphics::Surface *OSystem_N64::lockScreen() { return &_screenSurface; }
void OSystem_N64::unlockScreen() { _dirty = true; }
void OSystem_N64::setShakePos(int shakeOffset) { _shake = shakeOffset; _dirty = true; }

void OSystem_N64::showOverlay() { _overlayVisible = true; _dirty = true; }
void OSystem_N64::hideOverlay() { _overlayVisible = false; _dirty = true; }
void OSystem_N64::clearOverlay() { memset(_overlay16, 0, 320*240*2); _dirty = true; }
void OSystem_N64::grabOverlay(void *buf, int pitch) {
    for (int y=0; y<240; ++y) memcpy((byte*)buf+y*pitch, _overlay16+y*320, 640);
}
void OSystem_N64::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
    const byte *src=(const byte*)buf;
    for (int row=0; row<h && y+row<240; ++row)
        memcpy(_overlay16+(y+row)*320+x, src+row*pitch, w*2);
    _dirty=true;
}
int16 OSystem_N64::getOverlayHeight() { return 240; }
int16 OSystem_N64::getOverlayWidth() { return 320; }

bool OSystem_N64::showMouse(bool visible) {
    bool old=_mouseVisible; _mouseVisible=visible; _dirty=true; return old;
}
void OSystem_N64::warpMouse(int x, int y) {
    if (x<0) x=0; if (x>=_gameW) x=_gameW-1;
    if (y<0) y=0; if (y>=_gameH) y=_gameH-1;
    _mouseX=x; _mouseY=y; _dirty=true;
}
void OSystem_N64::setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY,
                                 uint32 keycolor, bool, const Graphics::PixelFormat *) {
    free(_cursor8); free(_cursor16);
    _cursor8=(uint8*)malloc(w*h);
    _cursor16=(uint16*)malloc(w*h*2);
    memcpy(_cursor8, buf, w*h);
    _cursorW=w; _cursorH=h; _cursorHotX=hotspotX; _cursorHotY=hotspotY; _cursorKey=keycolor&255;
    const uint16 *pal=_cursorUsesOwnPalette?_cursorPalette:_palette;
    for (uint i=0;i<w*h;++i) _cursor16[i]=(_cursor8[i]==_cursorKey)?0:pal[_cursor8[i]];
    _dirty=true;
}
void OSystem_N64::setCursorPalette(const byte *colors, uint start, uint num) {
    for(uint i=0;i<num;++i) {
        _cursorPalette[start+i]=rgb555(colors[0],colors[1],colors[2]); colors+=3;
    }
    _cursorUsesOwnPalette=true;
}

bool OSystem_N64::pollEvent(Common::Event &event) {
    pumpAudio();
    joypad_poll();
    joypad_inputs_t in=joypad_get_inputs(JOYPAD_PORT_1);
    joypad_buttons_t down=joypad_get_buttons_pressed(JOYPAD_PORT_1);
    joypad_buttons_t up=joypad_get_buttons_released(JOYPAD_PORT_1);

    static uint32 lastMove=0;
    uint32 now=getMillis();
    if (now-lastMove >= 16) {
        lastMove=now;
        int dx=in.stick_x/18, dy=-(in.stick_y/18);
        if (dx || dy) {
            warpMouse(_mouseX+dx,_mouseY+dy);
            event.type=Common::EVENT_MOUSEMOVE;
            event.mouse.x=_mouseX; event.mouse.y=_mouseY;
            return true;
        }
    }

    if (down.a) { event.type=Common::EVENT_LBUTTONDOWN; event.mouse.x=_mouseX; event.mouse.y=_mouseY; return true; }
    if (up.a)   { event.type=Common::EVENT_LBUTTONUP;   event.mouse.x=_mouseX; event.mouse.y=_mouseY; return true; }
    if (down.b) { event.type=Common::EVENT_RBUTTONDOWN; event.mouse.x=_mouseX; event.mouse.y=_mouseY; return true; }
    if (up.b)   { event.type=Common::EVENT_RBUTTONUP;   event.mouse.x=_mouseX; event.mouse.y=_mouseY; return true; }

    if (down.start) {
        event.type=Common::EVENT_KEYDOWN; event.kbd.keycode=Common::KEYCODE_F5; event.kbd.ascii=Common::ASCII_F5; return true;
    }
    if (up.start) {
        event.type=Common::EVENT_KEYUP; event.kbd.keycode=Common::KEYCODE_F5; event.kbd.ascii=Common::ASCII_F5; return true;
    }
    if (down.l) {
        event.type=Common::EVENT_KEYDOWN; event.kbd.keycode=Common::KEYCODE_ESCAPE; event.kbd.ascii=27; return true;
    }
    if (up.l) {
        event.type=Common::EVENT_KEYUP; event.kbd.keycode=Common::KEYCODE_ESCAPE; event.kbd.ascii=27; return true;
    }
    return false;
}

uint32 OSystem_N64::getMillis() { return (uint32)(get_ticks_ms()); }
void OSystem_N64::delayMillis(uint msecs) { wait_ms(msecs); pumpAudio(); }

OSystem::MutexRef OSystem_N64::createMutex(void) { return 0; }
void OSystem_N64::lockMutex(MutexRef) {}
void OSystem_N64::unlockMutex(MutexRef) {}
void OSystem_N64::deleteMutex(MutexRef) {}
void OSystem_N64::quit() { while (1) { pumpAudio(); } }
Audio::Mixer *OSystem_N64::getMixer() { return _mixer; }

void OSystem_N64::getTimeAndDate(TimeDate &t) const {
    memset(&t,0,sizeof(t));
    uint32 s=(uint32)(get_ticks_ms()/1000);
    t.tm_sec=s%60; t.tm_min=(s/60)%60; t.tm_hour=(s/3600)%24;
    t.tm_mday=1; t.tm_mon=0; t.tm_year=110;
}
void OSystem_N64::logMessage(LogMessageType::Type, const char *message) {
    debugf("%s", message);
}
