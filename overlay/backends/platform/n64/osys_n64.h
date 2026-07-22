#ifndef FT64_OSYS_N64_H
#define FT64_OSYS_N64_H

#include "common/config-manager.h"
#include "common/events.h"
#include "backends/base-backend.h"
#include "backends/timer/default/default-timer.h"
#include "graphics/surface.h"
#include "graphics/palette.h"
#include "graphics/pixelformat.h"
#include "graphics/colormasks.h"
#include "audio/mixer_intern.h"

#include <libdragon.h>

#define DEFAULT_SOUND_SAMPLE_RATE 22050

class OSystem_N64 : public EventsBaseBackend, public PaletteManager {
public:
    OSystem_N64();
    virtual ~OSystem_N64();

    virtual void initBackend();

    virtual bool hasFeature(Feature f);
    virtual void setFeatureState(Feature f, bool enable);
    virtual bool getFeatureState(Feature f);

    virtual const GraphicsMode *getSupportedGraphicsModes() const;
    virtual int getDefaultGraphicsMode() const;
    virtual bool setGraphicsMode(const char *name);
    virtual bool setGraphicsMode(int mode);
    virtual int getGraphicsMode() const;

    virtual void initSize(uint width, uint height, const Graphics::PixelFormat *format = 0);
    virtual int16 getHeight();
    virtual int16 getWidth();

    virtual PaletteManager *getPaletteManager() { return this; }
    virtual void setPalette(const byte *colors, uint start, uint num);
    virtual void grabPalette(byte *colors, uint start, uint num);

    virtual void copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h);
    virtual void updateScreen();
    virtual Graphics::Surface *lockScreen();
    virtual void unlockScreen();
    virtual void setShakePos(int shakeOffset);

    virtual void showOverlay();
    virtual void hideOverlay();
    virtual void clearOverlay();
    virtual void grabOverlay(void *buf, int pitch);
    virtual void copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h);
    virtual int16 getOverlayHeight();
    virtual int16 getOverlayWidth();
    virtual Graphics::PixelFormat getOverlayFormat() const { return Graphics::createPixelFormat<555>(); }

    virtual bool showMouse(bool visible);
    virtual void warpMouse(int x, int y);
    virtual void setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY,
                                uint32 keycolor, bool dontScale,
                                const Graphics::PixelFormat *format = 0);
    virtual void setCursorPalette(const byte *colors, uint start, uint num);

    virtual bool pollEvent(Common::Event &event);

    virtual uint32 getMillis();
    virtual void delayMillis(uint msecs);

    virtual MutexRef createMutex(void);
    virtual void lockMutex(MutexRef mutex);
    virtual void unlockMutex(MutexRef mutex);
    virtual void deleteMutex(MutexRef mutex);

    virtual void quit();
    virtual Audio::Mixer *getMixer();
    virtual void getTimeAndDate(TimeDate &t) const;
    virtual void logMessage(LogMessageType::Type type, const char *message);

private:
    void pumpAudio();
    void convertGameBuffer(surface_t *dst);
    uint16 rgb555(byte r, byte g, byte b) const;

    Audio::MixerImpl *_mixer;
    Graphics::Surface _screenSurface;

    uint8 *_screen8;
    uint16 *_overlay16;
    uint16 _palette[256];
    byte _exactPalette[256 * 3];

    uint16 *_cursor16;
    uint8 *_cursor8;
    uint16 _cursorPalette[256];
    uint _cursorW, _cursorH;
    int _cursorHotX, _cursorHotY;
    int _cursorKey;
    bool _cursorUsesOwnPalette;
    bool _mouseVisible;

    int _mouseX, _mouseY;
    int _screenW, _screenH;
    int _gameW, _gameH;
    int _shake;
    bool _overlayVisible;
    bool _dirty;
    int _gfxMode;

    uint16 _prevButtons;
};
#endif
