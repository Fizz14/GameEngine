#include <iostream>
#include <vector>
#include <unistd.h>
#include <pthread.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <fcntl.h>

using namespace std;

void* printpid(void* arg) {
    cout << "child's pid is " << getpid() << endl;
    return NULL;
}

int main(int argc, char* argv[]) {
    // cout << "Running..." << endl;
    // int foo[5] = {1,2,3,4,5};
    // foo[0] = 0;
    // cout << foo[0] << " " << foo[4] << endl;
    cout << "my pid is " << getpid() << endl;

    pthread_t t1;
    pthread_create(&t1, NULL, &printpid, NULL);
    sleep(1000);
}