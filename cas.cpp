#include <bits/stdc++.h>
#include <atomic>

using namespace std;

volatile std::atomic_bool isReady = ATOMIC_VAR_INIT(false);
volatile std::atomic_int mycount = ATOMIC_VAR_INIT(0);

void task() {
    while (!isReady) {
        std::this_thread::yield();
    }
    for (int i = 0; i < 1000; i++) {
        mycount++;
    }
}

int main() {
    list<std::thread> tlist;
    for (int i = 0; i < 100; i++) {
        tlist.push_back(std::thread(task));
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    cout << "start run" << endl;
    isReady = true;

    for (std::thread &t : tlist) {
        t.join();
    }
    cout << "mycount:" << mycount << endl;
    return 0;
}
