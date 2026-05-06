#include "Sandbox.h"

int main()
{
    Sandbox app;
    if (!app.Init(SANDBOX_ASSET_DIR))
        return -1;

    app.Run();
    return 0;
}
