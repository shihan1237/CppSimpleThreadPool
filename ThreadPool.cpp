
#include "ThreadPool.h"


int main(){

    ThreadPool pool;
    pool.init();
    for (int i = 1; i <= 3; ++i){
        for (int j = 1; j <= 3; ++j)
        {
            pool.submit([](int i, int j){
                printf("%d * %d = %d\n", i, j, i * j);
            }, i, j);
        }
    }
    pool.shutdown();
    
    return 0;
}