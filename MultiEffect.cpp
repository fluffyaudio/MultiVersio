#include "fx/multi_versio.h"
#include "daisy_versio.h"

MultiVersio *mv;

int main(void)
{
    mv = new MultiVersio();
    mv->run();
}
