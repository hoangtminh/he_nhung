#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

    void gotoScreen2ScreenNoTransition();
    void gotoScreen2ScreenNoTransitionImpl();
    void gotoScreen3ScreenNoTransition();
    void gotoScreen3ScreenNoTransitionImpl();

private:
    touchgfx::Callback<FrontendApplication> transitionCallback2;
    touchgfx::Callback<FrontendApplication> transitionCallback3;
};

#endif // FRONTENDAPPLICATION_HPP
