#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{

}

void FrontendApplication::gotoScreen2ScreenNoTransition()
{
    transitionCallback2 = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoScreen2ScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &transitionCallback2;
}

void FrontendApplication::gotoScreen2ScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen2View, Screen2Presenter, touchgfx::NoTransition, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoScreen3ScreenNoTransition()
{
    transitionCallback3 = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoScreen3ScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &transitionCallback3;
}

void FrontendApplication::gotoScreen3ScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen3View, Screen3Presenter, touchgfx::NoTransition, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}
