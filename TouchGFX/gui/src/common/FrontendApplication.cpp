#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

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
